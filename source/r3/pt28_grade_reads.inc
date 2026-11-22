/* Cache stack: PT28 in-call reads (DriPt28Grade, about 2.5 M calls a season).
   Inside ONE grade call the original reads the franchise and season records twice (the ready check and the grade
   body), the season's stage / offseason / week fields up to three times (checkpoint, body, timing) and the graded
   player's six fields twice (the first pt28_player for the timing, the second only for the incoming flag). Nothing
   writes between those reads (the PT28 index build and the departures read only read; pt28_lock is held), and a
   reflection getter has no side effect, so the repeats return the same values.
   fast: the ready check's references are handed to the grade, the season fields are read once and passed to the
   checkpoint and the timing, and the second candidate is the first with only the incoming-dependent HS flag
   recomputed from the same two team fields.
   shadow: every original read is made at its original place, decides, and is compared with the value fast would
   use (PT28_READS). Otherwise this is pt28_grade_impl line for line. */
static int pt28_reads_cmp(int hoisted,int live,const char *what) {
 if(!cache_shadow())return hoisted;
 cache_counts[CACHE_PT28_READS].compares++;
 if(hoisted!=live && cache_mismatch(CACHE_PT28_READS))log_line("CACHE_STACK_MISMATCH,PT28_READS,-,%s,hoisted=%d,live=%d",what,hoisted,live);
 return live;
}
/* pt28_player's reads, in its order (incoming = 0, as the first original call), keeping the two team fields the
   incoming flag needs. */
static int pt28_player_fields(Rec player,Pt28Player *p,unsigned *previous,unsigned *current) {
 *previous=scalar(player,230,0x100);*current=scalar(player,261,0x100);
 p->table=q((U)player+0x10);p->id=row(player);p->pos=(int)scalar(player,3,0x108);
 p->ovr=(int)scalar(player,191,0x100);p->year=(int)scalar(player,242,0x108);
 p->hs=*previous==255&&*current==255;p->develop=!p->hs;
 p->redshirt=scalar(player,234,0x108)==2;p->arrival_delay=0;p->rostered=0;
 return p->pos>=0&&p->pos<22&&p->ovr>=0&&p->ovr<=99&&p->year>=0&&p->year<=3;
}
/* pt28_timing with the three season fields it reads passed in (same tests, same order of exits). */
static int pt28_timing_fields(const Pt28Player *candidate,int *advance,int *development,int *delay_hs,int stage,int week,int offseason) {
 if(stage<0||stage>2)return 0;
 *delay_hs=0;
 if(stage!=2){*advance=candidate->hs?1:0;*development=*advance;*delay_hs=!candidate->hs;return 1;}
 if(week<0||week>60)return 0;
 *advance=week<7;
 if(offseason<0||offseason>15)return 0;
 *development=offseason<8;return 1;
}
/* pt28_grade_impl with the in-call re-reads removed. franchise_in/season_in: the ready check's references (fast),
   released here where the original releases its own; 0 = read them here (shadow: ftable/frow are the ready check's
   franchise identity, compared). */
static int pt28_grade_once(U context,Rec team,Rec player,int *grade,Pt28Result *details,Rec franchise_in,Rec season_in,U ftable,uint32_t frow){
 CacheCount *k=&cache_counts[CACHE_PT28_READS];k->queries++;if(cache_fast())k->hits++;
 Rec franchise=franchise_in?franchise_in:context_ref(context,2);if(!franchise){release(season_in);return 0;}
 Rec season=franchise_in?season_in:reference(franchise,44),tuning=context_ref(context,5);
 if(ftable)pt28_reads_cmp(1,q((U)franchise+0x10)==ftable && row(franchise)==frow,"franchise");
 if(!season||!tuning){release(tuning);release(season);release(franchise);return 0;}
 int stage=(int)scalar(season,3,0x108),offseason=(int)scalar(season,8,0x100);
 int checkpoint=pt28_checkpoint_fields(franchise,season,stage,offseason);
 if(checkpoint<=0){
  if(checkpoint==0&&InterlockedIncrement(&pt28_wait_logs)<=8)
   log_line("PT28_WAIT,%u,%u,%u,NATIVE_DURING_TRAINING_ADVANCE",scalar(season,2,0x100),scalar(season,4,0x100),scalar(season,8,0x100));
  release(tuning);release(season);release(franchise);return checkpoint==0?-1:0;
 }
 int year=(int)scalar(season,2,0x100),week=(int)scalar(season,4,0x100),type=(int)scalar(season,5,0x108);
 stage=pt28_reads_cmp(stage,cache_shadow()?(int)scalar(season,3,0x108):0,"stage");
 offseason=pt28_reads_cmp(offseason,cache_shadow()?(int)scalar(season,8,0x100):0,"offseason");
 int advance=0,development=0,delay_hs=0;
 Pt28Player native_candidate;unsigned previous=0,current=0;
 int timing=pt28_player_fields(player,&native_candidate,&previous,&current);
 if(timing) {
  timing=pt28_timing_fields(&native_candidate,&advance,&development,&delay_hs,stage,week,offseason);
  if(cache_shadow()) {
   Pt28Player live;int a2=0,d2=0,h2=0;
   int t2=pt28_player(player,0,&live)&&pt28_timing(season,&live,&a2,&d2,&h2);
   pt28_reads_cmp(pt28_player_same(&native_candidate,&live),1,"first_candidate");
   timing=pt28_reads_cmp(timing,t2,"timing");
   if(t2){advance=pt28_reads_cmp(advance,a2,"advance");development=pt28_reads_cmp(development,d2,"development");
          delay_hs=pt28_reads_cmp(delay_hs,h2,"delay_hs");}
  }
 }
 int step=(int)scalar(tuning,39,0x100);release(tuning);release(season);
 unsigned tid=scalar(team,383,0x100);
 if(tid>=138||year<1900||year>2200||week<0||week>60||step<0||step>20||!timing){release(franchise);return 0;}
 int ok=pt28_ensure(franchise,year,week,type,stage,offseason);
 /* Persuasion can change a departure status without advancing the clock. */
 if(ok&&stage==2&&offseason==2)ok=pt28_read_departures(franchise);
 Pt28Player candidate;
 if(ok) {
  int incoming=pt28_published_owner(player)>=0;
  candidate=native_candidate;
  candidate.hs=previous==255&&(current==255||incoming);candidate.develop=!candidate.hs;
  if(cache_shadow()) {
   Pt28Player live;int lv=pt28_player(player,incoming,&live);
   pt28_reads_cmp(pt28_player_same(&candidate,&live),1,"candidate");
   if(lv)candidate=live;else ok=0;
  }
 }
 if(!ok){release(franchise);return 0;}
 Pt28Room room;memset(&room,0,sizeof(room));int group=pt28_group(candidate.pos);
 ok=pt28_jobs(team,group,&room)&&pt28_roster_cached(&room,team,group,stage==2)&&
    pt28_roster_array(&room,team,33,1,group,0)&&pt28_board(&room,franchise,team,group);
 release(franchise);if(!ok)return 0;
 if(delay_hs)for(int i=0;i<room.count;i++){
  Pt28Player *p=&room.players[i];
  if(p->rostered){p->hs=0;p->develop=1;}
  else if(p->hs)p->arrival_delay=1;
 }
 for(int i=0;i<room.count;i++)if(room.players[i].table==candidate.table&&room.players[i].id==candidate.id){candidate=room.players[i];break;}
 Pt28Result result;if(!pt28_compute(&room,&candidate,step,advance,development,&result))return 0;
 int recruiting=scalar(player,261,0x100)!=tid;
 if(pt28_log_scope.samples[recruiting][candidate.pos]<3){
  pt28_log_scope.samples[recruiting][candidate.pos]++;
  log_line("PT28_OPPORTUNITY,%d,%d,%d,%u,%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%.4f,%.4f,%.4f,%d",
   year,week,recruiting,tid,candidate.id,candidate.pos,candidate.ovr,candidate.year,candidate.redshirt,advance,development,candidate.hs,
   result.projected_ovr,result.rank,result.peer_count,result.starters,result.useful,result.following_available,
   result.immediate,result.following,result.score,result.grade);
 }
 *grade=result.grade;if(details)*details=result;return 1;
}
