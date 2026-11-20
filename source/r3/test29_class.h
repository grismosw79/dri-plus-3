#ifndef DRI_TEST29_CLASS_H
#define DRI_TEST29_CLASS_H
#include <stdint.h>
#include "class31_policy.h"
/* Annual HS source allocation. This is a pursuit plan, not roster capacity,
   an entitlement to scholarships, or a count of executed native actions. */
typedef struct {
 int ideal[22],returning[22],secured[22],junior[22];
 int vacancy[22],optional[22],candidates[22];
 int protected_vacancy[22],protected_optional[22];
 int optional_value[22]; /* Best eligible candidate utility, not free room. */
 int general,academy;
 int annual_valid,annual_target;
} Test29ClassInput;
typedef struct {
 int target,secured,replacement,remaining,available,planned,shortfall;
 int replacement_exception,protected_over_target,general;
 int vacancy[22],optional[22];
} Test29ClassPlan;
static inline int test29_class_academy(unsigned tid) {return tid==0 || tid==7 || tid==55;}
static inline int test29_class_floor(unsigned tid) {return test29_class_academy(tid)?20:15;}
static inline void test29_class_plan(const Test29ClassInput *in,Test29ClassPlan *out) {
 *out=(Test29ClassPlan){0};
 int candidates[22]={0},optional_used=0;
 for(int p=0;p<22;p++) {
  out->secured+=in->secured[p];
  if(in->ideal[p]>in->returning[p])out->replacement+=in->ideal[p]-in->returning[p];
  candidates[p]=in->candidates[p]>0?in->candidates[p]:0;
  int v=in->vacancy[p]>0?in->vacancy[p]:0;
  if(v>candidates[p])v=candidates[p];
  out->available+=v;
 }
 /* Center at20 while adapting to the replacement burden. Academies cannot
    use portal arrivals, so their genuine replacement burden is all HS/JUCO. */
 int target=in->annual_valid?in->annual_target:in->academy?out->replacement:20+(out->replacement-20)/2;
 if(!in->annual_valid && in->academy && target<20)target=20;
 if(target<15)target=15;
 if(target>25)target=25;
 if(!in->annual_valid && in->academy && out->replacement>25){target=out->replacement;out->replacement_exception=1;}
 out->target=target;out->remaining=target>out->secured?target-out->secured:0;
 int general=in->general>0?in->general:0;if(general>2)general=2;
 int possible_optional=0;
 for(int p=0;p<22;p++)if(in->optional[p] && candidates[p]>0)possible_optional++;
 out->available+=possible_optional<general?possible_optional:general;
 /* Do not withdraw a credible already funded primary solely to satisfy a
    class target. Its normal eligibility and shared-budget gates still apply. */
 for(int p=0;p<22;p++) {
  int v=in->protected_vacancy[p];if(v<0)v=0;
  if(v>in->vacancy[p])v=in->vacancy[p];
  if(v>candidates[p])v=candidates[p];
  out->vacancy[p]=v;out->planned+=v;candidates[p]-=v;
  if(in->protected_optional[p] && in->optional[p] && candidates[p]>0 && optional_used<general) {
   out->optional[p]=1;optional_used++;out->planned++;candidates[p]--;
  }
 }
 out->protected_over_target=out->planned>out->remaining?out->planned-out->remaining:0;
 /* Slots are the Test31 normal annual plan when annual_valid, otherwise the
    legacy diagnostic input. Current cohort exposure prioritizes these same
    authorized slots; it cannot enlarge the annual source plan. */
 while(out->planned<out->remaining) {
  int best=-1,score=-1;
  for(int p=0;p<22;p++)if(candidates[p]>0 && out->vacancy[p]<in->vacancy[p]) {
   int need=in->vacancy[p]-out->vacancy[p],ideal=in->ideal[p]>0?in->ideal[p]:1;
   int value=need*1000/ideal+in->junior[p]*10+(in->secured[p]==0?25:0)+((p==19 || p==20)?40:0);
   if(value>score){score=value;best=p;}
  }
  if(best<0)break;
  out->vacancy[best]++;out->planned++;candidates[best]--;
 }
 while(out->planned<out->remaining && optional_used<general) {
  int best=-1,score=-1;
  for(int p=0;p<22;p++)if(in->optional[p] && !out->optional[p] && candidates[p]>0) {
   int value=in->optional_value[p]+in->junior[p]*10+(in->secured[p]==0?25:0);
   if(value>score){score=value;best=p;}
  }
  if(best<0)break;
  out->optional[best]=1;out->planned++;optional_used++;candidates[best]--;
 }
 out->general=optional_used;
 out->shortfall=out->remaining>out->planned?out->remaining-out->planned:0;
}
/* A context-specific preference among native-positive, legal choices. Native
   score/influence/eligibility are untouched. Stars are native0..4. */
static inline int test29_academy_preference(int academy,int stars,int rank,int points,int leader,int need,int base) {
 if(!academy || !need || base<=0 || stars<0 || stars>4)return 0;
 int competitive=(rank>0 && rank<=5) || (leader>0 && (long long)points*4>=(long long)leader*3);
 if(stars==1 || stars==2)return competitive?20:10;
 /* Test30 permits four-star HS without a special academy selection bonus. */
 return 0;
}
/* Stable native-index permutation within each existing stage bucket. Only
   traversal order changes; original score totals and candidate bytes remain. */
static inline int test29_source_prioritize(int n,uint64_t *indices,const int *bucket,const int *priority) {
 if(n<0 || n>128)return -1;
 int seen[128]={0};uint64_t original[128];
 for(int i=0;i<n;i++) {
  if(indices[i]>=(uint64_t)n || seen[indices[i]]++ || bucket[indices[i]]<0 || bucket[indices[i]]>1)return -1;
  original[i]=indices[i];
 }
 for(int b=0;b<2;b++) {
  int positions[128],count=0,next=0;uint64_t ordered[128];
  for(int i=0;i<n;i++)if(bucket[original[i]]==b)positions[count++]=i;
  for(int first=1;first>=0;first--)for(int i=0;i<count;i++) {
   uint64_t k=original[positions[i]];
   if(!!priority[k]==first)ordered[next++]=k;
  }
  for(int i=0;i<count;i++)indices[positions[i]]=ordered[i];
 }
 int changed=0;for(int i=0;i<n;i++)changed+=indices[i]!=original[i];return changed;
}
#endif
