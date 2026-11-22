/* Cache stack: the portal-room memo (portal_rosters_collect for one position; see cache_stack.inc).
   Every portal_decision, portal_roster and portal_commit_room builds the school's room for one position (scheme,
   roster arrays 239/33, departures, the stage-5/6 walk of lists 39/46 with their ownership). In the portal windows
   the funding observer, the executed-action witnesses and the recovery plan rebuild the same school's rooms again
   and again: every firm check runs the commitment and capacity arbitration, which asks portal_decision for every
   other board target of the school.
   Key: the school record (table, row), the eval, the eval tuning record and the position. Valid within one
   generation (the full one: no excused writes), one epoch and one class epoch (every test29_class_invalidate), on
   the thread that opened the window. Only a clean live call is stored: it returned 1, made no log_line call, moved
   neither the generation nor an epoch (no ownership-ledger store, no PT28 invalidation, no hook entry), did not
   fault and left the caller's issue record untouched. A hit copies the whole PortalRoster.
   shadow: the live collect decides; field-by-field compare (ROOM), and a would-be hit whose live call was not
   clean is also reported (ROOM_SIDE_EFFECT). */
#define ROOM_MEMO_SLOTS 2048u
#define ROOM_MEMO_LIMIT 1433u
typedef struct {U ttable,eval,tuning;uint32_t trow,stamp;int pos;PortalRoster room;} RoomMemoEntry;
static struct {RoomMemoEntry *slot;unsigned stamp,s_gen,s_epoch,s_class,count;} room_memo;
static int portal_rosters_collect_live(PortalRoster *s,int rooms,Rec team,U context,int single_pos,PortalRoomIssue *issue);
static int room_same(const PortalRoster *a,const PortalRoster *b,int *at) {
 /* The head (eight ints) and both PortalQuality blocks are plain ints; each used player is compared by field
    (PortalPlayer has tail padding). Players beyond count are zero in both (the live collect clears the struct). */
 *at=-1;
 if(memcmp(a,b,offsetof(PortalRoster,players))){*at=-2;return 0;}
 if(memcmp(&a->quality,&b->quality,sizeof(a->quality))){*at=-3;return 0;}
 if(memcmp(&a->projected,&b->projected,sizeof(a->projected))){*at=-4;return 0;}
 for(int i=0;i<a->count && i<192;i++) {
  const PortalPlayer *x=&a->players[i],*y=&b->players[i];
  if(x->table!=y->table || x->row!=y->row || x->ovr!=y->ovr || x->portal!=y->portal || x->signed_here!=y->signed_here ||
     x->develop!=y->develop || x->qb_young!=y->qb_young || x->qb_priority!=y->qb_priority){*at=i;return 0;}
 }
 return 1;
}
/* The table is logically empty whenever the generation, the epoch or the class epoch has moved. */
static void room_memo_sync(void) {
 if(!room_memo.stamp || cache.gen!=room_memo.s_gen || cache.epoch!=room_memo.s_epoch || cache.class_epoch!=room_memo.s_class) {
  room_memo.stamp++;if(!room_memo.stamp)room_memo.stamp=1;
  room_memo.s_gen=cache.gen;room_memo.s_epoch=cache.epoch;room_memo.s_class=cache.class_epoch;room_memo.count=0;
 }
}
static RoomMemoEntry *room_memo_find(Rec team,U eval,U tuning,int pos,int *found) {
 *found=0;
 if(!room_memo.slot) {
  room_memo.slot=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(RoomMemoEntry)*ROOM_MEMO_SLOTS);
  if(!room_memo.slot)return 0;
 }
 room_memo_sync();
 U tt=q((U)team+0x10);uint32_t tr=row(team);
 uint64_t h=(uint64_t)tt*0x9E3779B97F4A7C15ull^(uint64_t)tr*0xC2B2AE3D27D4EB4Full^(uint64_t)eval*0x165667B19E3779F9ull^(uint64_t)(unsigned)pos*0x27D4EB2F165667C5ull;
 h^=h>>33;h*=0xff51afd7ed558ccdull;h^=h>>33;h*=0xc4ceb9fe1a85ec53ull;h^=h>>33;
 unsigned at=(unsigned)h&(ROOM_MEMO_SLOTS-1);
 for(unsigned i=0;i<ROOM_MEMO_SLOTS;i++,at=(at+1)&(ROOM_MEMO_SLOTS-1)) {
  RoomMemoEntry *e=&room_memo.slot[at];
  if(e->stamp!=room_memo.stamp)return room_memo.count<ROOM_MEMO_LIMIT?e:0;
  if(e->ttable==tt && e->trow==tr && e->eval==eval && e->tuning==tuning && e->pos==pos){*found=1;return e;}
 }
 return 0;
}
static int portal_rosters_collect(PortalRoster *s,int rooms,Rec team,U context,int single_pos,PortalRoomIssue *issue) {
 if(rooms!=1 || !cache_memo_ready() || !team || !context || single_pos<0 || single_pos>=POSITIONS || GetCurrentThreadId()!=cache.tid)
  return portal_rosters_collect_live(s,rooms,team,context,single_pos,issue);
 CacheCount *c=&cache_counts[CACHE_ROOM];c->queries++;
 U tuning=q(context+0xd0);int found=0;
 RoomMemoEntry *e=room_memo_find(team,context,tuning,single_pos,&found);
 if(found)c->hits++;
 if(found && cache_fast()){memcpy(s,&e->room,sizeof(*s));return 1;}
 PortalRoomIssue issue0={0};if(issue)issue0=*issue;
 uint64_t logs0=cache_log_calls;unsigned gen0=cache.gen,epoch0=cache.epoch,class0=cache.class_epoch;int faulted0=faulted;
 int v=portal_rosters_collect_live(s,rooms,team,context,single_pos,issue);
 int clean=v==1 && !faulted && !faulted0 && mode==1 && cache_log_calls==logs0 && cache.gen==gen0 && cache.epoch==epoch0 &&
  cache.class_epoch==class0 && (!issue || !memcmp(&issue0,issue,sizeof(issue0)));
 if(found) {
  int at=-1;c->compares++;
  if(v!=1 || !room_same(s,&e->room,&at)) {
   if(cache_mismatch(CACHE_ROOM))log_line("CACHE_STACK_MISMATCH,ROOM,%s,team=%u,pos=%d,live_ret=%d,live_count=%d,memo_count=%d,first_diff=%d,gen=%u,epoch=%u",
    cache.site?cache.site:"-",scalar(team,383,0x100),single_pos,v,v==1?s->count:-1,e->room.count,at,cache.gen,cache.epoch);
  } else if(!clean) {
   if(cache_mismatch(CACHE_ROOM))log_line("CACHE_STACK_MISMATCH,ROOM_SIDE_EFFECT,%s,team=%u,pos=%d,logs=%llu,gen_moved=%d,epoch_moved=%d,class_moved=%d,faulted=%d",
    cache.site?cache.site:"-",scalar(team,383,0x100),single_pos,(unsigned long long)(cache_log_calls-logs0),cache.gen!=gen0,cache.epoch!=epoch0,
    cache.class_epoch!=class0,faulted);
  }
 }
 if(clean) {
  if(e) {
   if(!found)room_memo.count++;
   e->ttable=q((U)team+0x10);e->trow=row(team);e->eval=context;e->tuning=tuning;e->pos=single_pos;
   e->stamp=room_memo.stamp;memcpy(&e->room,s,sizeof(*s));
  }
 } else c->unclean++;
 return v;
}
