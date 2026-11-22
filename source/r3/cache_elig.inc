/* Cache stack: the native recruit-eligibility memo and the recovery-cost memo (see cache_stack.inc).
   Eligibility. Inside one window the same (school, recruit) eligibility is asked again and again: the abandoned-
   recruit recovery asks it in its school loop and again in the destination check, the coverage seed in its choice
   and again in the destination, and the seed, recovery and market forecast ask the same pairs. The memo keeps each
   (school record, recruit record, eval) answer for one generation, on the thread that opened the window. A call
   with an active HS34 trace (it wants the native reason text) or under a t36 relation always runs live and is not
   stored; only a fault-free call during which the generation did not move is stored. The record-type checks and
   their faults run before the memo, unchanged.
   It assumes the native predicate is a function of the game state plus the DRI state its nested grade hooks read,
   and that inside one generation neither changes: every DRI write path, native removal and non-grade hook entry
   advances the generation, and the grade hooks' own state (EPS, PT28, staff, RC4) changes only in hooks that do.
   Recovery cost. hs28_recovery_cost re-reads the eval tuning action list (about 40 reflection reads) on every
   call; the two answers (offered 0/1) are kept for one generation, eval and tuning record. The tuning record is
   game configuration that no DRI code writes.
   shadow: the live call decides; ELIGIBILITY / RECOVERY_COST mismatch rows. */
#define ELIG_MEMO_SLOTS (1u<<19)
#define ELIG_MEMO_LIMIT ((ELIG_MEMO_SLOTS/10)*7)
typedef struct {U ttable,rtable;uint32_t trow,rrow,gen;int allowed;} EligMemoEntry;
static struct {EligMemoEntry *slot;unsigned count,count_gen;U eval;} elig_memo;
static int hs34_trace_matches(Rec team,Rec recruit); /* portal.inc */
static void elig_memo_sync(void){if(elig_memo.count_gen!=cache.gen){elig_memo.count_gen=cache.gen;elig_memo.count=0;}}
/* Returns the slot to store into after a live call (-1: none); *cached = the valid memo answer or -1;
   *gen0 = the generation the answer belongs to. */
static int elig_memo_begin(U eval,Rec team,Rec recruit,int *cached,unsigned *gen0) {
 *cached=-1;*gen0=0;
 if(!cache_memo_ready() || GetCurrentThreadId()!=cache.tid)return -1;
 CacheCount *c=&cache_counts[CACHE_ELIGIBILITY];c->queries++;
 if(hs34_trace_matches(team,recruit) || t36_relation || t36_relation_team)return -1;
 if(!elig_memo.slot) {
  elig_memo.slot=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(EligMemoEntry)*ELIG_MEMO_SLOTS);
  if(!elig_memo.slot)return -1;
  cache_bump();
 }
 if(elig_memo.eval!=eval){cache_bump();elig_memo.eval=eval;}
 elig_memo_sync();*gen0=cache.gen;
 U tt=q((U)team+0x10),rt=q((U)recruit+0x10);uint32_t tr=row(team),rr=row(recruit);
 uint64_t h=(uint64_t)tt*0x9E3779B97F4A7C15ull^(uint64_t)rt*0xC2B2AE3D27D4EB4Full^((uint64_t)tr<<32|rr)*0x165667B19E3779F9ull;
 h^=h>>33;h*=0xff51afd7ed558ccdull;h^=h>>33;h*=0xc4ceb9fe1a85ec53ull;h^=h>>33;
 unsigned at=(unsigned)h&(ELIG_MEMO_SLOTS-1);
 for(unsigned i=0;i<256;i++,at=(at+1)&(ELIG_MEMO_SLOTS-1)) {
  EligMemoEntry *e=&elig_memo.slot[at];
  if(e->gen!=cache.gen)return elig_memo.count>=ELIG_MEMO_LIMIT?-1:(int)at;
  if(e->ttable==tt && e->trow==tr && e->rtable==rt && e->rrow==rr){*cached=e->allowed;c->hits++;return (int)at;}
 }
 return -1;
}
static void elig_memo_end(int slot,int cached,unsigned gen0,int allowed,Rec team,Rec recruit) {
 if(slot<0 || faulted || mode!=1 || !elig_memo.slot)return;
 EligMemoEntry *e=&elig_memo.slot[slot];
 if(cached>=0) {
  cache_counts[CACHE_ELIGIBILITY].compares++;
  if(cached!=allowed && cache_mismatch(CACHE_ELIGIBILITY))
   log_line("CACHE_STACK_MISMATCH,ELIGIBILITY,%s,team=%u,recruit=%u,memo=%d,live=%d,gen=%u",cache.site?cache.site:"-",
    scalar(team,383,0x100),row(recruit),cached,allowed,cache.gen);
  if(cache.gen==gen0)e->allowed=allowed;
  return;
 }
 if(cache.gen!=gen0){cache_counts[CACHE_ELIGIBILITY].unclean++;return;}
 elig_memo_sync();
 if(e->gen!=cache.gen)elig_memo.count++;
 e->ttable=q((U)team+0x10);e->trow=row(team);e->rtable=q((U)recruit+0x10);e->rrow=row(recruit);e->gen=cache.gen;e->allowed=allowed;
}

static struct {unsigned gen;U eval,tuning;int valid[2],value[2];} cost_memo;
/* -1: not used; 0: miss; 1: hit (*value). */
static int cost_memo_get(U eval,int offered,int *value) {
 if(!cache_memo_ready() || GetCurrentThreadId()!=cache.tid || offered<0 || offered>1)return -1;
 CacheCount *c=&cache_counts[CACHE_RECOVERY_COST];c->queries++;
 U tuning=q(eval+0xd0);
 if(cost_memo.gen!=cache.gen || cost_memo.eval!=eval || cost_memo.tuning!=tuning) {
  cost_memo.gen=cache.gen;cost_memo.eval=eval;cost_memo.tuning=tuning;cost_memo.valid[0]=cost_memo.valid[1]=0;
 }
 if(!cost_memo.valid[offered])return 0;
 c->hits++;*value=cost_memo.value[offered];return 1;
}
static void cost_memo_put(int state,int offered,int cached,int live) {
 if(state<0 || faulted || cost_memo.gen!=cache.gen)return;
 if(state==1) {
  cache_counts[CACHE_RECOVERY_COST].compares++;
  if(cached!=live && cache_mismatch(CACHE_RECOVERY_COST))
   log_line("CACHE_STACK_MISMATCH,RECOVERY_COST,%s,offered=%d,memo=%d,live=%d,gen=%u",cache.site?cache.site:"-",offered,cached,live,cache.gen);
 }
 cost_memo.valid[offered]=1;cost_memo.value[offered]=live;
}
