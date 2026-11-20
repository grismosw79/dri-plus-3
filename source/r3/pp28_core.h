#ifndef DRI_PP28_CORE_H
#define DRI_PP28_CORE_H
#include <stddef.h>
#include <stdint.h>
#define PP28_GROUPS 10
#define PP28_MAX_SCHOOLS 256
#define PP28_MAX_HISTORY 4096
#define PP28_UNPROVEN_GRADE 7 /* C */
typedef struct {
    int32_t history_points, projected_points, composite;
} PP28Evidence;
typedef struct {
    int grade, better, tied;
    int32_t composite, projected_points;
} PP28Grade;
/* Native ProPotential uses lower-is-better zero-based ordinal rank cutoffs.
   Exact ties receive their midpoint rank. Positive evidence has the same C
   floor as no evidence, preserving score monotonicity. No player sort occurs. */
int pp28_grade(const PP28Evidence *values,size_t count,size_t index,
               const int32_t cutoffs[13],PP28Grade *out);
/* First annual repair: complete shape only. Entries at 0 are current draft
   results awaiting native aging and are left unchanged. Other active entries
   must contain each of 2,3,4 and no other year. Actual lifecycle guard is
   supplied by the native adapter, not inferred from this history alone. */
int pp28_history_plan(const uint32_t *old_years,size_t count,int first_annual,
                      uint32_t *new_years,size_t *changed);
#endif
