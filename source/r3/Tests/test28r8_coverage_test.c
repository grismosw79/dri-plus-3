#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "hs28_coverage.h"
static Hs28Choice choice(int rid,int score,int stars,int selected,int role) {
 return (Hs28Choice){.rid=(unsigned)rid,.pos=3,.ovr=78,.stars=stars,.native_score=score,
 .legal=1,.viable=1,.selected=selected,.slot=97,.choice=role,.weight=role==1?40:20,.order=role};
}
int main(void) {
 Hs28Choice p[5];int counts[5]={3,2,0,1,0};
 p[0]=choice(1,400,4,1,1);p[1]=choice(2,300,3,1,2);p[2]=choice(3,300,3,0,0);
 p[3]=choice(4,350,3,0,0);p[4]=choice(5,400,3,0,0);p[4].pos=7;
 assert(hs28_coverage_swap(p,5,counts)==1);
 assert(p[0].selected && p[0].choice==1 && p[2].selected && !p[1].selected && !p[3].selected && !p[4].selected);
 assert(p[2].slot==97 && p[2].weight==20 && p[0].weight==40);
 /* One additional race is not enough: releasing this backup would strand it. */
 p[1]=choice(2,300,3,1,2);p[2]=choice(3,300,3,0,0);counts[1]=1;
 assert(hs28_coverage_swap(p,3,counts)==0);
 counts[1]=2;p[2].native_score=299;assert(hs28_coverage_swap(p,3,counts)==0);
 p[2].native_score=300;p[2].ovr=77;assert(hs28_coverage_swap(p,3,counts)==0);
 p[2].ovr=78;p[2].legal=0;assert(hs28_coverage_swap(p,3,counts)==0);
 p[2].legal=1;p[2].viable=0;assert(hs28_coverage_swap(p,3,counts)==0);
 p[2].viable=1;p[2].stars=2;p[2].native_score=500;assert(hs28_coverage_swap(p,3,counts)==0);
 p[2].stars=3;p[2].urgent=1;assert(hs28_coverage_swap(p,3,counts)==1);
 assert(p[0].weight==30 && p[2].weight==30);
 puts("PASS: zero-before-one coverage; same-opening backup replacement; protected primaries; preserved scoring/quality; no donor starvation; urgency weights");
}
