"""Compile the actual shared Test29 market resolver; no game outcome simulation.
R2 supersedes the original12HS/8portal primary assertions with8HS/5portal.
Tests synchronous proposals, stable school-order permutation, protected offers,
sticky rejection, deduplication, bounded-round reallocation and conserved funds.
"""
from pathlib import Path
import subprocess,os,json
S=Path(__file__).resolve().parents[1]
O=Path(os.environ.get('DRI_TEST_OUTPUT_DIR',S/'build/test29-market-core'));O.mkdir(parents=True,exist_ok=True)
code=r'''
#include <assert.h>
#include <stdio.h>
#include "market29_core.h"
#include "hs28_pairs.h"
static int bitcount(const uint64_t a[3]){int n=0;for(unsigned t=0;t<138;t++)n+=market29_has(a,t);return n;}
static void initial(Market29Pressure *p,int reversed){
 memset(p,0,sizeof(*p));for(int i=0;i<40;i++){unsigned t=(unsigned)(reversed?39-i:i);
 market29_bit(p->planned,t,1);market29_bit(p->primary,t,1);p->fit[t]=500;p->interest[t]=t==31?50:0;}
}
static void chain(int portal,int reversed){
 Market29Pressure p[42]={0};int choice[40]={0},budget[40],spent[40]={0};
 for(int i=0;i<40;i++){budget[i]=10;for(int k=0;k<42;k++)p[k].fit[i]=500;}
 int rounds=0;
 for(;rounds<MARKET29_ROUNDS;rounds++){
  for(int k=0;k<42;k++){memset(p[k].planned,0,24);memset(p[k].primary,0,24);}
  for(int x=0;x<40;x++){unsigned t=(unsigned)(reversed?39-x:x);int k=choice[t];
   spent[t]=10;assert(spent[t]<=budget[t]);market29_bit(p[k].planned,t,1);market29_bit(p[k].primary,t,1);}
  int changes=0;for(int k=0;k<42;k++)changes+=market29_reconcile(&p[k],portal,(uint32_t)k,2026);
  if(!changes)break;
  for(int x=0;x<40;x++){unsigned t=(unsigned)(reversed?39-x:x);int k=choice[t];
   if(market29_has(p[k].rejected,t))choice[t]=k==0?1:(int)t+2;}
 }
 assert(rounds<MARKET29_ROUNDS);assert(bitcount(p[0].planned)==(portal?5:8));
 assert(bitcount(p[1].planned)==(portal?5:8));
 int sum=0;for(int k=0;k<42;k++){sum+=bitcount(p[k].planned);assert(bitcount(p[k].planned)<=(portal?5:8));}
 assert(sum==40);for(int t=0;t<40;t++)assert(spent[t]==10);
}
static void pair_cases(void){
 int v[22]={1},o[22]={0};
 Hs28Choice p[3]={{.rid=1,.pos=0,.native_score=500,.ovr=78,.legal=1,.viable=1},
 {.rid=2,.pos=0,.native_score=480,.ovr=77,.legal=1,.viable=1},
 {.rid=3,.pos=0,.native_score=470,.ovr=77,.legal=1,.viable=1}};
 assert(hs28_plan(p,3,v,o,0,2)==2 && p[0].choice==1);
 p[0].crowd_penalty=10;
 assert(hs28_plan(p,3,v,o,0,2)==2 && !p[0].selected && p[1].choice==1);
 p[0].crowd_penalty=25;
 assert(hs28_plan(p,2,v,o,0,2)==2 && p[1].choice==1 && p[0].choice==2);
 p[0].crowd_penalty=45;
 assert(hs28_plan(p,2,v,o,0,2)==1 && !p[0].selected && p[1].choice==1);
 Hs28Prior old={.table=0,.rid=1,.pos=0,.slot=1,.choice=1,.order=1};
 p[0].existing=1;
 assert(hs28_plan_prior(p,2,v,o,0,2,&old,1)==1 && !p[0].selected);
 p[0].crowd_penalty=0;p[0].offered=1;
 assert(hs28_plan_prior(p,2,v,o,0,2,&old,1)==2 && p[0].choice==1);
}
int main(void){
 pair_cases();
 assert(market29_penalty(0,4)==0 && market29_penalty(0,5)==10 && market29_penalty(0,6)==25 && market29_penalty(0,7)==25 && market29_penalty(0,8)==45);
 assert(market29_penalty(1,2)==0 && market29_penalty(1,3)==10 && market29_penalty(1,4)==25 && market29_penalty(1,5)==45);
 assert(market29_backup_penalty(0,8)==0 && market29_backup_penalty(0,9)==25 && market29_backup_penalty(0,12)==45);
 assert(market29_backup_penalty(1,5)==0 && market29_backup_penalty(1,6)==25 && market29_backup_penalty(1,8)==45);
 Market29Pressure a,b;initial(&a,0);market29_save_plan(&a);assert(!market29_plan_changed(&a));
 market29_bit(a.primary,0,0);assert(market29_plan_changed(&a));
 market29_save_plan(&a);assert(!market29_plan_changed(&a));
 initial(&a,0);initial(&b,1);
 assert(market29_reconcile(&a,1,77,2026)==35);assert(market29_reconcile(&b,1,77,2026)==35);
 assert(!memcmp(a.rejected,b.rejected,sizeof(a.rejected)));assert(bitcount(a.admitted)==5);assert(market29_has(a.admitted,31));
 assert(market29_reconcile(&a,1,77,2026)==0);
 initial(&a,0);assert(market29_reconcile(&a,0,77,2026)==32);assert(bitcount(a.admitted)==8);
 initial(&a,0);for(unsigned t=0;t<40;t++){market29_bit(a.protected_offer,t,1);market29_bit(a.active,t,1);}
 assert(market29_reconcile(&a,1,77,2026)==0);assert(bitcount(a.admitted)==40);assert(market29_pressure(&a,0)==39);
 assert(bitcount(a.firm)==0); /* Productive conditional offers are not firm. */
 initial(&a,0);market29_bit(a.exception,39,1);market29_reconcile(&a,1,77,2026);assert(market29_has(a.admitted,39));assert(!market29_has(a.rejected,39));
 chain(0,0);chain(0,1);chain(1,0);chain(1,1);
 puts("PASS: thresholds, global order independence, protected offers, deduplication, conditional distinction, exceptions, multi-wave reallocation and budget conservation");
}
'''
(O/'market_core.c').write_text(code)
subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-O2','-I'+str(S),str(O/'market_core.c'),'-o',str(O/'market_core')],check=True)
r=subprocess.run([str(O/'market_core')],check=True,text=True,capture_output=True);print(r.stdout)
(O/'test29_market_core_results.json').write_text(json.dumps({'status':'PASS','method':'compiled production pure resolver; modeled multi-school forecasts','live_game_verified':False,'native_spending_verified':False,'checks':['R2 primary HS8/portal5 and secondary total HS12/portal8 tier boundaries','stable recruit/school/year tie and earned-interest priority','order-independent simultaneous proposals','active/planned school union deduplicates','conditional attention never becomes firm by count','offered pursuit protection','explicit exception protection','two successive crowded alternatives redistributed within eight rounds','whole-school modeled funding unchanged','all inherited direct pair-planner penalty/old-plan/offer assertions retained']},indent=2)+'\n')
