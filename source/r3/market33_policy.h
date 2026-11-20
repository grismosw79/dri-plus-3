#ifndef DRI_MARKET33_POLICY_H
#define DRI_MARKET33_POLICY_H
#include "market29_core.h"
#include "hs28_pairs.h"
/* HS-only policy. Enum stars are zero based; the absent one-star tier keeps
   the fallback8/16 limits. Portal callers continue to use market29_core unchanged. */
static inline int hs33_soft_stars(int stars) {return stars==1 || stars==2?3:8;}
static inline int hs33_limit_stars(int stars) {return stars>=3?12:stars==1 || stars==2?6:16;}
static inline int hs33_penalty_stars(int stars,int others) {
 if(stars==1 || stars==2)return others<2?0:others<3?10:others<5?25:45;
 return market29_penalty(0,others);
}
static inline int hs33_backup_penalty_stars(int stars,int others) {
 int soft=hs33_soft_stars(stars),hard=hs33_limit_stars(stars);
 return others<soft?0:others<hard?25:45;
}
typedef struct {
 int attention,primaries,offers,best_fit,leader,leader_gain,live,earned;
 unsigned char fit_rank[MARKET29_TEAMS];
} Market33Cache;
static inline void market33_cache(const Market29Pressure *p,Market33Cache *c,uint32_t rid,unsigned year) {
 memset(c,0,sizeof(*c));unsigned order[MARKET29_TEAMS];int n=0;
 for(unsigned t=0;t<MARKET29_TEAMS;t++) {
  c->attention+=market29_has(p->active,t)||market29_has(p->planned,t);
  c->primaries+=market29_has(p->firm,t)||market29_has(p->primary,t);
  c->offers+=market29_has(p->protected_offer,t);
  if(p->fit[t]>c->best_fit)c->best_fit=p->fit[t];
  if(p->fit[t]>0 && (market29_has(p->active,t)||market29_has(p->planned,t))) {
   c->live=1;if(p->interest[t]>c->leader)c->leader=p->interest[t];
   if(market29_has(p->protected_offer,t) && p->gain[t]>0)c->earned=1;
  }
  if(p->fit[t]<=0)continue;
  int j=n;while(j>0 && (p->fit[t]>p->fit[order[j-1]] ||
    (p->fit[t]==p->fit[order[j-1]] && market29_tie(rid,t,year)<market29_tie(rid,order[j-1],year)))) {
   order[j]=order[j-1];j--;
  }order[j]=t;n++;
 }
 for(int j=0;j<n;j++)c->fit_rank[order[j]]=(unsigned char)j;
 for(unsigned t=0;t<MARKET29_TEAMS;t++)if(p->fit[t]>0 &&
   (market29_has(p->active,t)||market29_has(p->planned,t)) && p->interest[t]==c->leader &&
   p->gain[t]>c->leader_gain)c->leader_gain=p->gain[t];
}
static inline int market33_primary_pressure(const Market29Pressure *p,const Market33Cache *c,unsigned own) {
 return c->primaries-(market29_has(p->firm,own)||market29_has(p->primary,own));
}
static inline int market33_attention(const Market29Pressure *p,const Market33Cache *c,unsigned own) {
 return c->attention-(market29_has(p->active,own)||market29_has(p->planned,own));
}
/* Same result as competition32_close, including a current choice whose points
   differ from the frozen snapshot. The rare own-leader exception rescans only
   after the ordinary85% fast path fails. */
static inline int market33_close(const Hs28Choice *p,const Market29Pressure *m,const Market33Cache *c,unsigned own) {
 int leader=c->live?c->leader:p->leader;
 if(p->stage>=4)return 1;
 if(leader<=0 || p->points<=0)return 0;
 if((int64_t)p->points*100>=(int64_t)leader*85)return 1;
 int leader_gain=c->leader_gain;
 if(own<MARKET29_TEAMS && m->interest[own]==leader && m->gain[own]==leader_gain) {
  leader_gain=0;for(unsigned t=0;t<MARKET29_TEAMS;t++)if(t!=own && m->fit[t]>0 &&
   (market29_has(m->active,t)||market29_has(m->planned,t)) && m->interest[t]==leader && m->gain[t]>leader_gain)leader_gain=m->gain[t];
 }
 int64_t closing=own<MARKET29_TEAMS?(int64_t)m->gain[own]-leader_gain:0;
 return closing>0 && (int64_t)leader-p->points<=closing*3;
}
static inline int market33_strong_fit(const Market29Pressure *p,const Market33Cache *c,unsigned own) {
 return own<MARKET29_TEAMS && c->best_fit>0 && p->fit[own]>0 && c->fit_rank[own]<3 &&
  (int64_t)p->fit[own]*100>=(int64_t)c->best_fit*90;
}
/* Current native scoring can change after the snapshot; preserve the original
   fit comparison in that rare case, while unchanged choices use cached ranks. */
static inline int market33_strong_choice(const Hs28Choice *p,const Market29Pressure *m,
 const Market33Cache *c,unsigned own,uint32_t rid,unsigned year) {
 if(own>=MARKET29_TEAMS || p->native_score<=0 || c->best_fit<=0 ||
   (int64_t)p->native_score*100<(int64_t)c->best_fit*90)return 0;
 if(p->native_score==m->fit[own])return c->fit_rank[own]<3;
 int better=0;for(unsigned t=0;t<MARKET29_TEAMS;t++)if(m->fit[t]>p->native_score ||
   (m->fit[t]==p->native_score && market29_tie(rid,t,year)<market29_tie(rid,own,year)))better++;
 return better<3;
}
typedef struct {uint64_t eligible[3],need[3],recovered[3],failed[3],inflight[3];int stars,stage;} Market33Admissions;
static inline int market33_rank_before(const Market29Pressure *p,const Market33Admissions *a,
 const Market33Cache *c,unsigned x,unsigned y,uint32_t rid,unsigned year) {
 int xp=market29_has(p->firm,x)||market29_has(p->primary,x),yp=market29_has(p->firm,y)||market29_has(p->primary,y);
 if(xp!=yp)return xp>yp;
 /* Earned native influence orders established races; opening nominations
    use native fit first. No fabricated influence or prestige-specific quota. */
 if(c->earned && p->interest[x]!=p->interest[y])return p->interest[x]>p->interest[y];
 if(p->fit[x]!=p->fit[y])return p->fit[x]>p->fit[y];
 int xn=market29_has(a->need,x),yn=market29_has(a->need,y);if(xn!=yn)return xn>yn;
 if(p->interest[x]!=p->interest[y])return p->interest[x]>p->interest[y];
 if(p->gain[x]!=p->gain[y])return p->gain[x]>p->gain[y];
 return market29_tie(rid,x,year)<market29_tie(rid,y,year);
}
static inline int market33_exception(const Market29Pressure *p,const Market33Admissions *a,
 const Market33Cache *c,unsigned t) {
 if(market29_has(a->need,t) || market33_strong_fit(p,c,t) || market29_has(a->recovered,t))return 1;
 if(!c->earned && a->stage<4)return 0;
 Hs28Choice q={0};q.points=p->interest[t];q.stage=a->stage;
 return market33_close(&q,p,c,t);
}
/* Recomputed, never sticky: unsuccessful or no-longer-selected nominations
   release capacity. Only actual offers survive a completed school turn.
   The returned mask admits school membership, not a frozen primary role. */
static inline int market33_select(Market29Pressure *p,const Market33Admissions *a,const Market33Cache *c,
 const uint64_t completed[3],uint32_t rid,unsigned year) {
 uint64_t old[3];memcpy(old,p->hard_authorized,sizeof(old));
 memcpy(p->hard_authorized,p->protected_offer,sizeof(p->hard_authorized));
 memset(p->hard_rejected,0,sizeof(p->hard_rejected));
 unsigned order[MARKET29_TEAMS];int n=0,used=c->offers;
 for(unsigned t=0;t<MARKET29_TEAMS;t++)if(market29_has(a->inflight,t) && market29_has(a->eligible,t) &&
   !market29_has(p->protected_offer,t) && !market29_has(a->failed,t) && !market29_has(completed,t)) {
  market29_bit(p->hard_authorized,t,1);used++;
 }
 for(unsigned t=0;t<MARKET29_TEAMS;t++)if(market29_has(a->eligible,t) &&
   !market29_has(p->hard_authorized,t) && !market29_has(a->failed,t) &&
   !market29_has(completed,t) && p->fit[t]>0) {
  int j=n;while(j>0 && market33_rank_before(p,a,c,t,order[j-1],rid,year)){order[j]=order[j-1];j--;}
  order[j]=t;n++;
 }
 for(int i=0;i<n;i++) {
  unsigned t=order[i];int yes=used<hs33_limit_stars(a->stars) &&
   (used<hs33_soft_stars(a->stars) || market33_exception(p,a,c,t));
  market29_bit(p->hard_authorized,t,yes);market29_bit(p->hard_rejected,t,!yes);if(yes)used++;
 }
 return memcmp(old,p->hard_authorized,sizeof(old))!=0;
}
#endif
