#ifndef DRI_PORTAL_POLICY_H
#define DRI_PORTAL_POLICY_H
#include <stdint.h>
#include "roster_budget.h"
#include "quality_bands.h"
static int dri_portal_star(int ovr,int year) {
 int rating=ovr+(year==0?2:(year==1?1:0));
 return rating>=90?4:(rating>=82?3:(rating>=75?2:(rating>=70?1:0)));
}
typedef struct {
 int pos,ideal,starters,count,base_count,incoming_best;
 int rating[192],incoming[192];
 /* Set from the complete team projection; never infer a roster vacancy from
    a single position. One flex addition consumes this position's permission. */
 int flex_allowed;
 int team_excess;
 /* Test26: feasible allocations of secured extra arrivals to the 2/2/1
    buckets. Rebuilt from the save; restarting never replenishes allowance. */
 uint32_t exception_states;
 /* Current HS/JUCO acquisitions in base_count. They retain role protection,
    but newly created overages must share the two general exception places. */
 int hs_count;
 /* Offered, native-funded HS primary vacancies reserved by the shared class
    plan. They are not players and never alter actual roster accounting. */
 int hs_reserved;
 /* Test31 annual class permission is distinct from discretionary extras.
    This is a quota only; actual hs_count identities determine its use. */
 int hs_normal_limit;
 /* Distinguishes an actual zero annual quota from legacy policy fixtures.
    Real collectors always publish this flag alongside the annual map. */
 int hs_annual_valid;
 /* R3: current quality is separate from projected role. Collector metadata
    follows the full returning/secured room, not a cached preferred school. */
 int qb_best_current,qb_leader_rating,qb_leader_priority,qb_young_leader;
} PortalQuality;
static int dri_group(int prestige) {return prestige<2?0:prestige<4?1:prestige<6?2:prestige<8?3:4;}
#define DRI_PORTAL_QUALITY_POINT_VALUE 15
#define DRI_PORTAL_ELITE_QB 82
#define DRI_QB_YOUNG_MIN 80
#define DRI_QB_DEPTH_GAP 5
static inline int dri_qb_young(int raw,int year,int dev,int returning) {
 return returning && raw>=DRI_QB_YOUNG_MIN && year>=0 && year<=1 && dev>=2 && dev<=3;
}
static inline void dri_qb_observe(PortalQuality *q,int raw,int projected,int young,int priority) {
 if(q->pos!=0)return;
 if(raw>q->qb_best_current)q->qb_best_current=raw;
 if(projected>q->qb_leader_rating || (projected==q->qb_leader_rating && priority>q->qb_leader_priority)) {
  q->qb_leader_rating=projected;q->qb_leader_priority=priority;q->qb_young_leader=young;
 }
}
static inline int dri_qb_block(const PortalQuality *q,int candidate,int raw) {
 if(q->pos!=0 || !q->count)return 0;
 int best=q->rating[0];
 if(q->qb_young_leader && q->qb_leader_rating==best &&
    (raw>=DRI_PORTAL_ELITE_QB || candidate>best-DRI_QB_DEPTH_GAP))return 20;
 if(q->qb_best_current>=DRI_PORTAL_ELITE_QB &&
    (raw>=DRI_PORTAL_ELITE_QB || candidate>best-DRI_QB_DEPTH_GAP))return 2;
 if(q->incoming_best>=0) {
  /* Symmetric: depth-first must leave the same five-point separation as
     starter-first. A sub-82 QB can be a starter in a genuinely weak room. */
  if(candidate>=best) {if(q->incoming_best>candidate-DRI_QB_DEPTH_GAP)return 2;}
  else if(raw>=DRI_PORTAL_ELITE_QB || candidate>best-DRI_QB_DEPTH_GAP)return 2;
 }
 return 0;
}
/* Useful rotation, not every developmental roster seat. Preserve schemes
   with more starters; cap every horizon at the configured position ideal.
   QB/OL/LB/S/K/P keep their existing two-deep horizon. Zero-ideal FB/LS stay
   closed. Receiving and defensive-front rotations have position-specific
   minimums, including HB3, TE3, WR5, edge3, DT4 and CB5. */
static inline int dri_portal_useful_depth(const PortalQuality *q) {
 static const unsigned char minimum[22]={2,3,2,5,3,2,2,2,2,2,3,3,4,2,2,2,5,2,2,2,2,0};
 int n=2*q->starters;
 if(q->pos>=0 && q->pos<22 && n<minimum[q->pos])n=minimum[q->pos];
 if(n>q->ideal)n=q->ideal;
 if(n>22)n=22;
 return n>0?n:0;
}
static inline int dri_portal_extra_position(const PortalQuality *q) {
 return q->ideal>0 && q->pos!=0 && q->pos!=19 && q->pos!=20;
}
/* R4: a useful, prestige-appropriate contributor is not a developmental
   overage. Re-evaluate the complete projected room, including secured HS and
   transfers. Ties follow incumbents: equal players cannot extend a full
   rotation forever. This does not waive native eligibility or spend hours.
   QB and specialist protections retain their separate existing rules. */
static inline int dri_portal_quality_depth_rank(const PortalQuality *q,int prestige,int candidate) {
 if(!dri_portal_extra_position(q) || q->pos<1 || q->pos>18 ||
    q->count<0 || q->count>=192 || candidate<0 || candidate>99)return 0;
 int rank=0;while(rank<q->count && q->rating[rank]>=candidate)rank++;
 if(rank>=dri_portal_useful_depth(q))return 0;
 int floor=dri_quality_floor[dri_group(prestige)][q->pos][rank];
 return floor>0 && candidate*4>=floor?rank+1:0;
}
static inline int dri_portal_quality_depth_minimum(const PortalQuality *q,int prestige) {
 if(!dri_portal_extra_position(q) || q->pos<1 || q->pos>18)return 100;
 int minimum=100;
 for(int rank=0;rank<dri_portal_useful_depth(q);rank++) {
  int floor=dri_quality_floor[dri_group(prestige)][q->pos][rank];
  if(!floor)continue;
  int candidate=(floor+3)/4;
  if(rank<q->count && candidate<=q->rating[rank])candidate=q->rating[rank]+1;
  if(candidate<minimum && dri_portal_quality_depth_rank(q,prestige,candidate))minimum=candidate;
 }
 return minimum;
}
static inline int dri_portal_normal_cap(const PortalQuality *q) {
 return (q->base_count>q->ideal?q->base_count:q->ideal)+1;
}
static int dri_overage_full(const PortalQuality *q) {return q->count+q->hs_reserved>=dri_portal_normal_cap(q);}
/* Only the 83/84 ideal-total shortfall is flexible. Existing position overages
   consume that headcount already; ordinary vacancies must still be filled. */
static inline int dri_portal_flex_budget(const PortalQuality rooms[22]) {
 int ideals=0,covered=0;
 for(int p=0;p<22;p++) {
  ideals+=rooms[p].ideal;
  int effective=rooms[p].count+rooms[p].hs_reserved;
  covered+=effective>rooms[p].ideal?effective:rooms[p].ideal;
 }
 if(ideals!=83 && ideals!=84)return 0;
 return covered<85?85-covered:0;
}
static inline int dri_portal_set_flex(PortalQuality rooms[22]) {
 int budget=dri_portal_flex_budget(rooms);
 int counts[22],ideals[22];
 for(int p=0;p<22;p++){counts[p]=rooms[p].count+rooms[p].hs_reserved;ideals[p]=rooms[p].ideal;}
 int excess=dri_team_excess(counts,ideals);
 for(int p=0;p<22;p++) {
  rooms[p].team_excess=excess;
  rooms[p].flex_allowed=budget>0 && excess<DRI_TEAM_OPTIONAL_LIMIT &&
   dri_portal_extra_position(&rooms[p]) && !rooms[p].hs_reserved && rooms[p].count==rooms[p].ideal;
 }
 return budget;
}
static inline int dri_portal_flex_candidate(const PortalQuality *q,int candidate) {
 return q->flex_allowed && dri_portal_extra_position(q) && q->count==q->ideal &&
        q->count>0 && candidate>=q->rating[q->count-1];
}
/* Test29: inadequate useful roles retain their three-point repair rule.
   Adequate roles may create bounded demand for an actual material upgrade:
   five projected points at a starting seat, four at useful rotation. These
   permissions never expand the position cap or the shared 2/2/1 allowance. */
static inline int dri_portal_upgrade_minimum(const PortalQuality *q,int prestige,int i) {
 if(i<0 || i>=dri_portal_useful_depth(q))return 100;
 int current=i<q->count?q->rating[i]:0;
 int floor=dri_quality_floor[dri_group(prestige)][q->pos][i];
 if(i<q->count && q->incoming[i])return 100;
 if(i>=q->count && q->hs_reserved>0)return 100;
 if(floor && current*4<floor)return current+3;
 if(i<q->count)return current+(i<q->starters?5:4);
 return 100;
}
static inline int dri_portal_seat_upgrade(const PortalQuality *q,int prestige,
                                         int candidate,int current_rating,int i) {
 (void)current_rating;
 /* Both values use the same next-season estimate. A secured portal arrival
    protects its filled role even if the new candidate is materially better. */
 if(candidate<dri_portal_upgrade_minimum(q,prestige,i))return 0;
 int current=i<q->count?q->rating[i]:0;
 int floor=dri_quality_floor[dri_group(prestige)][q->pos][i];
 if(floor && current*4<floor)return 1; /* Existing inadequate-role repair. */
 int rank=0;while(rank<q->count && q->rating[rank]>candidate)rank++;
 /* A marginal new starter cannot qualify through a much weaker backup.
    Material adequacy upgrades must improve the category the newcomer plays. */
 return rank<dri_portal_useful_depth(q) && ((rank<q->starters)==(i<q->starters));
}
static int dri_quality_rank_candidate(const PortalQuality *q,int prestige,int candidate,int current_rating) {
 int n=dri_portal_useful_depth(q);
 for(int i=0;i<n;i++)if(dri_portal_seat_upgrade(q,prestige,candidate,current_rating,i))return i+1;
 return 0;
}
#define DRI_EXCEPTION_GENERAL 1u
#define DRI_EXCEPTION_EMERGENCY 2u
#define DRI_EXCEPTION_RB 4u
#define DRI_EXCEPTION_READY 0x40000000u
static inline uint32_t dri_exception_step(uint32_t states,unsigned mask) {
 uint32_t next=0;
 for(int g=0;g<=2;g++)for(int e=0;e<=2;e++)for(int r=0;r<=1;r++) {
  unsigned bit=(unsigned)((g*3+e)*2+r);
  if(!(states&(1u<<bit)))continue;
  if((mask&DRI_EXCEPTION_GENERAL) && g<2)next|=1u<<(bit+6);
  if((mask&DRI_EXCEPTION_EMERGENCY) && e<2)next|=1u<<(bit+2);
  if((mask&DRI_EXCEPTION_RB) && r<1)next|=1u<<(bit+1);
 }
 return next;
}
static unsigned dri_portal_exception_mask(const PortalQuality *q,int prestige,int candidate) {
 unsigned mask=DRI_EXCEPTION_GENERAL;
 /* Special allowances repair inherited full rooms. A normal vacancy repair
    is free, and cannot subsequently create another emergency at this room. */
 if(q->base_count<q->ideal || q->incoming_best>=0)return mask;
 /* A material QB1 improvement uses the existing starter-emergency pool.
    The elite-QB and one-arrival guards still apply before acceptance. */
 if(q->pos==0 && q->count>0 && !q->incoming[0] && candidate>=q->rating[0]+5)
  mask|=DRI_EXCEPTION_EMERGENCY;
 for(int i=0;i<q->starters && i<q->count && i<22;i++) {
  int floor=dri_quality_floor[dri_group(prestige)][q->pos][i];
  if(floor-q->rating[i]*4>=20 && candidate>=q->rating[i]+5 && !q->incoming[i])
   mask|=DRI_EXCEPTION_EMERGENCY;
 }
 if(q->pos==1 && q->base_count==q->ideal)for(int i=1;i<3 && i<q->count;i++) {
  int floor=dri_quality_floor[dri_group(prestige)][1][i];
  if(floor>q->rating[i]*4 && candidate>=q->rating[i]+5 && !q->incoming[i])mask|=DRI_EXCEPTION_RB;
 }
 return mask;
}
static int dri_portal_exception_available(const PortalQuality *q,int prestige,int candidate) {
 uint32_t states=q->exception_states;
 if(!(states&DRI_EXCEPTION_READY))states=1u;
 return dri_exception_step(states,dri_portal_exception_mask(q,prestige,candidate))!=0;
}
static inline int dri_hs_normal_ceiling(const PortalQuality *q) {
 int inherited=q->base_count-q->hs_count;
 int normal=q->hs_normal_limit>0?q->hs_normal_limit:0;
 if(normal>q->hs_count)normal=q->hs_count;
 int ceiling=inherited+normal;
 return ceiling>q->ideal?ceiling:q->ideal;
}
static inline int dri_hs_extra_count(const PortalQuality *q) {
 int extra=q->base_count-dri_hs_normal_ceiling(q);
 /* Test34's under-ideal vacancy repair still spends a general place.
    Reconstruct from secured identities, including zero-quota positions,
    so a reload or later portal arrival cannot refund this permission. */
 if(q->hs_annual_valid) {
  int annual=q->hs_count-(q->hs_normal_limit>0?q->hs_normal_limit:0);
  if(annual>extra)extra=annual;
 }
 return extra>0?extra:0;
}
static inline int dri_hs_reserved_extra_count(const PortalQuality *q) {
 if(!q->hs_annual_valid || q->hs_reserved<=0)return 0;
 int normal=q->hs_normal_limit>q->hs_count?q->hs_normal_limit-q->hs_count:0;
 return q->hs_reserved>normal?q->hs_reserved-normal:0;
}
static uint32_t dri_portal_set_exceptions(PortalQuality rooms[22],int prestige) {
 uint32_t states=1u;
 /* Protect inherited returners and actual normal annual HS/JUCO arrivals.
    A reload reconstructs this charge from player identities and the stable
    annual map; neither offers nor forecasts create exempt players. Only HS
    arrivals beyond normal intake can use shared general places. Emergency/RB
    exceptions remain portal-only and the same 2+2+1 state spans both pools. */
 for(int pos=0;pos<22;pos++) {
  const PortalQuality *q=&rooms[pos];
  int extra=dri_hs_extra_count(q)+dri_hs_reserved_extra_count(q);
  for(int i=0;i<extra;i++)states=dri_exception_step(states,DRI_EXCEPTION_GENERAL);
 }
 for(int pos=0;pos<22;pos++) {
  const PortalQuality *q=&rooms[pos];
  int ceiling=q->base_count>q->ideal?q->base_count:q->ideal;
  int extra=q->count+q->hs_reserved-ceiling;
  if(extra<=0)continue;
  PortalQuality base=*q;base.count=0;base.incoming_best=-1;
  int incoming[192],n=0;
  for(int i=0;i<q->count;i++) {
   if(q->incoming[i])incoming[n++]=q->rating[i];
   else {int k=base.count++;base.rating[k]=q->rating[i];base.incoming[k]=0;}
  }
  /* Quality-depth acquisitions have their own permission, reconstructed from
     actual incoming identities against the non-portal baseline. They do not
     spend (or replenish) general/emergency/RB allowances. Later quality
     arrivals must still qualify against the complete current room at every
     decision; reconstruction never makes a new candidate committable. */
  int quality=0;
  for(int i=0;i<n;i++)if(dri_portal_quality_depth_rank(&base,prestige,incoming[i]))quality++;
  extra-=quality;if(extra<=0)continue;
  /* If the baseline was already full, the one-per-position cap makes this
     the sole extra arrival. Other full rooms charge the general allowance. */
  for(int i=0;i<extra;i++) {
   unsigned mask=DRI_EXCEPTION_GENERAL;
   if(q->base_count>=q->ideal && extra==1 && n==1)
    mask=dri_portal_exception_mask(&base,prestige,incoming[0]);
   states=dri_exception_step(states,mask);
  }
 }
 states|=DRI_EXCEPTION_READY;
 for(int pos=0;pos<22;pos++)rooms[pos].exception_states=states;
 return states;
}
static inline void dri_portal_refresh_budget(PortalQuality rooms[22],int prestige) {
 dri_portal_set_flex(rooms);dri_portal_set_exceptions(rooms,prestige);
}
static inline void dri_portal_set_hs_reservations(PortalQuality rooms[22],const int reserved[22],int prestige) {
 for(int p=0;p<22;p++) {
  /* A material portal addition can coexist with a paid HS primary through
     an existing general exception. Measure the HS quantity seat against
     actual returners/HS, so the portal arrival cannot erase that reservation
     and silently refund its shared allowance before the HS decision. */
  int vacancy=rooms[p].ideal-rooms[p].base_count;if(vacancy<0)vacancy=0;
  int count=reserved[p];if(count<0)count=0;if(count>vacancy)count=vacancy;
  rooms[p].hs_reserved=count;
 }
 dri_portal_refresh_budget(rooms,prestige);
}
static int dri_quality_block_candidate(const PortalQuality *q,int prestige,int candidate,int current) {
 if(q->ideal==0)return 3;
 if(q->count>=192)return 4;
 int best=q->count?q->rating[0]:0;
 /* All projected incumbents count, including returners and secured HS/JUCO.
    This is an incoming CPU decision, never a cut of an existing player. */
 /* Premium centers may fill two true vacancies. Once the first portal center
    fills the only vacancy, a second premium backup would duplicate that job. */
 if(q->pos==7 && q->starters==1 && current>=80 && q->incoming_best>=82 &&
    q->count>=q->ideal && q->count && candidate<=q->rating[0]+4)return 19;
 int qb_block=dri_qb_block(q,candidate,current);if(qb_block)return qb_block;
 if((q->pos==19 || q->pos==20) && q->incoming_best>=0) {
  int upper=candidate>q->incoming_best?candidate:q->incoming_best;
  int final_best=candidate>best?candidate:best;
  int gap=candidate-q->incoming_best;if(gap<0)gap=-gap;
  if(upper>=final_best-3 && gap<=3)return 2;
 }
 /* A credible starter/rotation addition may coexist with paid HS development
    and previously secured players even above the positional target. The
    quality test, not the shared optional-player ledger, bounds this route. */
 if(dri_portal_quality_depth_rank(q,prestige,candidate))return 0;
 /* A paid HS primary keeps its quantity path. A material portal upgrade may
    coexist using the existing general allowance, while the reservation stays
    in effective count and the shared budget. Mere vacancy filling or a flex
    preference cannot displace it; no new exception or position cap is added.
    The unchanged HS optional path supports only a first acquisition in this
    position. Multiple paid HS vacancies, or an existing signed HS there,
    therefore keep their ordinary source protection until they resolve. */
 if(q->hs_reserved>0 && q->count+q->hs_reserved>=q->ideal &&
    (q->hs_reserved>1 || q->hs_count>0 ||
     !dri_quality_rank_candidate(q,prestige,candidate,current)))return 18;
 /* A vacant position and an actual starter upgrade remain accessible when
    the rest of the roster exceeds its optional allowance. Protect paid HS
    reservations and any starter already secured through the portal. */
 if(q->count+q->hs_reserved<q->ideal)return 0;
 if(q->hs_reserved==0)for(int i=0;i<q->starters && i<q->count;i++)
  if(!q->incoming[i] && dri_portal_seat_upgrade(q,prestige,candidate,current,i))return 0;
 if(dri_overage_full(q))return 4;
 if(q->count+q->hs_reserved>=q->ideal) {
  if(!dri_quality_rank_candidate(q,prestige,candidate,current) &&
     !dri_portal_flex_candidate(q,candidate))return 1;
  if(!dri_portal_exception_available(q,prestige,candidate))return 8;
 }
 return 0;
}
/* Test25: admission and valuation answer different questions. A real vacancy
   permits comparing useful improvements even when the soft minimum is met.
   This never opens a full room: the unchanged admission gate runs first. */
static inline int dri_portal_value_seat(const PortalQuality *q,int prestige,int candidate,int current,int i) {
 if(dri_portal_seat_upgrade(q,prestige,candidate,current,i))return 1;
 return i<q->count && !q->incoming[i] && candidate>=q->rating[i]+3;
}
static int dri_portal_value_rank(const PortalQuality *q,int prestige,int candidate,int current) {
 if(dri_quality_block_candidate(q,prestige,candidate,current))return 0;
 for(int i=0;i<dri_portal_useful_depth(q);i++)
  if(dri_portal_value_seat(q,prestige,candidate,current,i))return i+1;
 return dri_portal_quality_depth_rank(q,prestige,candidate);
}
static int dri_quality_bonus_candidate(const PortalQuality *q,int prestige,int candidate,int current,int *rank) {
 *rank=0;if(dri_quality_block_candidate(q,prestige,candidate,current))return 0;
 int k=dri_portal_value_rank(q,prestige,candidate,current);if(!k)return 0;
 int starters=q->starters<q->count?q->starters:q->count;
 int vacancy_reference=starters?q->rating[starters-1]:0;
 int best=0,n=dri_portal_useful_depth(q);
 /* Price actual useful improvements against their own incumbents. Use the
    strongest single improvement, never add multiple seat bonuses together.
    Taking the maximum prevents a better player losing value when his first
    qualifying role changes from rotation to starter. Additional rotation
    beyond the original two-deep gets a smaller role premium (50 vs 100). */
 for(int i=0;i<n;i++)if(dri_portal_value_seat(q,prestige,candidate,current,i)) {
  int reference=i<q->count?q->rating[i]:vacancy_reference;
  int premium=dri_portal_seat_upgrade(q,prestige,candidate,current,i)?
    (i<q->starters?200:i<2*q->starters?100:50):0;
  int bonus=premium+(candidate-reference)*DRI_PORTAL_QUALITY_POINT_VALUE;
  if(bonus>best)best=bonus;
 }
 /* A qualifying rotation contributor can be valuable without a four/five
    point material-upgrade gap or an unprotected incumbent. Use one modest
    role premium; never add a second bonus to the existing improvement price. */
 int depth=dri_portal_quality_depth_rank(q,prestige,candidate);
 if(depth && best<50)best=50;
 *rank=k;return best;
}
/* Current-rating callers and legacy direct policy tests can use these wrappers.
   Production projected-room callers must pass the exact raw OVR above. */
static inline int dri_quality_rank(const PortalQuality *q,int prestige,int candidate) {
 return dri_quality_rank_candidate(q,prestige,candidate,candidate);
}
static inline int dri_quality_block(const PortalQuality *q,int prestige,int candidate) {
 return dri_quality_block_candidate(q,prestige,candidate,candidate);
}
static inline int dri_quality_bonus(const PortalQuality *q,int prestige,int candidate,int *rank) {
 return dri_quality_bonus_candidate(q,prestige,candidate,candidate,rank);
}
#endif
