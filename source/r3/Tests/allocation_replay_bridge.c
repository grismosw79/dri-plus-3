#include <stdint.h>
#include <string.h>
#include "../portal_allocation.h"
int allocation_evaluate_current(const PortalQuality *q,int prestige,int ovr,int current,int base,PortalChoice *p) {
 int rank=0,reason=dri_quality_block_candidate(q,prestige,ovr,current);
 int bonus=base>0?dri_quality_bonus_candidate(q,prestige,ovr,current,&rank):0;
 p->ovr=ovr;p->raw_ovr=current;p->pos=q->pos;p->score=base>0 && !reason?base+bonus:0;
 p->native_base=base;p->plan_score=p->score;
 p->role=p->score>0?dri_portal_role_candidate(q,prestige,ovr,current):0;p->deficit=0;
 if(rank) p->deficit=dri_quality_floor[dri_group(prestige)][q->pos][rank-1]-4*(rank<=q->count?q->rating[rank-1]:0);
 return reason;
}
void allocation_order(PortalChoice *p,int n,int cap,const PortalQuality *rooms,int prestige){dri_portal_order(p,n,cap,rooms,prestige);}
int allocation_shortlist(int score,int role){return dri_portal_shortlist_score(score,role);}
int allocation_star(int ovr,int year){return dri_portal_star(ovr,year);}
int allocation_choice_size(void){return (int)sizeof(PortalChoice);}
int allocation_room_size(void){return (int)sizeof(PortalQuality);}

int allocation_evaluate(const PortalQuality *q,int prestige,int ovr,int base,PortalChoice *p) {return allocation_evaluate_current(q,prestige,ovr,ovr,base,p);}
int allocation_set_flex(PortalQuality rooms[22]) {return dri_portal_set_flex(rooms);}
int allocation_block(const PortalQuality *q,int prestige,int ovr,int current) {return dri_quality_block_candidate(q,prestige,ovr,current);}
void allocation_add(PortalQuality *q,int ovr) {dri_portal_project_add(q,ovr);}

void allocation_refresh(PortalQuality rooms[22],int prestige){dri_portal_refresh_budget(rooms,prestige);}
