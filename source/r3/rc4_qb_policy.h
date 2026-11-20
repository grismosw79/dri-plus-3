#ifndef DRI_RC4_QB_POLICY_H
#define DRI_RC4_QB_POLICY_H
#include <stdint.h>
/* A two-point QB improvement competes only when the opening-specific recruiting
   value is at least three quarters of the school's original first choice. */
static inline int rc4_competes(int rating,int merit,int preferred_rating,int preferred_merit){
    return merit>0 && preferred_merit>0 && rating>=preferred_rating+2 &&
           (int64_t)merit*4>=(int64_t)preferred_merit*3;
}
#endif
