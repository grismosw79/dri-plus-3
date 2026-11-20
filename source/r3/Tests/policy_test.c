#include <assert.h>
#include <stdio.h>
#include "policy.h"
int main(void) {
 for(int p=0;p<22;p++) for(int ideal=0;ideal<=30;ideal++) {
  int limit=dri_annual_limit(p,ideal);
  if((p==2 || p==21) && ideal==0) assert(limit==0);
  else {assert(limit>=1);assert(limit==(ideal<2?1:(ideal+1)/3));}
  if(p!=19 && p!=20) for(int c=0;c<12;c++)
   assert(dri_position_closed(p,ideal,limit,c,ideal,0)==(ideal==0 || c>=limit));
 }
 for(int own=0;own<9;own++) for(int other=0;other<9;other++) {
  int open=dri_specialist_open(own,other);
  assert(open==(own<2));
 }
 assert(dri_annual_limit(0,3)==1);assert(dri_annual_limit(3,9)==3);
 assert(dri_close_challenger(1,2,1,1,0,80,100));
 assert(!dri_close_challenger(1,2,1,1,0,79,100));
 assert(!dri_close_challenger(1,2,1,1,1,90,100));
 puts("PASS: rounding, closures, specialist coverage and bounds, challenger threshold");
}
