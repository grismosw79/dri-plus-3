/* Cache stack: the two league-wide scans under the HS snapshots and portal rooms (see cache_stack.inc).
   Owner index. portal_published_owner(recruit) walks all 138 teams' published players (array 33) to find who owns
   ONE recruit's player, thousands of times per window. One pass builds a player -> owner index with the scan's
   own rules: a player in two teams is -1, and any unreadable team, array or entry, or an array over 192, makes
   every query -1, as the scan fails on the first unreadable element. A full index is never used.
   Secured list. hs15_secured and portal_commits walk the whole franchise recruit list (39 HS pool, 46 portal pool)
   only to find the stage 5/6 entries. The cache keeps the ordered indices of those entries and the first
   unreadable index; the per-entry work (hs15_secured_entry / portal_commits_entry, the original loop bodies) runs
   on the same entries in the same order.
   Valid within one window and one epoch (no sub-phase boundary), for the same eval, franchise (table, row) and
   team count or list identity and length; the secured list also only until DRI writes a recruit stage. */
#define OWNER_INDEX_SLOTS 32768
typedef struct {U table;uint32_t row;int owner;int used;} OwnerIndexEntry; /* owner: tid, or -2 = in two teams */
static struct {int valid,status;unsigned epoch;U eval,ftable;uint32_t frow;U n;OwnerIndexEntry *slot;} owner_index;
typedef struct {int valid;unsigned epoch,gen;U eval,ftable,atable;uint32_t frow,arow;U n;U null_at;unsigned count;uint32_t idx[12001];} SecuredList;
static SecuredList *secured_lists[2];
static int portal_published_owner_live(Rec recruit,U eval,int *tid);
static OwnerIndexEntry *owner_index_slot(U table,uint32_t rid,int create) {
 unsigned at=(unsigned)((table>>4)^((U)rid*2654435761u))&(OWNER_INDEX_SLOTS-1);
 for(unsigned i=0;i<OWNER_INDEX_SLOTS;i++,at=(at+1)&(OWNER_INDEX_SLOTS-1)) {
  OwnerIndexEntry *e=&owner_index.slot[at];
  if(e->used && e->table==table && e->row==rid)return e;
  if(!e->used){if(!create)return 0;e->used=1;e->table=table;e->row=rid;e->owner=-1;return e;}
 }
 return 0;
}
/* The loop and failure rules of portal_published_owner_live, for every player at once. */
static void owner_index_build(Rec teams,U n) {
 memset(owner_index.slot,0,OWNER_INDEX_SLOTS*sizeof(*owner_index.slot));owner_index.status=1;
 for(U i=0;i<n && owner_index.status>0;i++) {
  Rec team=array_at(teams,i);if(!team){owner_index.status=-1;break;}
  unsigned id=scalar(team,383,0x100);
  Rec a=array_field(team,33);
  if(id>=138 || !a){release(a);release(team);owner_index.status=-1;break;}
  U count=array_count(a);
  if(count>192){release(a);release(team);owner_index.status=-1;break;}
  for(U j=0;j<count;j++) {
   Rec p=array_at(a,j);if(!p){owner_index.status=-1;break;}
   OwnerIndexEntry *e=owner_index_slot(q((U)p+0x10),row(p),1);release(p);
   if(!e){owner_index.status=-2;break;} /* index full: never decide from it */
   if(e->owner>=0 && e->owner!=(int)id)e->owner=-2;else if(e->owner!=-2)e->owner=(int)id;
  }
  release(a);release(team);
 }
}
/* -3: not applicable (the caller runs the scan); otherwise the scan's exact result. */
static int owner_index_lookup(Rec recruit,U eval,int *tid) {
 if(tid)*tid=-1;
 if(!owner_index.slot){owner_index.slot=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,OWNER_INDEX_SLOTS*sizeof(*owner_index.slot));if(!owner_index.slot)return -3;}
 Rec player=reference(recruit,5),franchise=context_ref(eval,6);
 if(!player || !franchise){release(player);release(franchise);return -1;}
 Rec teams=array_field(franchise,45);U ftable=q((U)franchise+0x10);uint32_t frow=row(franchise);release(franchise);
 if(!teams){release(player);return -1;}
 U n=array_count(teams);if(n>138){release(teams);release(player);return -1;}
 if(!owner_index.valid || owner_index.epoch!=cache.epoch || owner_index.eval!=eval || owner_index.ftable!=ftable ||
    owner_index.frow!=frow || owner_index.n!=n) {
  owner_index_build(teams,n);owner_index.valid=1;owner_index.epoch=cache.epoch;owner_index.eval=eval;
  owner_index.ftable=ftable;owner_index.frow=frow;owner_index.n=n;
 }
 release(teams);
 int result;
 if(owner_index.status==-2)result=-3;
 else if(owner_index.status<0)result=-1;
 else {
  OwnerIndexEntry *e=owner_index_slot(q((U)player+0x10),row(player),0);
  if(!e)result=0;else if(e->owner==-2)result=-1;else {if(tid)*tid=e->owner;result=1;}
 }
 release(player);return result;
}
static int portal_published_owner(Rec recruit,U eval,int *tid) {
 if(cache_scan_ready()) {
  int cached_tid=-1,cached=owner_index_lookup(recruit,eval,&cached_tid);
  if(cached!=-3) {
   CacheCount *c=&cache_counts[CACHE_OWNER_INDEX];c->queries++;c->hits++;
   if(cache_shadow()) {
    int live_tid=-1,live=portal_published_owner_live(recruit,eval,&live_tid);c->compares++;
    if((live!=cached || (live==1 && live_tid!=cached_tid)) && cache_mismatch(CACHE_OWNER_INDEX))
     log_line("CACHE_STACK_MISMATCH,OWNER_INDEX,%s,recruit=%u,live=%d,live_tid=%d,cached=%d,cached_tid=%d,status=%d,teams=%llu",
      cache.site?cache.site:"-",recruit?row(recruit):0,live,live_tid,cached,cached_tid,owner_index.status,(unsigned long long)owner_index.n);
    if(tid)*tid=live_tid;return live;
   }
   if(tid)*tid=cached==1?cached_tid:-1;return cached;
  }
 }
 return portal_published_owner_live(recruit,eval,tid);
}
/* The ordered stage-5/6 indices of franchise list `ordinal` (39 HS pool, 46 portal pool) and its first null entry,
   built with the reads the original loop makes. 0 = not applicable. */
static SecuredList *secured_list_get(Rec franchise,Rec a,U n,U eval,int ordinal) {
 int k=ordinal==39?0:ordinal==46?1:-1;if(k<0 || n>12000)return 0;
 if(!secured_lists[k]){secured_lists[k]=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(SecuredList));if(!secured_lists[k])return 0;}
 SecuredList *c=secured_lists[k];
 U ftable=q((U)franchise+0x10),atable=q((U)a+0x10);uint32_t frow=row(franchise),arow=row(a);
 if(c->valid && c->epoch==cache.epoch && c->gen==cache.stage_gen && c->eval==eval && c->ftable==ftable &&
    c->frow==frow && c->atable==atable && c->arow==arow && c->n==n)return c;
 c->valid=0;c->count=0;c->null_at=n;
 for(U j=0;j<n;j++) {
  Rec r=array_at(a,j);if(!r){c->null_at=j;break;}
  unsigned stage=scalar(r,9,0x108);release(r);
  if(stage==5 || stage==6)c->idx[c->count++]=(uint32_t)j;
 }
 c->valid=1;c->epoch=cache.epoch;c->gen=cache.stage_gen;c->eval=eval;c->ftable=ftable;c->frow=frow;c->atable=atable;c->arow=arow;c->n=n;
 return c;
}
/* SHADOW: the cached list must hold exactly the stage-5/6 entries the live loop visited, in order (same_order,
   live_count), and the same first unreadable entry. stop = where the live loop stopped (n = ran to the end),
   null_stop = it stopped at an unreadable entry. */
static int secured_list_same(const SecuredList *c,int same_order,unsigned live_count,U stop,int null_stop,U n) {
 unsigned expect=0;int same=same_order;
 for(unsigned i=0;i<c->count;i++)if(null_stop || stop==n ? c->idx[i]<stop : c->idx[i]<=stop)expect++;
 if(expect!=live_count)same=0;
 if(null_stop ? c->null_at!=stop : stop==n ? c->null_at!=n : c->null_at<=stop)same=0;
 return same;
}
/* The site of the next portal recovery plan loop (set by its two callers): the loop runs inside a window. */
static const char *recover_window_site;
