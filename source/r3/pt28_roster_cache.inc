/* Cache stack: the roster-array (239) part of the PT28 playing-time room, per (team, position group).
   Each PT28 grade rebuilds the team's room: scheme jobs, the whole roster array 239, the incoming array 33 and the
   board. About 1,100 grades per team per week ask for the same (team, group) roster part again.
   Key: the PT28 index scope (franchise, league, year, week, type, stage, offseason, index epoch), the team record
   and index, the position group, the departure filter (flag, departure table, count and content hash, recomputed
   on every departure re-read) and the roster array identity and length. Only a complete successful read with at
   most PT28_ROSTER_CACHE_MAXP group players is stored. The scheme jobs, the incoming array, the board, the
   pending ledger and the candidate are still read live on every call. The grade runs under pt28_lock.
   It assumes a rostered player's position, OVR, class, team or redshirt does not change inside one PT28 scope
   without the roster array changing; ratings and positions change at stage boundaries, which change the scope.
   shadow: the live read decides and every would-be hit is compared player by player (PT28_ROSTER). */
#define PT28_ROSTER_CACHE_MAXP 32
typedef struct {U ptable;uint32_t pid,league;int year,week,type,stage,offseason;LONG epoch;U team_table;uint32_t team_row,tid;
 int group,use_dep;uint64_t dep_hash;U dep_table;unsigned dep_count;U atable;uint32_t arow;U n;} Pt28RosterKey;
typedef struct {int valid,count;Pt28RosterKey key;Pt28Player players[PT28_ROSTER_CACHE_MAXP];} Pt28RosterEntry;
static Pt28RosterEntry *pt28_roster_cache;
static int pt28_player_same(const Pt28Player *a,const Pt28Player *b) {
 return a->table==b->table&&a->id==b->id&&a->pos==b->pos&&a->ovr==b->ovr&&a->year==b->year&&a->hs==b->hs&&
  a->develop==b->develop&&a->redshirt==b->redshirt&&a->arrival_delay==b->arrival_delay&&a->rostered==b->rostered;
}
static int pt28_roster_cached(Pt28Room *room,Rec team,int group,int use_dep) {
 if(!cache_on())return pt28_roster_array(room,team,239,0,group,use_dep);
 Rec a=array_field(team,239);unsigned tid=scalar(team,383,0x100);
 if(!a || tid>=138 || group<0 || group>=18 || room->count){release(a);return pt28_roster_array(room,team,239,0,group,use_dep);}
 Pt28RosterKey k;memset(&k,0,sizeof(k));
 k.ptable=pt28_index.table;k.pid=pt28_index.id;k.league=pt28_index.league;k.year=pt28_index.year;k.week=pt28_index.week;
 k.type=pt28_index.type;k.stage=pt28_index.stage;k.offseason=pt28_index.offseason;k.epoch=pt28_index.epoch;
 k.team_table=q((U)team+0x10);k.team_row=row(team);k.tid=tid;k.group=group;k.use_dep=use_dep;
 if(use_dep){k.dep_hash=pt28_departures_hash;k.dep_table=pt28_index.departed.table;k.dep_count=pt28_index.departed.count;}
 k.atable=q((U)a+0x10);k.arow=row(a);k.n=array_count(a);release(a);
 if(!pt28_roster_cache)pt28_roster_cache=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,138*18*sizeof(*pt28_roster_cache));
 if(!pt28_roster_cache)return pt28_roster_array(room,team,239,0,group,use_dep);
 Pt28RosterEntry *e=&pt28_roster_cache[tid*18+(unsigned)group];
 int hit=e->valid && !memcmp(&e->key,&k,sizeof(k));
 CacheCount *c=&cache_counts[CACHE_PT28_ROSTER];c->queries++;if(hit)c->hits++;
 if(hit && cache_fast()){memcpy(room->players,e->players,(size_t)e->count*sizeof(*e->players));room->count=e->count;return 1;}
 int ok=pt28_roster_array(room,team,239,0,group,use_dep);
 if(hit) {
  int same=ok && room->count==e->count;
  for(int i=0;same && i<room->count;i++)if(!pt28_player_same(&room->players[i],&e->players[i]))same=0;
  c->compares++;
  if(!same && cache_mismatch(CACHE_PT28_ROSTER))
   log_line("CACHE_STACK_MISMATCH,PT28_ROSTER,-,%d,%d,%d,team=%u,group=%d,live_ok=%d,live_count=%d,cached_count=%d,n=%llu",
    pt28_index.year,pt28_index.type,pt28_index.week,tid,group,ok,room->count,e->count,(unsigned long long)k.n);
 }
 if(ok && room->count<=PT28_ROSTER_CACHE_MAXP){memcpy(&e->key,&k,sizeof(k));e->count=room->count;
  memcpy(e->players,room->players,(size_t)room->count*sizeof(*e->players));e->valid=1;}
 else e->valid=0;
 return ok;
}
