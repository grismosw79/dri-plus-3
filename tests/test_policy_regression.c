#include <assert.h>
#include <stdio.h>
#ifndef POLICY_HEADER
#define POLICY_HEADER "../source/roster/rm_policy.h"
#endif
#include POLICY_HEADER
static int quoted,allowed=1;
static int gate(void*ctx,int p,int t,int*q){(void)ctx;(void)p;(void)t;*q=quoted;return allowed;}
static RMState*state(void){RMState*s=calloc(1,sizeof(*s));assert(s);for(int t=0;t<RM_TEAMS;t++){s->t[t].head=-1;s->t[t].budget=100;s->t[t].prestige=5;s->t[t].user=t>2;for(int p=0;p<RM_POS;p++)s->t[t].config[p]=(RMConfig){1,2,3,1};}s->gate=gate;s->quotes=malloc(RM_PLAYERS*RM_TEAMS*sizeof(int));for(int i=0;i<RM_PLAYERS*RM_TEAMS;i++)s->quotes[i]=INT_MIN;quoted=0;allowed=1;return s;}
static void destroy(RMState*s){free(s->quotes);free(s);}
static int player(RMState*s,int t,int pos,int ovr,int year){int p=s->n++;s->p[p]=(RMPlayer){.id=1000+p,.original=t,.team=-1,.pos=pos,.ovr=ovr,.dev=1,.year=year,.next=-1};for(int k=0;k<3;k++)s->kicking[p][k]=ovr;if(t>=0)rm_link(s,p,t);return p;}
static void test_distinct_and_quantity_roles(void){RMState*s=state();int a=player(s,0,5,88,3),b=player(s,0,5,85,3);player(s,0,9,75,3);int jobs[RM_ROOM],n=rm_starters(s,0,jobs);assert(n==2);assert(jobs[0]!=jobs[1]);assert((jobs[0]==a||jobs[1]==a)&&(jobs[0]==b||jobs[1]==b));rm_protect(s);assert(s->p[a].held&&s->p[b].held);destroy(s);}
static void test_user_academy_and_hs_boundary(void){RMState*s=state();int p=player(s,-1,19,65,0);s->p[p].pool=1;s->p[p].hs=2;s->t[1].academy=1;int r,q;assert(!rm_edge(s,p,1,RM_ACADEMY_SPECIALISTS,&r,&q));s->p[p].hs=1;assert(rm_edge(s,p,1,RM_ACADEMY_SPECIALISTS,&r,&q));s->t[1].user=1;assert(!rm_edge(s,p,1,RM_ACADEMY_SPECIALISTS,&r,&q));s->t[1].user=0;assert(!rm_edge(s,p,1,RM_FILL_SPECIALISTS,&r,&q));s->p[p].original=0;s->p[p].pool=0;assert(!rm_edge(s,p,1,RM_CAPACITY,&r,&q));destroy(s);}
static void test_role_promise_conflict(void){RMState*s=state();int p=player(s,1,0,80,2);s->p[p].original=0;s->a[s->na++]=(RMAction){p,0,1,RM_CAPACITY,RM_STARTER,0};int q=player(s,2,0,85,2),r,price;s->t[0].initial=s->t[1].initial=s->t[2].initial=85;assert(!rm_edge(s,q,1,RM_CAPACITY,&r,&price));destroy(s);}
static void test_source_coverage_and_arrivals(void){RMState*s=state();int a=player(s,0,16,90,3),b=player(s,0,16,70,3),c=player(s,0,16,65,3);s->p[c].arrival=1;rm_protect(s);assert(s->p[a].held&&s->p[c].held&&!s->p[b].held);assert(rm_source_safe(s,b));rm_unlink(s,c,0);assert(!rm_source_safe(s,b));int arrival=player(s,0,16,99,0);s->p[arrival].original=2;assert(!rm_source_safe(s,b));destroy(s);}
static void test_no_home_kept(void){RMState*s=state();for(int t=1;t<RM_TEAMS;t++)s->t[t].user=1;for(int i=0;i<91;i++)player(s,0,16,30+i%60,3);s->t[0].initial=91;rm_protect(s);rm_source_phase(s,RM_CAPACITY);assert(s->na==0&&s->t[0].count==91);destroy(s);}
static void test_budget_reservations(void){RMState*s=state();int a=player(s,-1,19,65,0),b=player(s,-1,20,66,0);s->p[a].pool=s->p[b].pool=1;s->p[a].hs=s->p[b].hs=1;for(int t=1;t<RM_TEAMS;t++)s->t[t].user=1;s->t[0].budget=15;quoted=15;int c[]={a,b};assert(rm_match(s,c,2,RM_FLOOR)==1);assert(s->t[0].reserved==15&&s->na==1);destroy(s);}
static void test_native_reject_and_negative_quote(void){for(int mode=0;mode<2;mode++){RMState*s=state();int p=player(s,-1,19,70,0);s->p[p].pool=s->p[p].hs=1;int r,q;allowed=mode;quoted=mode?-1:0;assert(!rm_edge(s,p,0,RM_FLOOR,&r,&q));assert(s->t[0].count==0&&s->p[p].team==-1);destroy(s);}}
static void test_source_queue_independent(void){RMState*s=state();for(int i=0;i<8;i++)player(s,0,16,60+i,3);rm_protect(s);int copy[20],n=s->nq;memcpy(copy,s->queue,n*sizeof(int));s->t[1].prestige=10;for(int i=0;i<30;i++)player(s,1,16,99,0);rm_protect(s);int j=0;for(int k=0;k<s->nq;k++)if(s->p[s->queue[k]].original==0)assert(s->queue[k]==copy[j++]);assert(j==n);destroy(s);}
static void test_tied_reserve_does_not_claim_rotation(void){RMState*s=state();int p=player(s,1,1,80,3);s->p[p].original=-1;player(s,1,1,90,3);player(s,1,1,80,3);assert(rm_role(s,p,1)==RM_NO_ROLE);destroy(s);}
static int repair_q,repair_x,repair_block;
static int repair_gate(void*ctx,int p,int t,int*q){(void)ctx;*q=t==3?15:0;return(p==repair_q&&t==2)||(p==repair_x&&t==3&&!repair_block);}
static void test_capacity_repair_preserves_sources_and_checks_destination(void){
 for(int mode=0;mode<3;mode++){
  RMState*s=state();s->t[3].user=0;s->gate=repair_gate;repair_block=mode==1;
  player(s,0,0,90,3);player(s,0,0,80,3);repair_q=player(s,0,0,70,3);
  player(s,1,0,90,3);player(s,1,0,80,3);repair_x=player(s,1,0,75,3);
  for(int i=0;i<88;i++){player(s,0,3,60,3);player(s,1,3,60,3);}
  player(s,2,0,60,3);player(s,3,0,65,3);
  for(int t=0;t<RM_TEAMS;t++)s->t[t].initial=s->t[t].count;
  rm_unlink(s,repair_x,1);rm_link(s,repair_x,2);s->p[repair_x].moved=1;
  s->a[s->na++]=(RMAction){repair_x,1,2,RM_CAPACITY,RM_STARTER,0};
  s->queue[s->nq++]=repair_q;s->t[3].budget=mode==2?14:15;
  RMState*before=malloc(sizeof(*before));memcpy(before,s,sizeof(*s));
  assert(rm_repair_capacity(s));
  if(mode){assert(!memcmp(before,s,sizeof(*s)));assert(s->t[0].count==91);}
  else{assert(s->t[0].count==90&&s->t[1].count==90);assert(s->p[repair_q].team==2&&s->p[repair_x].team==3);assert(s->na==2&&s->t[3].reserved==15);assert(s->a[0].role==RM_STARTER&&s->a[1].role==RM_STARTER);assert(rm_promises(s,2)&&rm_promises(s,3));}
  free(before);destroy(s);
 }
}
static void test_source_opportunity_is_stable_while_coverage_remains_current(void){
 RMState*s=state();int a=player(s,0,1,90,3),b=player(s,0,1,80,3),p=player(s,0,1,70,3);player(s,0,1,60,3);
 rm_protect(s);assert(rm_source_rank(s,p,0)==3);
 rm_unlink(s,b,0);rm_link(s,b,1);assert(rm_source_rank(s,p,0)==3&&rm_rank(s,p,0,0,1)==2);
 rm_unlink(s,a,0);rm_link(s,a,1);assert(!rm_source_safe(s,p));destroy(s);
}
static void test_specialist_roles_and_cross_chart_no_false_vacancy(void){
 for(int incumbent_year=2;incumbent_year<=3;incumbent_year++){
  RMState*s=state();int old=player(s,1,19,83,incumbent_year);s->p[old].chart=UINT64_C(1)<<20;
  player(s,1,20,78,2);int p=player(s,1,19,80,3);s->p[p].original=0;
  assert(rm_role(s,p,1)==RM_NO_ROLE);s->p[p].year=0;s->source_young_rank[p]=3;
  assert(rm_role(s,p,1)==(incumbent_year==3?RM_SUCCESSION:RM_NO_ROLE));destroy(s);
 }
}
static void test_equal_qb_is_not_a_starter_promise(void){RMState*s=state();int p=player(s,1,0,72,0);s->p[p].original=0;s->p[p].dev=3;player(s,1,0,72,2);assert(rm_role(s,p,1)==RM_NO_ROLE);destroy(s);}
static void test_actual_second_string_does_not_claim_second_string_gain(void){RMState*s=state();int p=player(s,1,18,70,2);s->p[p].original=0;s->p[p].chart_depth=2;s->source_rank[p]=6;player(s,1,18,80,3);player(s,1,18,60,1);assert(rm_role(s,p,1)==RM_NO_ROLE);destroy(s);}
static void test_safety_chart_jobs_are_separate_from_coverage(void){RMState*s=state();int p=player(s,1,17,71,2);s->p[p].original=0;s->source_rank[p]=6;player(s,1,17,79,3);player(s,1,17,77,2);player(s,1,17,72,2);player(s,1,18,69,2);assert(rm_role(s,p,1)==RM_NO_ROLE);assert(rm_unit(17)==rm_unit(18)&&rm_job_unit(17)!=rm_job_unit(18));destroy(s);}
static void test_rotation_needs_top_backup_and_better_source_role(void){RMState*s=state();int p=player(s,1,4,73,2);s->p[p].original=0;s->source_rank[p]=5;player(s,1,4,80,3);int b=player(s,1,4,75,2);player(s,1,4,72,3);s->t[1].config[4]=(RMConfig){1,3,6,2};assert(rm_role(s,p,1)==RM_NO_ROLE);rm_unlink(s,b,1);assert(rm_role(s,p,1)==RM_ROTATION);destroy(s);}
static void test_specialist_ovr_alone_does_not_prove_a_starting_job(void){RMState*s=state();int p=player(s,1,20,75,0);s->p[p].original=0;int b=player(s,1,20,74,2);s->kicking[p][0]=89;s->kicking[p][1]=74;s->kicking[p][2]=64;s->kicking[b][0]=85;s->kicking[b][1]=72;s->kicking[b][2]=69;assert(rm_role(s,p,1)==RM_NO_ROLE);s->kicking[p][2]=70;assert(rm_role(s,p,1)==RM_STARTER);destroy(s);}
int main(void){test_distinct_and_quantity_roles();test_user_academy_and_hs_boundary();test_role_promise_conflict();test_source_coverage_and_arrivals();test_no_home_kept();test_budget_reservations();test_native_reject_and_negative_quote();test_source_queue_independent();test_tied_reserve_does_not_claim_rotation();test_capacity_repair_preserves_sources_and_checks_destination();test_source_opportunity_is_stable_while_coverage_remains_current();test_specialist_roles_and_cross_chart_no_false_vacancy();test_equal_qb_is_not_a_starter_promise();test_actual_second_string_does_not_claim_second_string_gain();test_safety_chart_jobs_are_separate_from_coverage();test_rotation_needs_top_backup_and_better_source_role();test_specialist_ovr_alone_does_not_prove_a_starting_job();puts("17 policy suites passed");return 0;}
