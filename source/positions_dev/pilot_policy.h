#ifndef DRI_RM07_PILOT_POLICY_H
#define DRI_RM07_PILOT_POLICY_H
#include <stdint.h>
/* Pure policy kernel. Not wired to game writes in RM07. */
enum { RM_LT=5,RM_LG=6,RM_C=7,RM_RG=8,RM_RT=9,RM_LE=10,RM_RE=11,
       RM_LOLB=13,RM_ROLB=15,RM_FS=17,RM_SS=18,RM_K=19,RM_P=20 };
typedef struct { uint64_t id; int listed_position; int grade[3]; unsigned verified_mask; } RMCandidate;
typedef struct { int feasible; int selected[3]; int sum,weakest,chart_matches,label_matches; } RMUnitPlan;
int rm_family(int position);
int rm_compatible(int from,int to);
RMUnitPlan rm_plan(const RMCandidate *p,int count,const int *positions,int roles,const uint64_t *current);
/* Map by identity, never merely by slot number. Reject lost/duplicate identities.
   All five identities must exist in both archetypes, including unearned slots. */
int rm_map_abilities(const int old_identity[5],const int old_tier[5],const int new_identity[5],int result[5]);
enum { RM_DEV_HOLD=0,RM_DEV_DOWN=1,RM_DEV_NATIVE_UP=2,RM_DEV_INVALID=-1 };
typedef struct {
    int before,native_after,eligible,complete_evidence,protected_season,injury_resolved;
    int rate_basis_points; /* -1 = policy not selected, never default to a rate */
    uint64_t player_identity;unsigned season,policy_revision;
} RMDevInput;
typedef struct { int action,final_trait;unsigned draw; } RMDevDecision;
RMDevDecision rm_dev_decision(RMDevInput in);
#endif
