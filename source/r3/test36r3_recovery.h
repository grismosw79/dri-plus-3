#ifndef DRI_TEST36R3_RECOVERY_H
#define DRI_TEST36R3_RECOVERY_H
/* Native stars are zero-based. HS retains its original 4/5-star threshold. */
static inline int r3_recovery_stars(int portal,int stars) {return stars>=(portal?2:3) && stars<=4;}
static inline int r3_recovery_needed(int portal,int stars,int stage,int working,int published) {
 return r3_recovery_stars(portal,stars) && stage>=0 && stage<5 && !working && !published;
}
#endif
