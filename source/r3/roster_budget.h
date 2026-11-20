#ifndef DRI_ROSTER_BUDGET_H
#define DRI_ROSTER_BUDGET_H
/* General discretionary allowance. Test28R5 reconstructs new HS/portal
   overages in portal_policy.h; inherited surpluses do not spend it. Raw
   positional excess remains useful for the separate 83/84-to-85 flex rule.
   The legacy scalar helper is retained for historical policy/replay tests. */
#define DRI_TEAM_OPTIONAL_LIMIT 2
static inline int dri_team_excess(const int count[22],const int ideal[22]) {
 int used=0;
 for(int p=0;p<22;p++)if(count[p]>ideal[p])used+=count[p]-ideal[p];
 return used;
}
static inline int dri_team_optional_block(int count,int ideal,int excess) {
 return ideal>0 && count>=ideal && excess>=DRI_TEAM_OPTIONAL_LIMIT;
}
#endif
