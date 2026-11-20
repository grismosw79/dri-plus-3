#include <assert.h>
#include <stdio.h>
#include "../policy.h"
int main(void) {
 /* A four-player room: one returner plus one secured recruit still leaves two
    openings even though its one-player normal annual allowance is exhausted. */
 assert(dri_hs_remaining(0,4,1,1,2,0)==2);
 assert(dri_hs_remaining(0,4,1,2,3,0)==1);
 assert(dri_hs_remaining(0,4,1,3,4,0)==0);
 /* Ordinary annual developmental recruiting remains available in a full room. */
 assert(dri_hs_remaining(3,9,3,1,9,0)==2);
 assert(dri_hs_remaining(3,9,3,3,9,0)==0);
 /* Two of EACH specialist: excess kickers never hide a missing punter. */
 assert(dri_hs_remaining(19,2,1,1,1,0)==1);
 assert(dri_hs_remaining(20,2,1,0,0,0)==2);
 assert(dri_hs_remaining(19,2,1,0,2,0)==0);
 assert(dri_specialist_open(1,3));assert(!dri_specialist_open(2,0));
 /* Five-star guard applies despite an annual or vacancy allowance. */
 assert(dri_hs_candidate_block(9,3,1,1,1,0,4,0,1,75,0)==2);
 assert(dri_hs_candidate_block(9,3,1,0,1,0,4,0,0,75,0)==0);
 /* Another four-star needs an actual distinct role. A lower-star class member
    can still use the ordinary developmental allowance at the same count. */
 assert(dri_hs_candidate_block(3,9,3,1,9,0,3,1,0,75,0)==3);
 assert(dri_hs_candidate_block(3,9,3,1,8,0,3,1,0,75,0)==0);
 assert(dri_hs_candidate_block(3,9,3,1,9,0,2,1,0,70,0)==0);
 /* HS/JUCO QB admission sees elite returners and portal arrivals too. */
 assert(dri_hs_candidate_block(0,4,1,0,1,0,3,0,0,85,1)==4);
 assert(dri_hs_candidate_block(0,4,1,0,1,0,3,0,0,84,1)==0);
 assert(dri_hs_candidate_block(0,4,1,0,1,0,3,0,0,85,0)==0);
 /* A flex place is exactly one actual opening, not a permanent raised ideal. */
 assert(dri_hs_remaining(3,9,3,3,9,1)==1);
 assert(dri_hs_remaining(3,9,3,4,10,1)==0);
 for(int p=0;p<22;p++) {
  assert(dri_hs_remaining(p,0,1,0,0,1)==0);
  for(int ideal=1;ideal<=30;ideal++)for(int count=0;count<=45;count++)for(int commits=0;commits<=20;commits++) {
   int n=dri_hs_remaining(p,ideal,dri_annual_limit(p,ideal),commits,count,0);
   assert(n>=0);
   if(p==19 || p==20)assert(n==(count<2?2-count:0));
   else if(commits>=dri_annual_limit(p,ideal))assert(n==(count<ideal?ideal-count:0));
  }
 }
 puts("PASS: HS/JUCO vacancy catch-up, two specialists each, elite guards, bounded flex and annual balance");
 return 0;
}
