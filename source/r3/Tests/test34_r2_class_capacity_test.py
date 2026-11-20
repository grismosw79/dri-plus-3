"""Full ordinary-plan regression and bounded class capacity; numeric policy only."""
from pathlib import Path
import os,subprocess,json
S=Path(__file__).resolve().parents[1];O=Path(os.environ['DRI_TEST_OUTPUT_DIR']);O.mkdir(parents=True,exist_ok=True)
c=r'''
#include <assert.h>
#include <string.h>
#include "class34_policy.h"
static const int ideals[22]={4,5,4,6,5,3,3,3,3,3,4,4,6,2,4,4,8,4,4,2,2,0};
static Test29ClassInput base(unsigned tid,int pos,int repair[22]) {
 Test29ClassInput in={0};int annual[22];
 in.annual_valid=1;in.annual_target=class31_annual(ideals,tid,2026,annual);in.general=2;
 for(int p=0;p<22;p++) {
  in.ideal[p]=ideals[p];in.returning[p]=ideals[p];
  in.vacancy[p]=annual[p];in.candidates[p]=annual[p];
 }
 in.secured[pos]=annual[pos];in.returning[pos]=ideals[pos]-annual[pos]-1;
 in.vacancy[pos]=0;in.optional[pos]=1;in.candidates[pos]=1;in.optional_value[pos]=300;
 memset(repair,0,22*sizeof(int));repair[pos]=1;return in;
}
int main(void) {
 Test29ClassPlan old,out;int repair[22],annual[22],hs[22]={0};
 /* All 138 school IDs and all 18 eligible positions: the complete normal
    map fits alongside the repair. No Navy/HB identity is in the policy. */
 for(unsigned tid=0;tid<138;tid++)for(int pos=1;pos<=18;pos++) {
  Test29ClassInput in=base(tid,pos,repair),copy=in;
  test29_class_plan(&in,&old);assert(!old.optional[pos]);
  assert(class34r2_plan(&in,repair,0,&out)==1);
  assert(out.target==in.annual_target+1 && out.optional[pos]==1);
  assert(out.planned==out.remaining && !out.shortfall && !out.protected_over_target);
  for(int p=0;p<22;p++)assert(out.vacancy[p]==in.vacancy[p]);
  assert(!memcmp(&in,&copy,sizeof(in)));
 }
 Test29ClassInput in=base(55,1,repair);
 assert(in.annual_target==18);class31_annual(ideals,55,2026,annual);
 for(int p=0;p<22;p++)assert(in.vacancy[p]+in.secured[p]==annual[p]);
 for(int p=4;p<=13;p++)if(p==4 || p==9 || p==13){in.secured[p]=1;in.vacancy[p]--;in.candidates[p]--;}
 assert(class34r2_plan(&in,repair,0,&out)==1);
 assert(out.secured==4 && out.target==19 && out.planned==15 && out.optional[1]);
 in.protected_vacancy[3]=1;assert(class34r2_plan(&in,repair,0,&out)==1 && out.vacancy[3]==1);
 in.general=0;assert(class34r2_plan(&in,repair,0,&out)==0 && out.target==18 && !out.optional[1]);
 in.general=2;repair[1]=0;assert(class34r2_plan(&in,repair,0,&out)==0 && out.target==18);
 repair[1]=1;in.candidates[1]=0;assert(class34r2_plan(&in,repair,0,&out)==0);
 in.candidates[1]=1;
 /* A second different positional repair may use the other general place. */
 int p2=6;in.secured[p2]=annual[p2];in.vacancy[p2]=0;in.optional[p2]=1;
 in.candidates[p2]=1;repair[p2]=1;in.optional_value[p2]=400;
 assert(class34r2_plan(&in,repair,0,&out)==2 && out.target==20 && out.optional[1] && out.optional[p2]);
 in.general=1;assert(class34r2_plan(&in,repair,0,&out)==1 && out.optional[p2] && !out.optional[1]);
 in.protected_optional[1]=1;
 assert(class34r2_plan(&in,repair,0,&out)==1 && out.optional[1] && !out.optional[p2]);
 /* Acquired HS extras retain their capacity after cold reconstruction, but
    do not replenish allowance or accumulate a new expansion each rebuild. */
 in=base(55,1,repair);in.secured[1]++;in.optional[1]=0;in.candidates[1]=0;repair[1]=0;in.general=1;
 hs[1]=annual[1]+1;assert(class34r2_secured_extra(1,annual,hs)==1);
 for(int n=0;n<4;n++) {
  assert(class34r2_plan(&in,repair,1,&out)==0 && out.target==19);
  for(int p=0;p<22;p++)assert(out.vacancy[p]==in.vacancy[p]);
 }
 in.optional[6]=1;in.candidates[6]=1;in.secured[6]=annual[6];in.vacancy[6]=0;repair[6]=1;
 assert(class34r2_plan(&in,repair,1,&out)==1 && out.target==20);
 in.general=0;assert(class34r2_plan(&in,repair,1,&out)==0 && out.target==19);
 /* Existing optional quality pursuit keeps the shared place; no double use. */
 in=base(55,1,repair);in.optional[0]=in.protected_optional[0]=1;in.candidates[0]++;
 assert(class34r2_plan(&in,repair,0,&out)==1 && out.optional[0] && out.optional[1] && out.general==2);
 in.general=1;assert(class34r2_plan(&in,repair,0,&out)==0 && out.optional[0] && !out.optional[1]);
 in=base(55,1,repair);in.annual_target=24;
 assert(class34r2_plan(&in,repair,1,&out)==0 && out.target==25);
 assert(class34r2_plan(&in,repair,0,&out)==1 && out.target==25);
 in.annual_target=25;assert(class34r2_plan(&in,repair,0,&out)==0 && out.target==25);
 for(int base=15;base<=25;base++)for(int used=0;used<=2;used++)for(int pending=0;pending<=5;pending++) {
  int capacity=class34r2_capacity(1,base,used,pending);
  assert(capacity>=base && capacity<=25 && capacity<=base+2);
 }
 for(int p=0;p<22;p++)if(p==0 || p>=19) {
  in=base(55,1,repair);memset(repair,0,sizeof(repair));repair[p]=1;in.optional[p]=in.candidates[p]=1;
  assert(class34r2_plan(&in,repair,0,&out)==0 && out.target==18);
 }
 assert(class34r2_capacity(0,18,2,2)==18);
 assert(class34r2_secured_extra(0,annual,hs)==0);
 return 0;
}
'''
(O/'class34r2.c').write_text(c)
subprocess.run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror','-I'+str(S),str(O/'class34r2.c'),'-o',str(O/'class34r2')],check=True)
subprocess.run([str(O/'class34r2')],check=True)
r={'passed':True,'full_plan_school_position_cases':138*18,'checks':['Old planner reproduces saturation failure','All ordinary places preserved with repair','Observed Navy four commitments plus14 normal primaries plus1 extra','Two shared places maximum','No candidate/no permission controls','Priority and incumbent continuity','Cold ownership reconstruction','25 maximum authorized capacity','QB/K/P/LS and invalid-map exclusions'],'live_game_verified':False}
(O/'test34_r2_class_capacity_results.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r))
