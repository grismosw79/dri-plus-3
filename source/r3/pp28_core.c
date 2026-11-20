#include "pp28_core.h"
#include <limits.h>

int pp28_grade(const PP28Evidence *v,size_t n,size_t at,
               const int32_t cuts[13],PP28Grade *out) {
    if(!v||!cuts||!out||!n||n>PP28_MAX_SCHOOLS||at>=n)return 0;
    for(int g=0;g<13;g++)
        if(cuts[g]<0 || cuts[g]>100000 || (g&&cuts[g]<cuts[g-1]))return 0;
    const PP28Evidence *a=v+at;
    int better=0,tied=0;
    for(size_t j=0;j<n;j++) {
        const PP28Evidence *b=v+j;
        if(b->history_points<0||b->projected_points<0||b->composite<0 ||
           b->history_points>1000000||b->projected_points>10000000||b->composite>100000000)return 0;
        if(b->composite>a->composite ||
           (b->composite==a->composite&&b->projected_points>a->projected_points))better++;
        if(b->composite==a->composite&&b->projected_points==a->projected_points)tied++;
    }
    if(!tied)return 0;
    int grade=PP28_UNPROVEN_GRADE;
    if(a->history_points||a->projected_points) {
        int rank_twice=2*better+tied-1;
        grade=12;
        for(int g=0;g<13;g++)if(rank_twice<=2*cuts[g]){grade=g;break;}
        if(grade>PP28_UNPROVEN_GRADE)grade=PP28_UNPROVEN_GRADE;
    }
    *out=(PP28Grade){grade,better,tied,a->composite,a->projected_points};
    return 1;
}

int pp28_history_plan(const uint32_t *old,size_t n,int first_annual,
                      uint32_t *next,size_t *changed) {
    if(!old||!next||!changed||n>PP28_MAX_HISTORY)return -1;
    *changed=0;
    for(size_t i=0;i<n;i++)next[i]=old[i];
    if(!first_annual||!n)return 0;
    unsigned mask=0;
    for(size_t i=0;i<n;i++) {
        if(old[i]==0)continue;
        if(old[i]<2||old[i]>4)return 0;
        mask|=1u<<old[i];
    }
    if(mask!=((1u<<2)|(1u<<3)|(1u<<4)))return 0;
    for(size_t i=0;i<n;i++)if(old[i]){next[i]=old[i]-1;(*changed)++;}
    return *changed?1:0;
}
