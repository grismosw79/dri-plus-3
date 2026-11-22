/* Ownership index for the BEFORE check of each move and cut in the transaction pass.
   rm_owned(p,src) walks all 138 teams' roster (239) and arrivals (33) arrays with two identity reads
   per record, once per action. rm_owned_before answers the same check from an index.
   The index is built at the first BEFORE check of a pass from one league read that uses rm_owned's
   own calls and rules: per array, whether it is readable and within RM_ROOM, and the identities of
   its non-empty readable entries; a hash map counts each identity's occurrences and sums their
   team*2+slot codes. After every executed action (moved, cut, stale or failed) its source and
   destination teams are marked dirty, and the next check re-reads both arrays of each dirty team
   first. The verdict is rm_owned's rule on the indexed arrays: 0 if any array is unreadable or
   oversize; otherwise 0 occurrences when no owner is expected, or exactly one occurrence, in the
   expected team's roster array.
   It equals rm_owned whenever arrays of teams the pass did not touch are unchanged since they were
   read. The pass runs synchronously inside the enrollment callback and its only writes are its own
   moves, cuts and NIL payments on the source and destination. The AFTER and FINAL checks stay
   rm_owned (full league), so a native side effect on a third team would still stop the pass at the
   AFTER check of the next action that involves that player. The index is active only inside
   rm_run_plan, between rm_owner_index_begin and rm_owner_index_end; any failure to build or update
   it falls back to rm_owned for that check. tests/test_roster_owner_index.py compares it with rm_owned. */
#define RM_OWNER_SLOTS 131072u
typedef struct {U store;uint32_t row;int used,count,code_sum;} RMOwnerEntry;
typedef struct {int state,n;U store[RM_ROOM];uint32_t row[RM_ROOM];} RMOwnerArray; /* state 0 unread, 1 valid, 2 unreadable or oversize */
static struct {
 int active,built,bad;unsigned char dirty[RM_TEAMS];RMOwnerEntry *map;RMOwnerArray *arr;
 long queries,builds,rescans,fallback;
} rm_oi;
static RMOwnerEntry *rm_oi_slot(U store,uint32_t row,int create){
 uint64_t h=(uint64_t)store*0x9E3779B97F4A7C15ull^(uint64_t)row*0xC2B2AE3D27D4EB4Full;h^=h>>33;h*=0xff51afd7ed558ccdull;h^=h>>33;
 unsigned at=(unsigned)h&(RM_OWNER_SLOTS-1);
 for(unsigned i=0;i<RM_OWNER_SLOTS;i++,at=(at+1)&(RM_OWNER_SLOTS-1)){
  RMOwnerEntry *e=&rm_oi.map[at];
  if(e->used&&e->store==store&&e->row==row)return e;
  if(!e->used){if(!create)return 0;e->used=1;e->store=store;e->row=row;e->count=0;e->code_sum=0;return e;}
 }
 return 0;
}
/* Re-read one array (team t, slot 0 = roster 239, 1 = arrivals 33) with rm_owned's calls; 0 = the index is unusable. */
static int rm_oi_scan(int t,int slot){
 RMOwnerArray *x=&rm_oi.arr[t*2+slot];int code=t*2+slot;
 if(x->state==1)for(int i=0;i<x->n;i++){RMOwnerEntry *e=rm_oi_slot(x->store[i],x->row[i],0);if(!e)return 0;e->count--;e->code_sum-=code;}
 if(x->state==2)rm_oi.bad--;
 x->state=0;x->n=0;
 int ok=0;Rec a=array_field(rm_teams[t],slot?33:239,&ok);U n=array_count(a);
 if(!ok||n>RM_ROOM){release(a);x->state=2;rm_oi.bad++;return 1;}
 for(U i=0;i<n;i++){Rec q=array_at(a,i,&ok);U s=0;uint32_t r=0;int have=q&&identity(q,&s,&r);release(q);
  if(!ok){x->n=0;x->state=2;rm_oi.bad++;release(a);return 1;}
  if(have){x->store[x->n]=s;x->row[x->n]=r;x->n++;}}
 release(a);
 for(int i=0;i<x->n;i++){RMOwnerEntry *e=rm_oi_slot(x->store[i],x->row[i],1);if(!e)return 0;e->count++;e->code_sum+=code;}
 x->state=1;return 1;
}
/* 1/0 = the index verdict, -1 = index unavailable (the caller uses rm_owned). */
static int rm_oi_verdict(Rec p,int expected){
 if(!rm_oi.map)rm_oi.map=calloc(RM_OWNER_SLOTS,sizeof(RMOwnerEntry));
 if(!rm_oi.arr)rm_oi.arr=calloc((size_t)RM_TEAMS*2,sizeof(RMOwnerArray));
 if(!rm_oi.map||!rm_oi.arr)return -1;
 if(!rm_oi.built){
  memset(rm_oi.map,0,sizeof(RMOwnerEntry)*RM_OWNER_SLOTS);memset(rm_oi.arr,0,sizeof(RMOwnerArray)*RM_TEAMS*2);
  rm_oi.bad=0;memset(rm_oi.dirty,0,sizeof(rm_oi.dirty));
  for(int t=0;t<RM_TEAMS;t++)for(int slot=0;slot<2;slot++)if(!rm_oi_scan(t,slot))return -1;
  rm_oi.built=1;rm_oi.builds++;
 }
 else for(int t=0;t<RM_TEAMS;t++)if(rm_oi.dirty[t]){rm_oi.dirty[t]=0;rm_oi.rescans++;
  for(int slot=0;slot<2;slot++)if(!rm_oi_scan(t,slot)){rm_oi.built=0;return -1;}}
 if(rm_oi.bad)return 0;
 U s=0;uint32_t r=0;if(!identity(p,&s,&r))return expected<0;
 const RMOwnerEntry *e=rm_oi_slot(s,r,0);int k=e?e->count:0;
 if(expected<0)return k==0;
 return k==1&&e->code_sum==expected*2;
}
/* The BEFORE check of a move or cut (rm_owned's place in rm_native_execute and rm_native_cut). */
static int rm_owned_before(Rec p,int expected){
 if(!rm_oi.active)return rm_owned(p,expected);
 rm_oi.queries++;int v=rm_oi_verdict(p,expected);
 if(v<0){rm_oi.built=0;rm_oi.fallback++;return rm_owned(p,expected);}
 return v;
}
/* After every executed action: the arrays it may have changed. */
static void rm_owner_index_touch(int t){if(rm_oi.active&&t>=0&&t<RM_TEAMS)rm_oi.dirty[t]=1;}
static void rm_owner_index_begin(void){
 rm_oi.active=1;rm_oi.built=0;rm_oi.queries=rm_oi.builds=rm_oi.rescans=rm_oi.fallback=0;
}
static void rm_owner_index_end(void){
 if(!rm_oi.active)return;
 log_line("RM18R22 ROSTER_OWNER_INDEX queries=%ld builds=%ld rescans=%ld fallback=%ld",rm_oi.queries,rm_oi.builds,rm_oi.rescans,rm_oi.fallback);
 rm_oi.active=0;rm_oi.built=0;free(rm_oi.map);free(rm_oi.arr);rm_oi.map=0;rm_oi.arr=0;
}
