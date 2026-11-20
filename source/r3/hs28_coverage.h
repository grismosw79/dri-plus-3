#ifndef DRI_HS28_COVERAGE_H
#define DRI_HS28_COVERAGE_H
#include "hs28_pairs.h"
/* Coverage is a market objective, never an extra score bonus. A replacement
   must be at least as good by the approved score and quality tie-break. */
static inline int hs28_coverage_comparable(const Hs28Choice *a,const Hs28Choice *b) {
 if(!a->legal || !a->viable || a->crowd_penalty>=45 || a->stars<3 || a->native_score<=0 || a->pos!=b->pos)return 0;
 if(hs28_selection_value(a)<hs28_selection_value(b))return 0;
 if(a->native_score!=b->native_score)return a->native_score>b->native_score;
 return a->ovr*10+a->stars*3>=b->ovr*10+b->stars*3;
}
static inline int hs28_coverage_swap(Hs28Choice *p,int n,const int *other_count) {
 int swaps=0;
 /* Zero-pursuit recruits get the first opportunity; then one-pursuit recruits.
    Only backups in races with two OTHER pursuits can be released. */
 for(int objective=0;objective<2;objective++) {
  for(;;) {
   int best=-1,drop=-1;
   for(int i=0;i<n;i++)if(!p[i].selected && other_count[i]==objective && p[i].stars>=3) {
    for(int j=0;j<n;j++)if(p[j].selected && p[j].choice==2 && other_count[j]>=2 &&
      hs28_coverage_comparable(&p[i],&p[j]) && (best<0 || hs28_better(&p[i],&p[best]))) {
     best=i;drop=j;
    }
   }
   if(best<0)break;
   p[best].selected=1;p[best].slot=p[drop].slot;p[best].choice=2;p[best].order=p[drop].order;
   p[best].weight=20;
   p[drop].selected=p[drop].slot=p[drop].choice=p[drop].weight=p[drop].order=0;
   for(int j=0;j<n;j++)if(p[j].selected && p[j].choice==1 && p[j].slot==p[best].slot) {
    p[j].weight=40;
    if(p[best].urgent && !p[j].urgent)p[j].weight=p[best].weight=30;
    break;
   }
   swaps++;
  }
 }
 return swaps;
}
#endif
