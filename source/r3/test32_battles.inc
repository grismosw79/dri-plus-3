/* Test32: observational battle evidence, emitted only after the existing
   coverage observer has checked the complete native board budget. No native
   reads or writes occur here, and none of these fields influence selection.

   Each row is scoped to year/type/week and POST_FUNDING or END. The displayed
   leader can be stale or conditional; it is labelled explicitly. Offline
   analysis must compute the actual offered/firm leader from all school rows,
   then compare END points across weeks to measure progress. Choice points use
   the displayed school entry when present, otherwise the native target total.
   Neither a planned
   primary nor one near-leader observation proves a sustained contender. */
typedef struct {
 unsigned rid,status,productive,actions,target_points,assigned,total;
 int stars,pos,stage,role,state,choice_points,display_leader,display_rank;
 int threshold,latest_gain,funded_in_scope;
} Test32BattleRow;
static Test32BattleRow test32_battle_row(const Hs28Choice *p,unsigned rid,
 unsigned status,unsigned productive,unsigned actions,unsigned target_points,
 int state,int gain,unsigned assigned,unsigned total,int funded_in_scope) {
 Test32BattleRow r={0};
 r.rid=rid;r.status=status;r.productive=productive;r.actions=actions;
 r.target_points=target_points;r.assigned=assigned;r.total=total;
 r.state=state;r.latest_gain=gain;r.funded_in_scope=funded_in_scope;
 if(p){r.stars=p->stars+1;r.pos=p->pos;r.stage=p->stage;
  r.role=p->selected?p->choice:0;r.choice_points=p->points;
  r.display_leader=p->leader;r.display_rank=p->rank;r.threshold=p->commit_score;}
 return r;
}
static void test32_battle_emit(const Test32BattleRow *r,const char *site,
 unsigned year,unsigned type,unsigned week,unsigned tid,int board_valid) {
 if(!r || r->stars<4 || r->stars>5 || r->status<2 || r->status>4 ||
   (strcmp(site,"POST_FUNDING") && strcmp(site,"END")))return;
 int current_funded=board_valid && r->state>0 && r->productive>0 &&
  r->assigned>=r->productive && r->assigned<=r->total;
 log_line("TEST32_ELITE_BATTLE,%s,%u,%u,%u,%u,%u,%d,%d,%d,%d,%d,%u,%u,%u,%u,%d,%d,%d,%d,%d,%d,%u,%u,%d,%d,%d",
  site,year,type,week,tid,r->rid,r->stars,r->pos,r->stage,r->role,r->state,
  r->status,r->productive,r->actions,r->target_points,r->choice_points,
  r->display_leader,r->display_leader-r->choice_points,r->display_rank,
  r->threshold,r->latest_gain,r->assigned,r->total,board_valid,
  current_funded,r->funded_in_scope);
}
