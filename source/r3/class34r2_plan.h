#ifndef DRI_CLASS34R2_PLAN_H
#define DRI_CLASS34R2_PLAN_H
#include "test29_class.h"
/* The deterministic annual map is the ordinary intake goal. HS arrivals
   beyond that map already consume shared general allowance; they must not
   also erase an ordinary planned place. Reconstruct them after reload. */
static inline int class34r2_secured_extra(int valid,const int annual[22],const int hs[22]) {
 if(!valid)return 0;
 int extra=0;
 for(int p=1;p<=18;p++)if(hs[p]>annual[p])extra+=hs[p]-annual[p];
 return extra>2?2:extra;
}
/* This is class-plan capacity, not a new source of shared permission.
   Callers still prove native legality, positional room and general budget. */
static inline int class34r2_capacity(int valid,int base,int secured_extra,int pending_extra) {
 if(!valid || base<15 || base>25)return base;
 if(secured_extra<0)secured_extra=0;
 if(pending_extra<0)pending_extra=0;
 int extra=secured_extra+pending_extra;if(extra>2)extra=2;
 return base+extra>25?25:base+extra;
}
static inline int class34r2_plan(const Test29ClassInput *input,const int repair[22],
 int secured_extra,Test29ClassPlan *out) {
 Test29ClassInput in=*input;
 if(!in.annual_valid || in.annual_target<15 || in.annual_target>25) {
  test29_class_plan(&in,out);return 0;
 }
 int base=in.annual_target;
 int settled=class34r2_capacity(1,base,secured_extra,0);
 int allowance=in.general>0?in.general:0;if(allowance>2)allowance=2;
 /* Existing non-repair optional primaries keep their same allowance place.
    New capacity is dedicated to legal under-ideal repairs, not offered to
    unrelated quality exceptions or ordinary candidates. */
 for(int p=0;p<22;p++)if(in.protected_optional[p] && in.optional[p] &&
   in.candidates[p]>0 && !repair[p])allowance--;
 if(allowance<0)allowance=0;
 int limit=2-secured_extra;if(limit<0)limit=0;
 if(limit>allowance)limit=allowance;
 if(limit>25-settled)limit=25-settled;
 int chosen[22]={0},added=0;
 /* Preserve a prior paid/reserved repair first. Remaining choices use the
    same candidate utility as other optional places, with stable pos ties. */
 for(int p=1;p<=18 && added<limit;p++)if(repair[p] && in.optional[p] &&
   in.candidates[p]>0 && in.protected_optional[p]){chosen[p]=1;added++;}
 while(added<limit) {
  int best=-1,value=-1;
  for(int p=1;p<=18;p++)if(repair[p] && in.optional[p] && in.candidates[p]>0 && !chosen[p]) {
   int score=in.optional_value[p]+in.junior[p]*10+(in.secured[p]==0?25:0);
   if(score>value){value=score;best=p;}
  }
  if(best<0)break;
  chosen[best]=1;added++;
 }
 for(int p=1;p<=18;p++)if(chosen[p])in.protected_optional[p]=1;
 in.annual_target=settled+added;
 test29_class_plan(&in,out);
 return added;
}
#endif
