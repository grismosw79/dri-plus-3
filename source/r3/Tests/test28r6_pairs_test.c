#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "hs28_pairs.h"
int main(void){
 Hs28Choice p[128];int v[22]={0},o[22]={0};
 memset(p,0,sizeof(p));v[0]=1;
 p[0]=(Hs28Choice){.rid=1,.pos=0,.ovr=80,.stars=3,.native_score=100,.legal=1,.viable=1};
 p[1]=(Hs28Choice){.rid=2,.pos=0,.ovr=75,.stars=2,.native_score=100,.legal=1,.viable=1};
 p[2]=(Hs28Choice){.rid=3,.pos=0,.ovr=70,.native_score=100,.legal=1,.viable=1};
 assert(hs28_plan(p,3,v,o,0,50)==2);
 assert(p[0].choice==1 && p[1].choice==2 && p[0].slot==p[1].slot && !p[2].selected);
 assert(p[0].weight==40 && p[1].weight==20);
 p[1].urgent=1;hs28_plan(p,3,v,o,0,50);assert(p[0].weight==30 && p[1].weight==30);
 p[0].viable=0;hs28_plan(p,3,v,o,0,50);assert(p[1].choice==1 && p[0].choice==0 && p[2].choice==2);
 v[0]=0;assert(hs28_plan(p,3,v,o,0,50)==0);
 /* Capacity protects primary coverage across positions before alternatives. */
 memset(v,0,sizeof(v));memset(p,0,sizeof(p));
 for(int j=0;j<100;j++)p[j]=(Hs28Choice){.rid=(uint32_t)j,.pos=j%10,.ovr=80-j/10,.native_score=100,.legal=1,.viable=1};
 for(int j=0;j<10;j++)v[j]=3;
 assert(hs28_plan(p,100,v,o,0,50)==50);
 int primary=0,backup=0;
 for(int j=0;j<100;j++)if(p[j].selected){primary+=p[j].choice==1;backup+=p[j].choice==2;assert(p[j].order<=50);}
 assert(primary==30 && backup==20);
 assert(hs28_plan(p,100,v,o,0,7)==7);
 for(int j=0;j<100;j++)if(p[j].selected)assert(p[j].choice==1 && p[j].pos<7);
 /* General exceptions are shared, not one extra for every full position. */
 memset(v,0,sizeof(v));for(int j=0;j<10;j++)o[j]=1;
 assert(hs28_plan(p,100,v,o,2,50)==4);
 primary=0;for(int j=0;j<100;j++)primary+=p[j].selected && p[j].choice==1;assert(primary==2);
 /* Final preference is globally ordered, independent of school continuity
    and influence; waiting on a primary cannot create a preference cycle. */
 p[0]=(Hs28Choice){.rid=10,.pos=0,.ovr=80,.native_score=100,.legal=1,.viable=1,.final=1};
 p[1]=(Hs28Choice){.rid=11,.pos=0,.ovr=79,.native_score=100,.legal=1,.viable=1,.existing=1,.offered=1,.points=999,.final=1};
 assert(hs28_better(&p[0],&p[1]));p[1].ovr=80;assert(hs28_better(&p[0],&p[1]));
 puts("PASS: pairs, cut attention, promotion, capacity coverage, exception scarcity, acyclic deadline preference");
 return 0;
}
