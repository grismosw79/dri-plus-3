#include "../portal_policy.h"
int quality_replay(const PortalQuality *q,int prestige,int candidate,int *bonus,int *rank) {
 *bonus=dri_quality_bonus(q,prestige,candidate,rank);return dri_quality_block(q,prestige,candidate);
}
int star_replay(int ovr,int year){return dri_portal_star(ovr,year);}

int quality_replay_current(const PortalQuality *q,int prestige,int projected,int current,int *bonus,int *rank) {
 *bonus=dri_quality_bonus_candidate(q,prestige,projected,current,rank);return dri_quality_block_candidate(q,prestige,projected,current);
}
