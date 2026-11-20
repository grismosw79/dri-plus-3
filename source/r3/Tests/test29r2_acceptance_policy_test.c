/* Current R4/Test30 acceptance contract. The archived R2 fixture's material()
   and capacity() expectations predate R4 quality-depth permission and also
   fail against unmodified R5. Keep selection plus explicit ordinary-budget
   coverage below; test29r4_quality_depth_policy_test covers quality overflow. */
#define main test29_prior_policy_main
#include "test29_portal_policy_test.c"
#undef main
static void source_coexistence(void) {
 PortalQuality q[22]={0};int reserved[22]={0};
 /* Controlled ordinary-path boundary: prestige10 edge starter floor=80.
    A 79 improves the 76 incumbent by three but remains below that floor,
    so R4 quality permission cannot mask the existing shared-budget path. */
 q[10]=room(10,3,4,1,76,76);reserved[10]=1;
 dri_portal_set_hs_reservations(q,reserved,10);
 assert(q[10].count==3 && q[10].hs_reserved==1);
 assert(dri_quality_block_candidate(&q[10],10,76,76)==18);
 assert(!dri_portal_quality_depth_rank(&q[10],10,79));
 assert(!dri_quality_block_candidate(&q[10],10,79,79));
 assert(!dri_portal_seat_upgrade(&q[10],10,99,99,3));
 q[10].hs_count=1;assert(dri_quality_block_candidate(&q[10],10,79,79)==18);q[10].hs_count=0;
 PortalQuality multiple=room(10,2,4,1,76,76);multiple.hs_reserved=2;
 assert(dri_quality_block_candidate(&multiple,10,79,79)==18);
 dri_portal_project_add(&q[10],79);
 dri_portal_set_hs_reservations(q,reserved,10);
 assert(q[10].count==4 && q[10].hs_reserved==1);
 assert(dri_overage_full(&q[10]));
 assert(dri_quality_block_candidate(&q[10],10,76,76)==18);
 /* Existing R4 permission admits a prestige-qualified contributor; it
    does not mutate the HS reservation or spend the ordinary allowance. */
 assert(dri_portal_quality_depth_rank(&q[10],10,99)==1);
 assert(!dri_quality_block_candidate(&q[10],10,99,99));
 assert(q[10].hs_reserved==1 && q[10].count==4);
 /* Reserved HS + portal cost exactly one general exception, retained when
    the HS player actually signs, and reconstructed identically on a reload. */
 uint32_t state=q[0].exception_states;
 assert(state==(DRI_EXCEPTION_READY|(1u<<6)));
 dri_portal_project_add(&q[10],68);q[10].incoming[q[10].count-1]=0;
 q[10].base_count++;q[10].hs_count++;reserved[10]=0;
 dri_portal_set_hs_reservations(q,reserved,10);
 assert(q[10].count==5 && q[10].hs_reserved==0 && q[0].exception_states==state);
 assert(dri_quality_block_candidate(&q[10],10,76,76)==4);
 dri_portal_refresh_budget(q,10);assert(q[0].exception_states==state);
 /* One other general addition fits; a third does not. HS/portal cannot each
    independently spend a general place that the other has already reserved. */
 q[5]=room(5,3,3,1,75,75);q[6]=room(6,3,3,1,76,76);
 dri_portal_refresh_budget(q,10);
 assert(!dri_portal_quality_depth_rank(&q[5],10,78));
 assert(!dri_portal_quality_depth_rank(&q[6],10,79));
 assert(!dri_quality_block_candidate(&q[5],10,78,78));
 dri_portal_project_add(&q[5],78);dri_portal_refresh_budget(q,10);
 assert(dri_quality_block_candidate(&q[6],10,79,79)==8);
 q[10]=room(10,3,4,1,76,76);reserved[10]=1;
 q[6]=room(6,3,3,1,76,76);dri_portal_project_add(&q[6],79);
 dri_portal_set_hs_reservations(q,reserved,10);
 assert(dri_quality_block_candidate(&q[10],10,79,79)==8);
 /* Removing the actual paid HS reservation restores ordinary native room;
    no fictional player or spent allowance survives its withdrawal. */
 reserved[10]=0;dri_portal_set_hs_reservations(q,reserved,10);
 assert(!dri_quality_block_candidate(&q[10],10,76,76));
 puts("PASS ordinary sub-floor improvement and paid HS seat share existing 2+2+1 without duplicate spending; reservation survives signing and reload; R4 qualifying quality permission remains open");
}
int main(void){selection();source_coexistence();return 0;}
