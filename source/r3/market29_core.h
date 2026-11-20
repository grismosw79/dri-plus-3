#ifndef DRI_MARKET29_CORE_H
#define DRI_MARKET29_CORE_H
#include <stdint.h>
#include <string.h>
#define MARKET29_TEAMS 138
#define MARKET29_ROUNDS 8
/* Numeric selection evidence only. Native scores, actions and ownership are
   never written here. Rejections redirect proposals toward close legal alternatives in the
   complete frozen planner. The final affordable roles then govern new entry
   for this weekly scope; established scholarships are never removed here. */
typedef struct {
 uint64_t active[3],firm[3],planned[3],primary[3],protected_offer[3];
 uint64_t admitted[3],rejected[3],exception[3];
 uint64_t backup_admitted[3],backup_rejected[3];
 uint64_t prior_planned[3],prior_primary[3];
 uint64_t hard_admitted[3],hard_rejected[3],hard_authorized[3];
 int fit[MARKET29_TEAMS],interest[MARKET29_TEAMS];
 int gain[MARKET29_TEAMS]; /* HS recent native influence gain; portal leaves zero. */
} Market29Pressure;
static inline void market29_save_plan(Market29Pressure *p) {
 memcpy(p->prior_planned,p->planned,sizeof(p->planned));memcpy(p->prior_primary,p->primary,sizeof(p->primary));
}
static inline int market29_plan_changed(const Market29Pressure *p) {
 return memcmp(p->prior_planned,p->planned,sizeof(p->planned)) || memcmp(p->prior_primary,p->primary,sizeof(p->primary));
}
static inline int market29_has(const uint64_t a[3],unsigned t) {
 return t<MARKET29_TEAMS && ((a[t/64]>>(t%64))&1u)!=0;
}
static inline void market29_bit(uint64_t a[3],unsigned t,int value) {
 if(t<MARKET29_TEAMS){uint64_t b=(uint64_t)1<<(t%64);if(value)a[t/64]|=b;else a[t/64]&=~b;}
}
static inline int market29_pressure(const Market29Pressure *p,unsigned own) {
 int n=0;for(unsigned t=0;t<MARKET29_TEAMS;t++)if(t!=own &&
  (market29_has(p->active,t)||market29_has(p->planned,t)))n++;
 return n;
}
/* Primary pressure counts destinations that can coexist, never conditional
   backups. Total attention is a separate, gentler backup selection policy. */
static inline int market29_primary_pressure(const Market29Pressure *p,unsigned own) {
 int n=0;for(unsigned t=0;t<MARKET29_TEAMS;t++)if(t!=own &&
  (market29_has(p->firm,t)||market29_has(p->primary,t)))n++;
 return n;
}
static inline int market29_penalty(int portal,int count) {
 return portal?(count<3?0:count<4?10:count<5?25:45):
               (count<5?0:count<6?10:count<8?25:45);
}
static inline int market29_backup_penalty(int portal,int count) {
 return portal?(count<4?0:count<6?25:45):(count<9?0:count<12?25:45);
}
static inline int market29_recovery_capacity(int capacity,int remaining,int cost,
 int nominated,int position_openings,int position_nominated) {
 return cost>0 && nominated>=0 && capacity>nominated && remaining/cost>nominated &&
  position_openings>position_nominated;
}
static inline uint32_t market29_tie(uint32_t recruit,unsigned school,unsigned year) {
 uint32_t x=recruit*2654435761u+school*2246822519u+year*3266489917u;
 x^=x>>16;x*=2246822519u;x^=x>>13;return x;
}
static inline int market29_before(const Market29Pressure *p,unsigned a,unsigned b,uint32_t rid,unsigned year) {
 int pa=market29_has(p->protected_offer,a),pb=market29_has(p->protected_offer,b);
 if(pa!=pb)return pa>pb;
 /* Existing preference is useful, but not a signing assignment. Only legal,
    affordable school proposals enter this comparison. */
 if(p->interest[a]!=p->interest[b])return p->interest[a]>p->interest[b];
 if(p->fit[a]!=p->fit[b])return p->fit[a]>p->fit[b];
 return market29_tie(rid,a,year)<market29_tie(rid,b,year);
}
/* One synchronous rejection round. Call only after EVERY school has completed
   its affordable proposal. Candidate iteration and native school order cannot
   change who retains priority. A rejected proposal may continue if no close
   legal alternative exists. Final feasible proposals survive the soft target. */
static inline int market29_reconcile(Market29Pressure *p,int portal,uint32_t rid,unsigned year) {
 int cap=portal?4:8,attention=portal?6:12,changed=0;
 for(unsigned t=0;t<MARKET29_TEAMS;t++) {
  int planned=market29_has(p->planned,t),primary=market29_has(p->primary,t);
  market29_bit(p->admitted,t,0);market29_bit(p->backup_admitted,t,0);
  if(!planned)continue;
  if(market29_has(p->protected_offer,t)||market29_has(p->exception,t)) {
   market29_bit(p->admitted,t,1);market29_bit(p->backup_admitted,t,1);continue;
  }
  int better=0;
  for(unsigned u=0;u<MARKET29_TEAMS;u++)if(u!=t) {
   int up=market29_has(p->firm,u)||market29_has(p->primary,u);
   int relevant=primary?up:(market29_has(p->active,u)||market29_has(p->planned,u));
   /* Primary destinations receive the attention places before backups. */
   if(relevant && ((!primary && up) || market29_before(p,u,t,rid,year)))better++;
  }
  uint64_t *admitted=primary?p->admitted:p->backup_admitted;
  uint64_t *rejected=primary?p->rejected:p->backup_rejected;
  if(better<(primary?cap:attention) && !market29_has(rejected,t))market29_bit(admitted,t,1);
  else if(!market29_has(rejected,t)){market29_bit(rejected,t,1);changed++;}
 }
 return changed;
}
/* Test30: a hard authorization is shared by every entry route. Existing
   offers are grandfathered, even in an over-limit loaded save. Exceptions
   affect soft preference only: they cannot create an extra authorization.
   Hard rejection remains sticky while the race is full; released capacity
   permits bounded reconsideration in a subsequent synchronous round. */
static inline int market30_count(const uint64_t a[3]) {
 int n=0;for(unsigned t=0;t<MARKET29_TEAMS;t++)n+=market29_has(a,t);return n;
}
static inline int market30_limit(int portal) {return portal?6:16;}
static inline int market30_hard_before(const Market29Pressure *p,unsigned a,unsigned b,uint32_t rid,unsigned year) {
 int pa=market29_has(p->protected_offer,a),pb=market29_has(p->protected_offer,b);
 if(pa!=pb)return pa>pb;
 int aa=market29_has(p->active,a),ab=market29_has(p->active,b);
 if(aa!=ab)return aa>ab;
 int ra=market29_has(p->primary,a)||market29_has(p->firm,a),rb=market29_has(p->primary,b)||market29_has(p->firm,b);
 if(ra!=rb)return ra>rb;
 return market29_before(p,a,b,rid,year);
}
static inline int market30_hard_reconcile(Market29Pressure *p,int portal,uint32_t rid,unsigned year) {
 int protected_count=0,changed=0,requests=0;
 for(unsigned t=0;t<MARKET29_TEAMS;t++)requests+=market29_has(p->protected_offer,t)||market29_has(p->planned,t)||market29_has(p->active,t);
 /* A withdrawn winner releases capacity: reconsider former losers in the
    next synchronous proposal rather than starving a newly sole destination. */
 if(requests<market30_limit(portal)) {
  for(unsigned t=0;t<MARKET29_TEAMS;t++)changed+=market29_has(p->hard_rejected,t);
  memset(p->hard_rejected,0,sizeof(p->hard_rejected));
 }
 memset(p->hard_admitted,0,sizeof(p->hard_admitted));
 for(unsigned t=0;t<MARKET29_TEAMS;t++)if(market29_has(p->protected_offer,t)) {
  market29_bit(p->hard_admitted,t,1);protected_count++;
 }
 for(unsigned t=0;t<MARKET29_TEAMS;t++) {
  if(market29_has(p->protected_offer,t) ||
    (!market29_has(p->planned,t) && !market29_has(p->active,t)))continue;
  int better=protected_count;
  for(unsigned u=0;u<MARKET29_TEAMS;u++)if(u!=t && !market29_has(p->protected_offer,u) &&
    !market29_has(p->hard_rejected,u) &&
    (market29_has(p->planned,u)||market29_has(p->active,u)) && market30_hard_before(p,u,t,rid,year))better++;
  if(better<market30_limit(portal) && !market29_has(p->hard_rejected,t))market29_bit(p->hard_admitted,t,1);
 }
 /* Publish losers only after comparing the full frozen population. */
 for(unsigned t=0;t<MARKET29_TEAMS;t++)if((market29_has(p->planned,t)||market29_has(p->active,t)) &&
   !market29_has(p->hard_admitted,t) && !market29_has(p->hard_rejected,t)) {
  market29_bit(p->hard_rejected,t,1);changed++;
 }
 return changed;
}
/* Finalize the bounded plan, then reserve spare authorizations for legal
   positively-scored recovery candidates in stable preference/fit order.
   Standby authorization is not an offer, role, funding witness or signing.
   All late recovery shares these finite slots; no execution-order allocator. */
static inline void market30_hard_freeze(Market29Pressure *p,int portal,uint32_t rid,unsigned year) {
 market30_hard_reconcile(p,portal,rid,year);
 int count=0;
 memcpy(p->hard_authorized,p->hard_admitted,sizeof(p->hard_authorized));
 for(unsigned t=0;t<MARKET29_TEAMS;t++) {
  count+=market29_has(p->hard_authorized,t);
  if(!market29_has(p->hard_admitted,t)) {
   market29_bit(p->planned,t,0);market29_bit(p->primary,t,0);
  }
 }
 while(count<market30_limit(portal)) {
  int best=-1;
  for(unsigned t=0;t<MARKET29_TEAMS;t++)if(p->fit[t]>0 && !market29_has(p->hard_authorized,t) &&
    (best<0 || market30_hard_before(p,t,(unsigned)best,rid,year)))best=(int)t;
  if(best<0)break;
  market29_bit(p->hard_authorized,(unsigned)best,1);market29_bit(p->hard_rejected,(unsigned)best,0);count++;
 }
}
#endif
