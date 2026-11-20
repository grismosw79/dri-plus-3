#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../portal_policy.h"

int main(void) {
 PortalQuality rooms[22]={0};
 for(int p=0;p<22;p++) {
  rooms[p].pos=p;rooms[p].ideal=4;rooms[p].starters=1;
  rooms[p].incoming_best=-1;
 }
 rooms[3].count=rooms[3].base_count=10; /* inherited, free */
 dri_portal_refresh_budget(rooms,10);
 assert(rooms[0].exception_states==(DRI_EXCEPTION_READY|1u));
 /* HS fills real seats first; inherited excess remains free. */
 rooms[5].count=rooms[5].base_count=4;rooms[5].hs_count=2;
 dri_portal_refresh_budget(rooms,10);
 assert(rooms[0].exception_states==(DRI_EXCEPTION_READY|1u));
 rooms[5].count++;rooms[5].base_count++;rooms[5].hs_count++;
 dri_portal_refresh_budget(rooms,10);
 assert(rooms[0].exception_states==(DRI_EXCEPTION_READY|(1u<<6)));
 rooms[6].count=rooms[6].base_count=5;rooms[6].hs_count=1;
 dri_portal_refresh_budget(rooms,10);
 assert(rooms[0].exception_states==(DRI_EXCEPTION_READY|(1u<<12)));
 assert(!dri_exception_step(rooms[0].exception_states,DRI_EXCEPTION_GENERAL));
 uint32_t s=rooms[0].exception_states;
 s=dri_exception_step(s,DRI_EXCEPTION_EMERGENCY);
 s=dri_exception_step(s,DRI_EXCEPTION_EMERGENCY);
 s=dri_exception_step(s,DRI_EXCEPTION_RB);
 assert(s==(1u<<17));
 assert(!dri_exception_step(s,7));
 /* An inherited surplus is not itself a permit for a third new HS extra. */
 rooms[3].count++;rooms[3].base_count++;rooms[3].hs_count++;
 dri_portal_refresh_budget(rooms,10);
 assert(rooms[0].exception_states==DRI_EXCEPTION_READY);
 puts("PASS: inherited baseline, HS vacancy, shared general places, and exact 2+2+1 ceiling");
 return 0;
}
