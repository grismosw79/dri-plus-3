#include <stdio.h>
#include <assert.h>
#include "../hs28_pairs.h"
int main(void){
 int vacancy[22]={0},optional[22]={0};vacancy[0]=1;
 Hs28Choice p[3]={{.table=1,.rid=1,.pos=0,.native_score=500,.ovr=80,.stars=4,.legal=1,.viable=1},
 {.table=1,.rid=2,.pos=0,.native_score=450,.ovr=80,.stars=3,.legal=1,.viable=1},
 {.table=1,.rid=3,.pos=0,.native_score=482,.ovr=80,.stars=3,.legal=1,.viable=1}};
 Hs28Prior old[2]={{.table=1,.rid=2,.pos=0,.slot=1,.choice=2,.order=1,.reserved=1}};
 assert(hs28_plan_prior(p,3,vacancy,optional,0,50,old,1)==2);
 assert(p[0].selected && p[1].selected && !p[2].selected);
 puts("Reserved backup without a retained/reserved primary: 450 backup retained despite unreserved 482; reservation binds.");
 old[1]=(Hs28Prior){.table=1,.rid=1,.pos=0,.slot=1,.choice=1,.order=2,.reserved=1};
 assert(hs28_plan_prior(p,3,vacancy,optional,0,50,old,2)==2);
 assert(p[0].selected && p[1].selected && !p[2].selected);
 puts("With the primary preserved in the same prior pair: reserved 450 backup is retained.");
 old[0].closed=1;
 assert(hs28_plan_prior(p,3,vacancy,optional,0,50,old,2)==2);
 assert(!p[1].selected && p[2].selected);
 old[0].closed=0;p[0].legal=0;
 assert(hs28_plan_prior(p,3,vacancy,optional,0,50,old,2)==2);
 assert(p[1].choice==1 && p[2].choice==2);
 vacancy[0]=0;assert(hs28_plan_prior(p,3,vacancy,optional,0,50,old,2)==0);
 return 0;
}
