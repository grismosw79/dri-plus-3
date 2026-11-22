/* Cache stack: the HS roster snapshot memo (hs15_collect; see cache_stack.inc).
   Every acceptance check rebuilds the school's whole snapshot (scheme, roster array 239, incoming array 33,
   departures, the stage-5/6 walk of lists 39/46 with their ownership), usually two or three times per decision
   (hs15_decision, hs28_commit_block, test29r1_capacity_block), and the same school again for the next recruit.
   Key: the school record (table, row), the eval and the eval tuning record. Valid while neither the snapshot
   generation (cache.gen minus the excused writes below) nor cache.epoch has moved, on the thread that opened the
   window. Excused writes: a recruit stage write whose old and new value are both below 5, and the HS preference
   re-ordering writes of HS recruits below stage 4 (portal_recovery_scalar classifies them); a snapshot reads only
   the stage-5/6 entries of the lists and their ownership, never those fields.
   Only a clean live call is stored: it returned 1, made no log_line call (counted before any filter), moved
   neither the generation nor the epoch (so it stored no ledger entry, invalidated nothing and entered no hook) and
   did not fault. Such a call has no effect beyond the snapshot it fills (the thread's ledger-scope generation, the
   only other state it touches, already holds the value it would set), so answering a later identical query from
   it is the same as running it again. Failed and unclean calls always run live.
   shadow: the live collect decides; a valid entry is compared field by field (SNAPSHOT), and a would-be hit whose
   live call was not clean is also reported (SNAPSHOT_SIDE_EFFECT). */
#define SNAPSHOT_MEMO_SLOTS 256u
#define SNAPSHOT_MEMO_LIMIT 200u
typedef struct {U ttable,eval,tuning;uint32_t trow,stamp;Hs15Snapshot snap;} SnapshotMemoEntry;
static struct {SnapshotMemoEntry *slot;unsigned stamp,s_gen,s_epoch,count;} snapshot_memo;
static int snapshot_same(const Hs15Snapshot *a,const Hs15Snapshot *b,int *at) {
 /* The head (int arrays and ints up to players[]) and the tail (annual...) have no padding; each used player is
    compared by field (Hs15Player has tail padding). Entries beyond used are zero in both. */
 *at=-1;
 if(memcmp(a,b,offsetof(Hs15Snapshot,players))){*at=-2;return 0;}
 if(memcmp(&a->annual,&b->annual,sizeof(*a)-offsetof(Hs15Snapshot,annual))){*at=-3;return 0;}
 for(int i=0;i<a->used && i<HS15_MAX_PLAYERS;i++) {
  const Hs15Player *x=&a->players[i],*y=&b->players[i];
  if(x->table!=y->table || x->rid!=y->rid || x->pos!=y->pos || x->ovr!=y->ovr || x->returning!=y->returning ||
     x->incoming!=y->incoming || x->hs!=y->hs || x->stars!=y->stars || x->published!=y->published ||
     x->qb_young!=y->qb_young || x->qb_priority!=y->qb_priority){*at=i;return 0;}
 }
 return 1;
}
/* The table is logically empty whenever the snapshot generation or the epoch has moved since it was filled. */
static void snapshot_memo_sync(void) {
 unsigned g=cache.gen-cache.excused,ep=cache.epoch;
 if(!snapshot_memo.stamp || g!=snapshot_memo.s_gen || ep!=snapshot_memo.s_epoch) {
  snapshot_memo.stamp++;if(!snapshot_memo.stamp)snapshot_memo.stamp=1;
  snapshot_memo.s_gen=g;snapshot_memo.s_epoch=ep;snapshot_memo.count=0;
 }
}
/* The matching entry (*found=1), the free slot to store into (*found=0), or 0 (unavailable or full). */
static SnapshotMemoEntry *snapshot_memo_find(Rec team,U eval,U tuning,int *found) {
 *found=0;
 if(!snapshot_memo.slot) {
  snapshot_memo.slot=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(SnapshotMemoEntry)*SNAPSHOT_MEMO_SLOTS);
  if(!snapshot_memo.slot)return 0;
 }
 snapshot_memo_sync();
 U tt=q((U)team+0x10);uint32_t tr=row(team);
 uint64_t h=(uint64_t)tt*0x9E3779B97F4A7C15ull^(uint64_t)tr*0xC2B2AE3D27D4EB4Full^(uint64_t)eval*0x165667B19E3779F9ull;
 h^=h>>33;h*=0xff51afd7ed558ccdull;h^=h>>33;h*=0xc4ceb9fe1a85ec53ull;h^=h>>33;
 unsigned at=(unsigned)h&(SNAPSHOT_MEMO_SLOTS-1);
 for(unsigned i=0;i<SNAPSHOT_MEMO_SLOTS;i++,at=(at+1)&(SNAPSHOT_MEMO_SLOTS-1)) {
  SnapshotMemoEntry *e=&snapshot_memo.slot[at];
  if(e->stamp!=snapshot_memo.stamp)return snapshot_memo.count<SNAPSHOT_MEMO_LIMIT?e:0;
  if(e->ttable==tt && e->trow==tr && e->eval==eval && e->tuning==tuning){*found=1;return e;}
 }
 return 0;
}
static int snapshot_memo_collect(Hs15Snapshot *s,Rec team,U eval) {
 if(!cache_memo_ready() || !team || !eval || GetCurrentThreadId()!=cache.tid)return hs15_collect_live(s,team,eval);
 CacheCount *c=&cache_counts[CACHE_SNAPSHOT];c->queries++;
 U tuning=q(eval+0xd0);int found=0;
 SnapshotMemoEntry *e=snapshot_memo_find(team,eval,tuning,&found);
 if(found)c->hits++;
 if(found && cache_fast()){memcpy(s,&e->snap,sizeof(*s));return 1;}
 uint64_t logs0=cache_log_calls;unsigned gen0=cache.gen-cache.excused,epoch0=cache.epoch;int faulted0=faulted;
 int v=hs15_collect_live(s,team,eval);
 int clean=v==1 && !faulted && !faulted0 && mode==1 && cache_log_calls==logs0 && cache.gen-cache.excused==gen0 && cache.epoch==epoch0;
 if(found) {
  int at=-1;c->compares++;
  if(v!=1 || !snapshot_same(s,&e->snap,&at)) {
   if(cache_mismatch(CACHE_SNAPSHOT))log_line("CACHE_STACK_MISMATCH,SNAPSHOT,%s,team=%u,live_ret=%d,live_used=%d,memo_used=%d,first_diff=%d,gen=%u,epoch=%u",
    cache.site?cache.site:"-",scalar(team,383,0x100),v,v==1?s->used:-1,e->snap.used,at,cache.gen,cache.epoch);
  } else if(!clean) {
   if(cache_mismatch(CACHE_SNAPSHOT))log_line("CACHE_STACK_MISMATCH,SNAPSHOT_SIDE_EFFECT,%s,team=%u,logs=%llu,gen_moved=%d,epoch_moved=%d,faulted=%d",
    cache.site?cache.site:"-",scalar(team,383,0x100),(unsigned long long)(cache_log_calls-logs0),cache.gen-cache.excused!=gen0,cache.epoch!=epoch0,faulted);
  }
 }
 if(clean) {
  /* Stored under the (generation, epoch) the call ended in; a clean call moved neither. */
  if(e) {
   if(!found)snapshot_memo.count++;
   e->ttable=q((U)team+0x10);e->trow=row(team);e->eval=eval;e->tuning=tuning;
   e->stamp=snapshot_memo.stamp;memcpy(&e->snap,s,sizeof(*s));
  }
 } else c->unclean++;
 return v;
}
