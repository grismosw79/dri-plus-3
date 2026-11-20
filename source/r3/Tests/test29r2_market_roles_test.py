"""Compile production R2 two-lane market and role selection. No live claim."""
from pathlib import Path
import os,subprocess,json
S=Path(__file__).resolve().parents[1];O=Path(os.environ.get('DRI_TEST_OUTPUT_DIR',S/'build/test29r2-market-roles'));O.mkdir(parents=True,exist_ok=True)
code=r'''
#include <assert.h>
#include <stdio.h>
#include "market29_core.h"
#include "portal_allocation.h"
#include "hs28_pairs.h"
static int bits(const uint64_t *a){int n=0;for(unsigned t=0;t<138;t++)n+=market29_has(a,t);return n;}
static void lanes(int portal){
 Market29Pressure m={0};
 for(unsigned t=0;t<30;t++){market29_bit(m.active,t,1);market29_bit(m.planned,t,1);m.fit[t]=500;}
 market29_bit(m.planned,31,1);market29_bit(m.primary,31,1);m.fit[31]=450;
 assert(market29_pressure(&m,31)==30 && market29_primary_pressure(&m,31)==0);
 market29_reconcile(&m,portal,19,2026);
 assert(market29_has(m.admitted,31) && !market29_has(m.rejected,31));
 assert(bits(m.backup_admitted)==(portal?7:11));
 /* Five or eight real primaries occupy the primary guideline; the total
    attention guideline does not let uncommittable backups consume it. */
 for(unsigned t=32;t<50;t++){market29_bit(m.planned,t,1);market29_bit(m.primary,t,1);m.fit[t]=500;}
 market29_reconcile(&m,portal,19,2026);assert(bits(m.admitted)==(portal?5:8));
}
int main(void){
 lanes(0);lanes(1);
 assert(!market29_recovery_capacity(0,100,10,0,1,0)); /* Two best-fit full boards cannot monopolize nominations. */
 assert(!market29_recovery_capacity(50,100,10,0,0,0)); /* No usable opening. */
 assert(market29_recovery_capacity(50,10,10,0,1,0));
 assert(!market29_recovery_capacity(50,10,10,1,2,0)); /* Same cohort budget cannot buy a second nomination. */
 assert(!market29_recovery_capacity(50,100,10,1,1,1)); /* Same position seat cannot be reused. */
 int v[22]={1},o[22]={0};
 Hs28Choice h[2]={{.rid=1,.native_score=500,.ovr=78,.legal=1,.viable=1,.market_roles=1,.market_known=1,.market_planned=1,.market_primary=0},
 {.rid=2,.native_score=480,.ovr=77,.legal=1,.viable=1,.market_roles=1,.market_known=1,.market_planned=1,.market_primary=1}};
 hs28_plan(h,2,v,o,0,50);assert(h[1].choice==1 && h[0].choice==2);
 hs28_plan(h,1,v,o,0,50);assert(!h[0].selected); /* Narrowed live pool cannot promote backup. */
 h[0].market_known=0;h[0].backup_penalty=45;
 hs28_plan(h,1,v,o,0,50);assert(h[0].choice==1); /* Backups never suppress a real primary. */
 Hs28Choice backup_choices[3]={{.rid=10,.native_score=600,.ovr=85,.legal=1,.viable=1,.market_roles=1},
 {.rid=11,.native_score=500,.ovr=78,.legal=1,.viable=1,.market_roles=1,.backup_penalty=45},
 {.rid=12,.native_score=480,.ovr=77,.legal=1,.viable=1,.market_roles=1}};
 hs28_plan(backup_choices,3,v,o,0,50);assert(backup_choices[0].choice==1 && backup_choices[2].choice==2 && !backup_choices[1].selected);
 backup_choices[2].ovr=60;backup_choices[2].native_score=200;
 hs28_plan(backup_choices,3,v,o,0,50);assert(backup_choices[1].choice==2); /* No close fallback, soft guide. */
 PortalQuality q[22]={0};q[0].pos=0;q[0].ideal=4;q[0].starters=1;q[0].count=3;
 for(int j=0;j<3;j++)q[0].rating[j]=65;
 PortalChoice p[2]={{.rid=1,.pos=0,.ovr=78,.raw_ovr=78,.score=500,.native_base=500,.race_ok=1,.market_known=1,.market_planned=1},
 {.rid=2,.pos=0,.ovr=77,.raw_ovr=77,.score=480,.native_base=480,.race_ok=1,.market_known=1,.market_planned=1,.market_primary=1}};
 dri_portal_order(p,2,50,q,5);assert(p[0].rid==2 && p[0].round==1 && p[1].round==2);
 PortalChoice backup=p[1];dri_portal_order(&backup,1,50,q,5);assert(!backup.selected);
 backup.market_protected=1;backup.offered=1;dri_portal_order(&backup,1,50,q,5);assert(backup.selected);
 backup.market_protected=0;backup.market_primary=1;backup.crowd_penalty=45;
 int raw=dri_portal_value_unpenalized(&backup,&q[0],5);
 assert(dri_portal_value(&backup,&q[0],5)==raw*55/100); /* New offer cannot erase frozen pressure. */
 puts("PASS primary/backup pressure separation, narrowed-pool role continuity, established offers and newly offered identity");
}
'''
(O/'r2_market_roles.c').write_text(code)
subprocess.run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Wno-unused-function','-I'+str(S),str(O/'r2_market_roles.c'),'-o',str(O/'r2_market_roles')],check=True)
r=subprocess.run([str(O/'r2_market_roles')],capture_output=True,text=True);assert r.returncode==0,r.stdout+r.stderr
print(r.stdout);(O/'test29r2_market_roles_results.json').write_text(json.dumps({'status':'PASS','method':'compiled production C market and role policies','live_game_verified':False,'checks':['primary8HS/5portal versus total12HS/8portal','30conditional offers cannot suppress the sole primary','backup cannot silently become primary in narrowed native pool','newly created offer does not gain frozen protection','existing frozen offer remains eligible']},indent=2)+'\n')
