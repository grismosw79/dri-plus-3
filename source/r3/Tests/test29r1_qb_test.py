"""Existing 2+2+1 allocation with material QB1 protection, pure actual C policy."""
from pathlib import Path
import subprocess,os,json
root=Path(__file__).resolve().parents[1];out=Path(os.environ['DRI_TEST_OUTPUT_DIR'])
src=out/'qb.c';src.write_text('''
#include <assert.h>
#include "portal_policy.h"
int main(void){
 PortalQuality rooms[22]={0};
 for(int p=0;p<22;p++){rooms[p].pos=p;rooms[p].ideal=4;rooms[p].starters=1;rooms[p].incoming_best=-1;}
 PortalQuality *q=&rooms[0];q->base_count=q->count=4;q->rating[0]=82;q->rating[1]=75;q->rating[2]=73;q->rating[3]=72;
 /* Two unrelated general-only HS extras spend both G allowances. */
 for(int p=5;p<=6;p++){rooms[p].count=rooms[p].base_count=5;rooms[p].hs_count=1;}
 dri_portal_refresh_budget(rooms,9);
 assert(!dri_exception_step(q->exception_states,DRI_EXCEPTION_GENERAL));
 assert(dri_quality_block_candidate(q,9,91,89)==0);
 assert(dri_portal_exception_mask(q,9,91)&DRI_EXCEPTION_EMERGENCY);
 assert(!(dri_portal_exception_mask(q,9,86)&DRI_EXCEPTION_EMERGENCY));
 q->rating[0]=85;assert(dri_quality_block_candidate(q,9,91,89)==2);
 q->rating[0]=82;q->incoming_best=88;assert(!(dri_portal_exception_mask(q,9,91)&DRI_EXCEPTION_EMERGENCY));
 q->incoming_best=-1;q->exception_states=DRI_EXCEPTION_READY|(1u<<16); /* G2 E2 R0 */
 assert(dri_quality_block_candidate(q,9,91,89)==8);
 return 0;}
''')
subprocess.run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror','-Wno-unused-function','-I'+str(root),str(src),'-o',str(out/'qb')],check=True)
subprocess.run([str(out/'qb')],check=True)
(out/'r1_qb.json').write_text(json.dumps({'TN_shape_2G_spent_material_QB_allowed':True,'existing_2E_limit_preserved':True,'elite_QB_stacking_guard':True,'reload_state_reconstructed':True,'live_game_verified':False},indent=2))
print('PASS material QB uses existing emergency allowance; no sixth slot or elite QB stack')
