#ifndef DRI_CLASS31_POLICY_H
#define DRI_CLASS31_POLICY_H
#include <stdint.h>
/* Test31 normal annual intake. The approved real-class composition/ATH model
   resolves to ideal/4.5 for these role groups, with the empirical QB and WR
   frequencies retained above that replacement reference. Current vacancies
   and first-year roster imbalance do not define this permanent baseline.
   Fractional rates use a stable calendar phase: reloading cannot reroll them. */
static inline uint32_t class31_phase(unsigned team,unsigned group) {
 uint32_t x=(team+1u)*0x9e3779b9u+(group+1u)*0x85ebca6bu;
 x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;x^=x>>16;return x;
}
static inline int class31_annual(const int ideal[22],unsigned team,unsigned year,int annual[22]) {
 static const unsigned char lo[11]={0,1,2,3,4,5,10,13,16,19,20};
 static const unsigned char hi[11]={0,1,2,3,4,9,12,15,18,19,20};
 int total=0,ideal_total=0;
 for(int p=0;p<22;p++){annual[p]=0;if(ideal[p]<0 || ideal[p]>30)return -1;ideal_total+=ideal[p];}
 if(team>=138 || year>10000 || ideal_total<40 || ideal_total>110 || ideal[0]<1)return -1;
 for(unsigned g=0;g<11;g++) {
  int amount=0;for(int p=lo[g];p<=hi[g];p++)amount+=ideal[p];
  if(!amount)continue;
  uint64_t numerator=(uint64_t)amount*2,denominator=9;
  if(g==0){numerator=(uint64_t)amount*105;denominator=400;} /* 1.05 at QB ideal4. */
  if(g==3){numerator=(uint64_t)amount*17791;denominator=68400;} /* 355.82/1368 WR. */
  uint64_t phase=class31_phase(team,g)%denominator;
  uint64_t first=((uint64_t)year*numerator+phase)/denominator;
  int places=(int)((((uint64_t)year+1)*numerator+phase)/denominator-first);
  /* Smooth weighted round robin maps broad roles to exact scheme positions.
     It preserves each ideal's share over a complete cycle without clustering
     all OL/DB recruits in one position. The annual map never uses signings or
     candidate order, so every callback reconstructs the same class slots. */
  int schedule[150],balance[22]={0};
  if(amount>150)return -1;
  for(int i=0;i<amount;i++) {
   int best=-1;
   for(int p=lo[g];p<=hi[g];p++)if(ideal[p]){
    balance[p]+=ideal[p];if(best<0 || balance[p]>balance[best])best=p;
   }
   if(best<0)return -1;
   schedule[i]=best;balance[best]-=amount;
  }
  unsigned offset=class31_phase(team,g+19u)%(unsigned)amount;
  for(int i=0;i<places;i++)annual[schedule[(first+offset+(unsigned)i)%(unsigned)amount]]++;
  total+=places;
 }
 /* The actual 138-school census stays within15..25. Keep bounded behavior
    for other supported schemes: adjust non-specialist developmental roles,
    never manufacture LS or a role with zero configured ideal. */
 while(total<15){
  int best=-1;int64_t score=INT64_MIN;
  for(int p=1;p<19;p++)if(ideal[p]>0){
   int64_t value=(int64_t)ideal[p]*1000/(annual[p]+1);
   if(value>score){score=value;best=p;}
  }
  if(best<0)return -1;
  annual[best]++;total++;
 }
 while(total>25){
  int best=-1;int64_t score=INT64_MIN;
  for(int p=1;p<19;p++)if(annual[p]>0){
   int64_t value=(int64_t)annual[p]*1000/(ideal[p]>0?ideal[p]:1);
   if(value>score){score=value;best=p;}
  }
  if(best<0)return -1;
  annual[best]--;total--;
 }
 return total;
}
static inline int class31_remaining(const int annual[22],const int secured[22],int pos) {
 if(pos<0 || pos>=22)return 0;
 return annual[pos]>secured[pos]?annual[pos]-secured[pos]:0;
}
#endif
