#ifndef DRI_PORTAL_PURSUIT_H
#define DRI_PORTAL_PURSUIT_H
#include "portal_policy.h"
/* H3 changes attention value, never room permission or a native score.
   The 82-current-OVR screen leaves ordinary lower-rated depth recruitment
   unchanged. Read the complete projected room, including secured arrivals
   and, when called by the planner, other selected primary additions. */
static int dri_portal_pursuit_percent(const PortalQuality *q,int prestige,int candidate,int raw) {
 if(raw<82 || q->pos<0 || q->pos>=22 || q->ideal<=0 ||
    q->count<0 || q->count>=192 || candidate<0 || candidate>99)return 100;
 int useful=dri_portal_useful_depth(q);
 if(useful<=0 || q->count<useful)return 100; /* actual missing useful depth */
 int rank=0;while(rank<q->count && q->rating[rank]>=candidate)rank++;
 if(rank<q->starters)return 100; /* a distinct starting role */
 int group=dri_group(prestige);
 if(q->pos==0 || q->pos==19 || q->pos==20) {
  /* A weak developmental backup is not a reason to buy another premium
     QB/K/P once a credible starter and the useful headcount already exist. */
  for(int i=0;i<q->starters;i++)
   if(i>=q->count || q->rating[i]*4<dri_quality_floor[group][q->pos][i])return 100;
  return 50;
 }
 for(int i=0;i<useful;i++) {
  int floor=dri_quality_floor[group][q->pos][i];
  if(floor>0 && q->rating[i]*4<floor)return 100; /* real rotation repair */
 }
 if(rank>=useful)return 40;
 if(q->pos==1)return 55;
 if(q->pos==17 || q->pos==18)return 65;
 return 80; /* multi-player rotations retain greater acquisition value */
}
static int dri_portal_pursuit_scale(int value,int percent) {
 if(value<=0)return value;
 if(percent<1 || percent>100)percent=100;
 int scaled=(int)((int64_t)value*percent/100);
 return scaled>0?scaled:1; /* a positive viable destination never becomes zero */
}
#endif
