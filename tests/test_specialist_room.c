#define main prior_suite
#include "test_policy_regression.c"
#undef main

static int unsigned_target, accept, asked;
static int room_gate(void *ctx,int i,int t,int *price) {
 RMState *s=ctx;asked++;*price=5;
 return (accept && i==unsigned_target && t==0) || (s->p[i].original==0 && t==1 && s->p[i].pos==3);
}
static void full_team(int allow) {
 RMState *s=state();s->ctx=s;s->gate=room_gate;accept=allow;
 for(int t=0;t<RM_TEAMS;t++){
  s->t[t].user=t>1;
  memset(s->t[t].config,0,sizeof(s->t[t].config));
  s->t[t].config[3]=(RMConfig){1,3,4,1};
 }
 s->t[0].config[19]=(RMConfig){1,3,2,1};s->t[0].config[20]=(RMConfig){1,3,2,1};
 int k=player(s,0,19,73,3);s->p[k].chart=UINT64_C(1)<<20;
 for(int j=0;j<89;j++)player(s,0,3,40+j%30,3);
 for(int j=0;j<85;j++)player(s,1,16,30,3);
 s->t[0].initial=90;s->t[1].initial=85;
 unsigned_target=player(s,-1,20,66,0);s->p[unsigned_target].pool=s->p[unsigned_target].hs=1;
 rm_protect(s);rm_repair_specialist_rooms(s);
 assert(!s->fault&&s->t[0].count==90&&s->p[k].team==0);
 if(allow){assert(s->na==2&&s->a[0].phase==RM_SPECIALIST_REHOME&&s->a[1].phase==RM_ROOM_REPAIR);
  assert(s->t[1].count==86&&s->a[1].player==unsigned_target&&rm_count(s,0,20,0)==1&&s->t[0].reserved==5);
 }else{assert(s->na==0&&s->t[1].count==85&&s->t[0].reserved==0&&s->t[1].reserved==0);}
 free(s->quotes);s->quotes=NULL;assert(rm_rewind_pending(s)&&s->na==0&&s->t[0].count==90&&s->t[1].count==85&&s->p[unsigned_target].team<0);
 destroy(s);
}
static void donors_and_frozen_edges(void) {
 RMState *s=state();int p=player(s,0,20,65,3),backup=player(s,0,20,75,3);
 int k1=player(s,0,19,75,3),k2=player(s,0,19,65,3);(void)k1;(void)k2;
 for(int j=0;j<82;j++)player(s,0,3,45,3);
 assert(rm_specialist_donor(s,p));s->p[backup].injured=1;assert(!rm_specialist_donor(s,p));s->p[backup].injured=0;
 s->t[0].count=85;assert(!rm_specialist_donor(s,p));s->t[0].count=86;s->t[0].academy=1;assert(!rm_specialist_donor(s,p));
 s->t[0].academy=0;s->t[0].user=1;assert(!rm_specialist_donor(s,p));s->t[0].user=0;
 assert(rm_note_stale(s,p,1)==1);assert(rm_quote(s,p,1)>=0);assert(rm_note_stale(s,p,1)==2);assert(rm_quote(s,p,1)<0);
 assert(rm_quote(s,p,2)>=0&&rm_quote(s,backup,1)>=0);
 free(s->stale_edges);s->stale_edges=NULL;destroy(s);
}
int main(void){full_team(1);full_team(0);donors_and_frozen_edges();puts("Passed full roster two-move placement, failed chain rollback, donor floor and healthy coverage, academy/user scope and repeated-quote recovery.");return 0;}
