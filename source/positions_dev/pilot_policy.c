#include "pilot_policy.h"
#include <stddef.h>
int rm_family(int p) {
    switch(p){case RM_LT:case RM_RT:return 0;case RM_LG:case RM_C:case RM_RG:return 1;
    case RM_LE:case RM_RE:return 2;case RM_LOLB:case RM_ROLB:return 3;
    case RM_FS:case RM_SS:return 4;case RM_K:case RM_P:return 5;default:return -1;}
}
int rm_compatible(int a,int b){int f=rm_family(a);return f>=0 && f==rm_family(b);}
static int better(RMUnitPlan a,RMUnitPlan b) {
    if(!b.feasible)return 1;
    if(a.sum!=b.sum)return a.sum>b.sum;
    if(a.weakest!=b.weakest)return a.weakest>b.weakest;
    if(a.chart_matches!=b.chart_matches)return a.chart_matches>b.chart_matches;
    if(a.label_matches!=b.label_matches)return a.label_matches>b.label_matches;
    return 0;
}
static void visit(const RMCandidate*p,int n,const int*pos,int roles,const uint64_t*cur,int depth,RMUnitPlan path,RMUnitPlan*best) {
    if(depth==roles){path.feasible=1;if(better(path,*best))*best=path;return;}
    for(int i=0;i<n;i++) {
        if(!p[i].id || !rm_compatible(p[i].listed_position,pos[depth]) || !(p[i].verified_mask&(1u<<depth)) || p[i].grade[depth]<0 || p[i].grade[depth]>99)continue;
        int duplicate=0;
        for(int j=0;j<depth;j++)if(p[path.selected[j]].id==p[i].id && rm_family(pos[depth])!=5)duplicate=1;
        if(duplicate)continue;
        RMUnitPlan next=path;next.selected[depth]=i;next.sum+=p[i].grade[depth];
        if(p[i].grade[depth]<next.weakest)next.weakest=p[i].grade[depth];
        next.chart_matches+=cur && cur[depth]==p[i].id;next.label_matches+=p[i].listed_position==pos[depth];
        visit(p,n,pos,roles,cur,depth+1,next,best);
    }
}
RMUnitPlan rm_plan(const RMCandidate*p,int count,const int*positions,int roles,const uint64_t*current) {
    RMUnitPlan best={0,{-1,-1,-1},0,100,0,0},start=best;
    if(!p || !positions || count<1 || count>180 || roles<2 || roles>3)return best;
    int family=rm_family(positions[0]);if(family<0 || roles!=(family==1?3:2))return best;
    for(int i=0;i<roles;i++){if(rm_family(positions[i])!=family)return best;for(int j=0;j<i;j++)if(positions[i]==positions[j])return best;}
    visit(p,count,positions,roles,current,0,start,&best);return best;
}
int rm_map_abilities(const int old[5],const int tiers[5],const int dest[5],int result[5]) {
    if(!old || !tiers || !dest || !result)return 0;int tmp[5];
    for(int i=0;i<5;i++) {
        int match=-1;
        for(int j=0;j<5;j++) {
            if(tiers[j]<0 || tiers[j]>4)return 0;
            if(old[j]==dest[i]){if(match>=0)return 0;match=j;}
            for(int k=0;k<j;k++)if(dest[j]==dest[k])return 0;
        }
        if(match<0)return 0;tmp[i]=tiers[match];
    }
    for(int i=0;i<5;i++)result[i]=tmp[i];return 1;
}
static uint64_t mix(uint64_t x) {
    x=(x^(x>>30))*UINT64_C(0xbf58476d1ce4e5b9);x=(x^(x>>27))*UINT64_C(0x94d049bb133111eb);return x^(x>>31);
}
RMDevDecision rm_dev_decision(RMDevInput in) {
    RMDevDecision out={RM_DEV_HOLD,in.native_after,0};
    if(in.before<0 || in.before>3 || in.native_after<0 || in.native_after>3 ||
       in.native_after<in.before || in.native_after>in.before+1 || in.rate_basis_points < -1 || in.rate_basis_points>10000) {
        out.action=RM_DEV_INVALID;return out;
    }
    /* Proposed conflict policy: preserve native upgrade; never upgrade then drop.
       This offline policy has not been selected for live release. */
    if(in.native_after>in.before){out.action=RM_DEV_NATIVE_UP;return out;}
    if(!in.before || !in.player_identity || !in.eligible || !in.complete_evidence ||
       in.protected_season || !in.injury_resolved || in.rate_basis_points<=0)return out;
    uint64_t seed=in.player_identity^((uint64_t)in.season<<32)^((uint64_t)in.policy_revision<<48);
    out.draw=(unsigned)(mix(seed)%10000);
    if(out.draw<(unsigned)in.rate_basis_points){out.final_trait=in.before-1;out.action=RM_DEV_DOWN;}
    return out;
}
