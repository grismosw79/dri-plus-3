#include <assert.h>
#include <stdio.h>
#include "hs28_pairs.h"
static Hs28Choice c(int id,int score,int ovr,int stars){
 return (Hs28Choice){.table=1,.rid=(uint32_t)id,.pos=12,.native_score=score,.ovr=ovr,.stars=stars,.legal=1,.viable=1,.existing=1,.offered=1};
}
int main(void){
 Hs28Choice p[3]={c(1,330,67,2),c(2,300,65,2),c(3,460,72,3)};
 Hs28Prior old[2]={{.table=1,.rid=1,.pos=12,.slot=385,.choice=1,.order=1},{.table=1,.rid=2,.pos=12,.slot=385,.choice=2,.order=2}};
 int v[22]={0},o[22]={0};v[12]=1;
 hs28_plan_prior(p,3,v,o,0,50,old,2);assert(p[2].choice==1 && p[0].choice==2 && !p[1].selected);
 for(int test=0;test<5;test++){
  p[2]=c(3,460,72,3);p[0].stage=0;
  if(test==0)p[2].native_score=294;
  if(test==1)p[2].ovr=70;
  if(test==2)p[2].stars=1;
  if(test==3)p[2].viable=0;
  if(test==4)p[0].stage=4;
  hs28_plan_prior(p,3,v,o,0,50,old,2);assert(p[0].choice==1);
 }
 v[12]=0;hs28_plan_prior(p,3,v,o,0,50,old,2);assert(!p[0].selected && !p[2].selected);
 /* A strong retained primary from another opening cannot be borrowed to
    upgrade this opening. A genuinely new challenger can replace its weaker
    incumbent while the other pair keeps its identity. */
 v[12]=2;p[0]=c(1,330,67,2);p[1]=c(2,500,82,3);p[2]=c(3,460,72,3);
 old[1].choice=1;old[1].slot=386;
 hs28_plan_prior(p,3,v,o,0,50,old,2);
 assert(p[2].choice==1 && p[2].slot==385 && p[1].choice==1 && p[1].slot==386);
 assert(p[0].choice==2 && p[0].slot==385);
 puts("PASS: material quality-plus-fit upgrades; below-margin/ability-only/two-star/inaccessible/soft-commit protection and no new capacity");
}
