#ifndef DRI_COMPETITION32_POLICY_H
#define DRI_COMPETITION32_POLICY_H
#include "market29_core.h"
#include "hs28_pairs.h"
/* Test32: a live opportunity is not the same as a displayed scholarship.
   A close contender or a challenger making credible net progress keeps its
   race. Forecast values change selection utility only, never native points. */
static inline int competition32_close(const Hs28Choice *p,const Market29Pressure *m,unsigned own) {
 int leader=0,gain=0,actual=0;
 for(unsigned t=0;t<MARKET29_TEAMS;t++)if(m->fit[t]>0 &&
   (market29_has(m->active,t)||market29_has(m->planned,t))) {
  actual=1;if(m->interest[t]>leader)leader=m->interest[t];
 }
 /* Initial seeded interest is useful before any feasible school pursues.
    Once a live/affordable race exists, a withdrawn or illegal old leader
    cannot suppress the actual challengers or supply a fictitious gain. */
 if(!actual)leader=p->leader;
 if(p->stage>=4)return 1;
 if(leader<=0 || p->points<=0)return 0;
 if((int64_t)p->points*100>=(int64_t)leader*85)return 1;
 for(unsigned t=0;t<MARKET29_TEAMS;t++)if(t!=own && m->fit[t]>0 && (market29_has(m->active,t)||market29_has(m->planned,t)) &&
   m->interest[t]==leader && m->gain[t]>gain)gain=m->gain[t];
 int64_t closing=own<MARKET29_TEAMS?(int64_t)m->gain[own]-gain:0;
 /* A three-week catch-up window protects an improving outsider without
    treating any positive gain, even one slower than the leader, as progress. */
 return closing>0 && (int64_t)leader-p->points<=(int64_t)closing*3;
}
static inline int competition32_before(const Market29Pressure *m,unsigned a,unsigned b,uint32_t rid,unsigned year) {
 if(m->interest[a]!=m->interest[b])return m->interest[a]>m->interest[b];
 if(m->fit[a]!=m->fit[b])return m->fit[a]>m->fit[b];
 return market29_tie(rid,a,year)<market29_tie(rid,b,year);
}
static inline int competition32_better(const Market29Pressure *m,unsigned own,uint32_t rid,unsigned year) {
 int n=0;
 for(unsigned t=0;t<MARKET29_TEAMS;t++)if(t!=own &&
   (market29_has(m->firm,t)||market29_has(m->primary,t)) && competition32_before(m,t,own,rid,year))n++;
 return n;
}
static inline int competition32_opportunity(const Hs28Choice *p,const Hs28Choice *alternative,
 int current_others,int alternative_others,int better,int preserve) {
 if(preserve || p->stage>=4 || p->stars<3 || alternative->stars<3 || !alternative->legal ||
   !alternative->viable || alternative->native_score<=0 || alternative->pos!=p->pos ||
   alternative->ovr<p->ovr-3 || (int64_t)alternative->native_score*100<(int64_t)p->native_score*70 ||
   current_others<3 || better<3 || alternative_others>=2 || alternative_others>=current_others)return 0;
 /* Several stronger rivals keep a competitive race. A marginal extra
    pursuit may prefer a near-quality elite with no, or only one, firm path.
    This is a bounded preference, not a required contender count or quota. */
 return alternative_others==0?25:10;
}
#endif
