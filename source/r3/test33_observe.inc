/* Test33: all-star HS execution evidence, emitted only after the existing
   observer validates the complete board budget. This helper performs no
   native reads, writes, selections or additional board scans.

   Unlike the elite market catalog, these rows include unoffered targets and
   all one- through five-star HS/JUCO recruits. A productive allocation on a
   rejected offer therefore remains visible for the lower-star cap audit.
   choice_points is the native displayed-school value when listed, otherwise
   the native target total; display_leader can be stale or conditional. Neither
   value is a computed firm leader. latest_gain is the signed native delta.

   current_funded describes this observation only. END action flags may have
   reset after valid earlier allocation; zero here does not mean never funded.
   No historical funding or firm/conditional state is inferred by this row. */
typedef struct {
 unsigned rid,status,productive,actions,target_points,assigned,total;
 int stars,pos,stage,role,choice_points,display_leader,latest_gain,plan_valid;
} Test33HsExecutionRow;
static Test33HsExecutionRow test33_execution_row(const Hs28Choice *p,unsigned rid,
 unsigned status,unsigned productive,unsigned actions,unsigned target_points,
 int gain,unsigned assigned,unsigned total,int plan_valid) {
 Test33HsExecutionRow r={0};
 r.rid=rid;r.status=status;r.productive=productive;r.actions=actions;
 r.target_points=target_points;r.assigned=assigned;r.total=total;
 r.latest_gain=gain;r.plan_valid=plan_valid;
 if(p){r.stars=p->stars+1;r.pos=p->pos;r.stage=p->stage;
  r.role=p->selected?p->choice:0;r.choice_points=p->points;
  r.display_leader=p->leader;}
 return r;
}
static void test33_execution_emit(const Test33HsExecutionRow *r,const char *site,
 unsigned year,unsigned type,unsigned week,unsigned tid,int board_valid) {
 if(!r || !site || r->stars<1 || r->stars>5 || r->status>4 ||
   (strcmp(site,"POST_FUNDING") && strcmp(site,"END")))return;
 int current_funded=board_valid && r->plan_valid && r->status>=2 &&
  r->status<=4 && r->productive>0 && r->assigned>=r->productive &&
  r->assigned<=r->total;
 log_line("TEST33_HS_EXECUTION,%s,%u,%u,%u,%u,%u,%d,%d,%d,%d,%u,%u,%u,%u,%d,%d,%d,%u,%u,%d,%d,%d",
  site,year,type,week,tid,r->rid,r->stars,r->pos,r->stage,r->role,
  r->status,r->productive,r->actions,r->target_points,r->choice_points,
  r->display_leader,r->latest_gain,r->assigned,r->total,r->plan_valid,
  board_valid,current_funded);
}
