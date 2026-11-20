#ifndef DRI_CLASS34_POLICY_H
#define DRI_CLASS34_POLICY_H
#include "class34r2_plan.h"
/* A single additional intake seat can repair an actual developmental-room
   vacancy while the total class remains short. QB, specialists and LS keep
   their existing separate rules. The caller must still check candidate
   eligibility and charge the shared general allowance prospectively. */
static inline int dri_hs_vacancy_extra(int valid,int target,int secured,int total,
 int pos,int ideal,int annual,int hs,int projected) {
 /* Annual intake is independent of provisional total roster size. */
 (void)total;
 return valid && target>=15 && target<=25 && secured<target &&
  pos>=1 && pos<=18 && ideal>0 && annual>=0 && hs==annual && projected<ideal;
}
#endif
