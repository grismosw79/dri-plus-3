#include "../pp28_core.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static const int32_t cuts[13]={5,10,15,25,35,45,55,65,75,90,105,120,250};
int main(void) {
    PP28Evidence v[138];PP28Grade g;
    memset(v,0,sizeof(v));
    for(size_t i=0;i<138;i++){assert(pp28_grade(v,138,i,cuts,&g));assert(g.grade==7&&g.tied==138);}
    /* Seven equal leaders span rank0..6: midpoint3 is A+ for every leader. */
    for(int i=0;i<7;i++)v[i]=(PP28Evidence){10,0,700};
    for(int i=0;i<7;i++){assert(pp28_grade(v,138,i,cuts,&g));assert(g.grade==0&&g.better==0&&g.tied==7);}
    for(int i=7;i<138;i++){assert(pp28_grade(v,138,i,cuts,&g));assert(g.grade==7);}
    /* Twenty-one stronger programs; tie at21..31 has midpoint26, exactly B. */
    for(int i=0;i<138;i++)v[i]=(PP28Evidence){1,0,70};
    for(int i=0;i<21;i++)v[i]=(PP28Evidence){3,0,210};
    for(int i=21;i<32;i++)v[i]=(PP28Evidence){2,0,140};
    for(int i=21;i<32;i++){assert(pp28_grade(v,138,i,cuts,&g));assert(g.grade==4&&g.better==21&&g.tied==11);}
    /* Native secondary projection ordering remains authoritative. */
    v[21].projected_points=1;
    assert(pp28_grade(v,138,21,cuts,&g)&&g.grade==3&&g.tied==1);
    /* Floor and input validation. */
    for(int i=0;i<138;i++)v[i]=(PP28Evidence){138-i,0,(138-i)*70};
    assert(pp28_grade(v,138,137,cuts,&g)&&g.grade==7);
    v[0].composite=-1;assert(!pp28_grade(v,138,137,cuts,&g));
    assert(!pp28_grade(v,0,0,cuts,&g));
    uint32_t old[]={0,0,2,2,3,4},next[6];size_t changed;
    assert(pp28_history_plan(old,6,1,next,&changed)==1&&changed==4);
    assert(next[0]==0&&next[1]==0&&next[2]==1&&next[4]==2&&next[5]==3);
    assert(pp28_history_plan(next,6,1,old,&changed)==0&&changed==0);
    uint32_t later[]={1,2,3,4};uint32_t later_out[4];
    assert(pp28_history_plan(later,4,1,later_out,&changed)==0);
    uint32_t malformed[]={2,3,4,5};
    assert(pp28_history_plan(malformed,4,1,later_out,&changed)==0);
    uint32_t original[]={2,3,4};uint32_t nochange[3];
    assert(pp28_history_plan(original,3,0,nochange,&changed)==0&&!memcmp(original,nochange,sizeof(original)));
    puts("PASS PP28 exact ties, midpoint boundaries, positive C floor, secondary sort, guarded history plan and idempotence");
    return 0;
}
