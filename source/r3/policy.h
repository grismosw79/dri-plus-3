#ifndef DRI_RETENTION_POLICY_H
#define DRI_RETENTION_POLICY_H
#include <stdint.h>
#include "roster_budget.h"
static int dri_close_challenger(int stage,int rank,int held,int eligible,int own,
                               uint32_t influence,uint32_t leader) {
 return (stage==1 || stage==2 || stage==4) && (rank==2 || rank==3) && held &&
        eligible && !own && leader>0 && (uint64_t)influence*100 >= (uint64_t)leader*80;
}
static int dri_quota(int base,int need) { return base>need?base:need; }
/* No .5 ties occur for integer ideals divided by three. */
static int dri_annual_limit(int position,int ideal) {
 if((position==2 || position==21) && ideal==0) return 0;
 int n=(ideal+1)/3;return n<1?1:n;
}
/* Test15: each specialist role has its own target of two. A surplus in the
   other role must not suppress a genuine vacancy in this one. */
static int dri_specialist_open(int own_role,int other_role) {
 (void)other_role;return own_role<2;
}
/* Annual class balance remains the normal allowance. Once it is exhausted,
   actual future-roster vacancies reopen the position. Counts include secured
   HS/JUCO and portal arrivals exactly once, never mere pursuit reservations. */
static int dri_hs_remaining(int position,int ideal,int limit,int commits,
                            int projected,int flex) {
 if(ideal<=0)return 0;
 if(position==19 || position==20)
  return dri_specialist_open(projected,0)?2-projected:0;
 int annual=limit-commits,vacancy=ideal+flex-projected;
 int result=annual>vacancy?annual:vacancy;
 return result>0?result:0;
}
static int dri_position_closed(int position,int ideal,int limit,int commits,
                              int projected,int flex) {
 return !dri_hs_remaining(position,ideal,limit,commits,projected,flex);
}
/* Stars use the native enum (0..4). These are CPU planning restrictions;
   runtime callers explicitly bypass the user team before invoking them. */
static int dri_hs_candidate_block(int position,int ideal,int limit,int commits,
 int projected,int flex,int stars,int four,int five,int candidate_ovr,int qb85) {
 if(stars==4 && five>0)return 2;
 if(stars==3 && four+five>0 && projected>=ideal+flex)return 3;
 if(position==0 && candidate_ovr>=85 && qb85>0)return 4;
 return dri_position_closed(position,ideal,limit,commits,projected,flex)?1:0;
}
static int dri_protection_quota(int closed,int native_quota) {
 return closed?0:(native_quota<2?2:native_quota);
}
#endif
