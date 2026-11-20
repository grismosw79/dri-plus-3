#ifndef DRI_PORTAL_ALLOCATION_H
#define DRI_PORTAL_ALLOCATION_H
#include <stdint.h>
#include <string.h>
#include "portal_policy.h"
#include "portal_pursuit.h"
#include "rc4_qb_policy.h"
#ifndef DRI_RC4_ELITE
#define DRI_RC4_ELITE(rec) 0
#endif
#ifndef DRI_RC4_CHANGED
#define DRI_RC4_CHANGED(need,old,nv,ov,val) ((void)0)
#endif
/* Roles describe the opening, not the candidate's absolute OVR. */
static int dri_portal_role_candidate(const PortalQuality *q,int prestige,int ovr,int current) {
 if(dri_quality_block_candidate(q,prestige,ovr,current))return 0;
 int rank=dri_portal_value_rank(q,prestige,ovr,current);
 if(rank)return rank<=q->starters?1:2;
 if(q->count<q->starters && q->count<q->ideal)return 1;
 if(q->count<dri_portal_useful_depth(q) && q->count<q->ideal)return 2;
 return 3;
}
static inline int dri_portal_role(const PortalQuality *q,int prestige,int ovr) {return dri_portal_role_candidate(q,prestige,ovr,ovr);}
static int dri_portal_shortlist_score(int score,int role) {
 return score>0 && role>=1 && role<=3?(4-role)*1000000+score:0;
}
typedef struct {
 int pos,role,score,deficit,existing,round,selected,index;
 unsigned rid;uintptr_t rec;
 int ovr,slot,priority;
 /* Internal planning metadata only; never changes either native candidate ABI.
    score is the immutable input used by native stage-budget conservation. */
 int native_base,plan_score,offered,influence,leader,commit_score,stage,race_ok;
 int raw_ovr;
 /* Test29 market metadata changes selection only; immutable native scores
    and native stage totals remain available for exact funding conservation. */
 int crowd_penalty,crowd_defer,crowd_admitted,coverage_priority;
 int backup_penalty,market_known,market_planned,market_primary,market_protected;
} PortalChoice;
typedef struct {
 int pos,role,rank,deficit,min_ovr,vacancy,ordinal,first;
 int flex;
} PortalOpening;
static void dri_portal_sort_openings(PortalOpening *out,int n) {
 for(int j=1;j<n;j++) {
  PortalOpening x=out[j];int k=j;
  while(k>0) {
   PortalOpening *y=&out[k-1];int before;
   if(x.vacancy!=y->vacancy)before=x.vacancy>y->vacancy;
   else if(x.ordinal!=y->ordinal)before=x.ordinal<y->ordinal;
   else if(x.role!=y->role)before=x.role<y->role;
   else if(x.deficit!=y->deficit)before=x.deficit>y->deficit;
   else before=x.pos<y->pos;
   if(!before)break;
   out[k]=*y;k--;
  }
  out[k]=x;
 }
}
static int dri_portal_fit_before(const PortalChoice *a,const PortalChoice *b) {
 if(a->score!=b->score)return a->score>b->score;
 if(a->existing!=b->existing)return a->existing>b->existing;
 return a->rid<b->rid;
}
/* An existing name alone earns no protection. The offer must still be legal,
   competitive, and have progressed at least halfway to its base commit score.
   These are selection criteria; no influence or commitment threshold changes. */
static int dri_portal_competitive(const PortalChoice *p) {
 return p->existing && p->offered && p->race_ok && p->stage>=0 && p->stage<5 &&
        p->native_base>0 && p->influence>0 && p->leader>0 && p->commit_score>0 &&
        (int64_t)p->influence*5>=(int64_t)p->leader*4 &&
        (int64_t)p->influence*2>=p->commit_score;
}
static int dri_portal_choice_pursuit_percent(const PortalChoice *p,const PortalQuality *q,int prestige) {
 /* Existing scholarships and nominated coverage retain their opportunity.
    H3 does not withdraw an offer to chase an unproven alternative. */
 if(p->offered || p->market_protected || p->coverage_priority)return 100;
 return dri_portal_pursuit_percent(q,prestige,p->ovr,p->raw_ovr);
}
static int dri_portal_value_unpenalized(const PortalChoice *p,const PortalQuality *q,int prestige) {
 if(p->native_base<=0 || dri_quality_block_candidate(q,prestige,p->ovr,p->raw_ovr))return 0;
 int rank=0,value=p->native_base+dri_quality_bonus_candidate(q,prestige,p->ovr,p->raw_ovr,&rank);
 /* Reprice an established pursuit using the school's actual relative progress.
    Fresh eligible names retain their ordinary value. A clearly trailing offer
    must compete with reachable alternatives instead of remaining primary on
    prestige/fit alone. Native score, influence and budget are unchanged. */
 if(p->leader>0 && !p->offered && p->commit_score>0) {
  if(!p->race_ok)return 0;
  int deficit=p->leader-p->influence;if(deficit<0)deficit=0;
  /* Seeded interest is not an offer. It is useful evidence of the ground a
     new pursuit must make up. A modest start remains reachable; an advanced
     rival lead costs more, relative to this recruit's native commit target. */
  value=(int)((int64_t)value*p->commit_score/(p->commit_score+deficit));
 }
 if(p->existing && p->offered && p->leader>0) {
  if(!p->race_ok)return 0;
  if((int64_t)p->influence*4<(int64_t)p->leader*3)
   value=(int)((int64_t)value*p->influence*4/((int64_t)p->leader*3));
 }
 return dri_portal_pursuit_scale(value,dri_portal_choice_pursuit_percent(p,q,prestige));
}
static int dri_portal_value(const PortalChoice *p,const PortalQuality *q,int prestige) {
 int value=dri_portal_value_unpenalized(p,q,prestige);
 int penalty=(p->market_protected || (!p->market_known && p->offered))?0:p->crowd_penalty;
 if(penalty<0)penalty=0;
 if(penalty>45)penalty=45;
 return (int)((int64_t)value*(100-penalty)/100);
}
/* Deferral is a local alternative choice after the global market settles.
   An offered race or accepted frozen proposal is never dropped merely because
   every other school saw the same crowd. Alternatives must fit this opening,
   remain legal after other primary reservations, and be close in fit/quality. */
static int dri_portal_crowd_defer(const PortalChoice *p,const PortalChoice *a,int n,
                                 const PortalQuality *q,int prestige,int minimum,int flex,int opening) {
 if(p->market_protected || (!p->market_known && p->offered) || p->crowd_admitted || !p->crowd_defer)return 0;
 int own=dri_portal_value_unpenalized(p,q,prestige);if(own<=0)return 0;
 for(int j=0;j<n;j++) {
  const PortalChoice *other=&a[j];
  if(other==p || (other->priority && !(other->round==1 && other->slot==opening)) ||
     other->pos!=p->pos || other->score<=0 ||
     other->ovr<minimum || other->raw_ovr<p->raw_ovr-3 ||
     (other->market_known && (!other->market_planned || !other->market_primary) && !other->market_protected) ||
     (other->crowd_defer && !other->market_protected && !other->crowd_admitted))continue;
  if(flex && !dri_portal_flex_candidate(q,other->ovr))continue;
  int value=dri_portal_value_unpenalized(other,q,prestige);
  if(value>0 && (int64_t)value*100>=(int64_t)own*85)return 1;
 }
 return 0;
}
static int dri_portal_backup_defer(const PortalChoice *p,const PortalChoice *a,int n,
 const PortalQuality *q,int prestige,int minimum,int flex) {
 if(p->market_protected || p->backup_penalty<45)return 0;
 int own=dri_portal_value_unpenalized(p,q,prestige);
 for(int j=0;j<n;j++) {
  const PortalChoice *other=&a[j];
  if(other==p || other->priority || other->pos!=p->pos || other->score<=0 ||
    other->ovr<minimum || other->raw_ovr<p->raw_ovr-3 || other->backup_penalty>=45 ||
    (other->market_known && !other->market_protected && !other->market_planned) ||
    (flex && !dri_portal_flex_candidate(q,other->ovr)))continue;
  int value=dri_portal_value_unpenalized(other,q,prestige);
  if(value>0 && (int64_t)value*100>=(int64_t)own*85)return 1;
 }
 return 0;
}
/* The role of a selected addition is its place in the revised depth chart.
   A lower hypothetical arrival must not turn a better backup into "depth"
   merely because the admission policy protects incoming quality slots. */
static int dri_portal_plan_role_candidate(const PortalQuality *q,int prestige,int ovr,int current) {
 if(dri_quality_block_candidate(q,prestige,ovr,current))return 0;
 int rank=0;while(rank<q->count && q->rating[rank]>ovr)rank++;
 return rank<q->starters?1:rank<dri_portal_useful_depth(q)?2:3;
}
static inline int dri_portal_plan_role(const PortalQuality *q,int prestige,int ovr) {return dri_portal_plan_role_candidate(q,prestige,ovr,ovr);}
static int dri_portal_value_before(const PortalChoice *a,int av,const PortalChoice *b,int bv,int protect) {
 if(protect) {
  int64_t ap=(int64_t)a->influence*b->commit_score,bp=(int64_t)b->influence*a->commit_score;
  if(ap!=bp)return ap>bp;
 }
 if(av!=bv)return av>bv;
 if(a->existing!=b->existing)return a->existing>b->existing;
 return a->rid<b->rid;
}
static void dri_portal_project_add_candidate(PortalQuality *q,int ovr,int raw) {
 int k=q->count++;
 while(k>0 && q->rating[k-1]<=ovr) {
  q->rating[k]=q->rating[k-1];q->incoming[k]=q->incoming[k-1];k--;
 }
 q->rating[k]=ovr;q->incoming[k]=1;q->flex_allowed=0;
 if(ovr>q->incoming_best)q->incoming_best=ovr;
 dri_qb_observe(q,raw,ovr,0,0);
}
static inline void dri_portal_project_add(PortalQuality *q,int ovr) {dri_portal_project_add_candidate(q,ovr,ovr);}
/* Shared forecast valuation of the final plan. Same-slot fallbacks replace
   their primary; different selected primaries coexist. Never count oneself
   or an unselected/conditional pursuit as a secured arrival. No room gate is
   changed here and no native record is written. */
static int dri_portal_planned_pursuit_percent(const PortalChoice *p,const PortalChoice *pool,int n,
                                            const PortalQuality rooms[22],int prestige) {
 if(p->offered || p->market_protected || p->coverage_priority)return 100;
 PortalQuality q=rooms[p->pos];
 for(int j=0;j<n && q.count<192;j++) {
  const PortalChoice *other=&pool[j];
  if(other==p || !other->selected || other->round!=1 || other->pos!=p->pos ||
     (p->slot>0 && other->slot==p->slot))continue;
  dri_portal_project_add_candidate(&q,other->ovr,other->raw_ovr);
 }
 return dri_portal_choice_pursuit_percent(p,&q,prestige);
}
/* Build distinct additions. A starter addition also fills one headcount vacancy;
   do not count those as two separate players. At most the unchanged room policy's
   remaining additions are planned. A full, adequate room has only optional work. */
static int dri_portal_openings(const PortalQuality rooms[22],int prestige,PortalOpening out[682]) {
 int n=0;
 for(int pos=0;pos<22;pos++) {
  const PortalQuality *q=&rooms[pos];int v=q->ideal-q->count-q->hs_reserved;if(v<0)v=0;
  int limit=(q->base_count>q->ideal?q->base_count:q->ideal)+1-q->count-q->hs_reserved;
  if(!q->ideal || limit<=0 || q->count>=192)continue;
  int ranks=dri_portal_useful_depth(q);
  int k=0;
  for(int rank=0;rank<ranks && k<limit;rank++) {
   int cur=rank<q->count?q->rating[rank]:0;
   int deficit=dri_quality_floor[dri_group(prestige)][pos][rank]-cur*4;
   if(deficit<=0 || (rank<q->count && q->incoming[rank]))continue;
   out[n++]=(PortalOpening){pos,rank<q->starters?1:2,rank+1,deficit,cur+3,k<v,k+1,-1,0};k++;
  }
  while(k<v && k<limit) {
   int rank=q->count+k;
   out[n++]=(PortalOpening){pos,rank<q->starters?1:rank<dri_portal_useful_depth(q)?2:3,rank+1,0,0,1,k+1,-1,0};k++;
  }
 }
 /* Cover positions before their additional slots; vacancies precede full-room
    upgrades. Within a coverage pass, prefer starter, backup, then depth. */
 dri_portal_sort_openings(out,n);
 return n;
}
/* An adequate matrix floor must not hide a material useful upgrade. Promote one
   existing addition, or add one optional addition within the original cap,
   only when the actual eligible pool contains such an upgrade. A promoted
   addition still fills its headcount vacancy; it is never counted twice. */
static int dri_portal_upgrade_openings(const PortalChoice *a,int n,const PortalQuality rooms[22],
                                      int prestige,PortalOpening *needs,int nn) {
 for(int pos=0;pos<22;pos++) {
  const PortalQuality *q=&rooms[pos];int first=-1,has_starter=0,count=0,best=-1,rank=0;
  for(int j=0;j<nn;j++)if(needs[j].pos==pos) {
   count++;if(needs[j].role==1)has_starter=1;
   if(first<0 || needs[j].ordinal<needs[first].ordinal)first=j;
  }
  if(!q->ideal || q->count>=192)continue;
  for(int j=0;j<n;j++) {
   if(a[j].pos!=pos || a[j].score<=0 || dri_quality_block_candidate(q,prestige,a[j].ovr,a[j].raw_ovr))continue;
   /* H3: a healthy room's real quantity vacancy must not become a mandatory
      premium-backup upgrade merely because that expensive candidate exists.
      The ordinary opening still compares every legal candidate, including
      the premium player when it is the sole viable option. */
   if(first>=0 && needs[first].vacancy &&
      dri_portal_choice_pursuit_percent(&a[j],q,prestige)<100)continue;
   int k=dri_quality_rank_candidate(q,prestige,a[j].ovr,a[j].raw_ovr);
   if(!k)k=dri_portal_quality_depth_rank(q,prestige,a[j].ovr);
   if(k<1 || k>dri_portal_useful_depth(q) || (has_starter && k<=q->starters))continue;
   /* A previously planned useful-role repair already covers the same single
      addition. Promote weaker/depth work, never manufacture a second seat. */
   if(first>=0 && needs[first].role<=(k<=q->starters?1:2))continue;
   if(best<0 || (k<=q->starters && rank>q->starters) ||
      ((k<=q->starters)==(rank<=q->starters) && dri_portal_fit_before(&a[j],&a[best]))){best=j;rank=k;}
  }
  if(best<0)continue;
  int current=rank<=q->count?q->rating[rank-1]:0;
  if(first<0) {
   int limit=(q->base_count>q->ideal?q->base_count:q->ideal)+1-q->count-q->hs_reserved;
   if((count>=limit && !dri_portal_quality_depth_rank(q,prestige,a[best].ovr)) || nn>=682)continue;
   first=nn++;needs[first]=(PortalOpening){pos,rank<=q->starters?1:2,rank,0,0,q->count+q->hs_reserved<q->ideal,1,-1,0};
  }
  needs[first].role=rank<=q->starters?1:2;needs[first].rank=rank;
  needs[first].min_ovr=dri_portal_upgrade_minimum(q,prestige,rank-1);
  int quality_min=dri_portal_quality_depth_minimum(q,prestige);
  if(quality_min<needs[first].min_ovr)needs[first].min_ovr=quality_min;
  needs[first].deficit=4*(a[best].ovr-current);
 }
 dri_portal_sort_openings(needs,nn);return nn;
}
/* Only the bounded 83/84-position-ideal flex seats supplement quality needs.
   Healthy full rooms no longer manufacture 80+ rotation upgrade openings. */
static int dri_portal_extra_openings(const PortalChoice *a,int n,const PortalQuality rooms[22],
                                     int prestige,PortalOpening *needs,int nn) {
 /* Flex candidates are ordinary useful depth, with no extra quality bonus.
    The complete-team budget is consumed only by selected primary additions. */
 if(dri_portal_flex_budget(rooms)>0)for(int pos=0;pos<22 && nn<682;pos++) {
  const PortalQuality *q=&rooms[pos];if(!q->flex_allowed)continue;
  int useful=0,count=0;
  for(int j=0;j<n;j++)if(a[j].pos==pos && a[j].score>0 &&
   !dri_quality_block_candidate(q,prestige,a[j].ovr,a[j].raw_ovr) &&
   dri_portal_flex_candidate(q,a[j].ovr)){useful=1;break;}
  if(!useful)continue;
  for(int z=0;z<nn;z++)if(needs[z].pos==pos)count++;
  needs[nn++]=(PortalOpening){pos,3,q->count+1,0,q->rating[q->count-1],0,count+1,-1,1};
 }
 dri_portal_sort_openings(needs,nn);return nn;
}
/* Rebuild an alternative's room with the other SELECTED primary additions.
   Exclude the primary for this opening; no fallback is a secured addition. */
static void dri_portal_alternative_room(PortalQuality *q,const PortalQuality rooms[22],int pos,
                                       const PortalOpening *needs,int nn,int own,
                                       const PortalChoice *a,int capacity,int prestige,const PortalChoice *extra) {
 *q=rooms[pos];int counts[22],ideals[22],covered=0,total=0;
 uint32_t states=rooms[0].exception_states;
 if(!(states&DRI_EXCEPTION_READY))states=1u;
 for(int p=0;p<22;p++){counts[p]=rooms[p].count+rooms[p].hs_reserved;ideals[p]=rooms[p].ideal;}
 /* Only the target depth chart needs copying. The global budget can advance
    its feasible category assignments using each coexisting primary once. */
 for(int j=0;j<=nn;j++) {
  const PortalChoice *p=0;
  if(j==nn)p=extra;
  else if(j!=own && needs[j].first>=0) {
   const PortalChoice *v=&a[needs[j].first];
   if(v->priority>0 && v->priority<=capacity)p=v;
  }
  if(!p)continue;
  const PortalQuality *base=&rooms[p->pos];
  int ceiling=base->base_count>base->ideal?base->base_count:base->ideal;
  if(counts[p->pos]>=ceiling && !dri_portal_quality_depth_rank(base,prestige,p->ovr)) {
   unsigned mask=counts[p->pos]==base->count?dri_portal_exception_mask(base,prestige,p->ovr):DRI_EXCEPTION_GENERAL;
   states=dri_exception_step(states,mask);
  }
  counts[p->pos]++;
  if(p->pos==pos)dri_portal_project_add_candidate(q,p->ovr,p->raw_ovr);
 }
 q->exception_states=states|DRI_EXCEPTION_READY;q->team_excess=dri_team_excess(counts,ideals);
 for(int p=0;p<22;p++){total+=ideals[p];covered+=counts[p]>ideals[p]?counts[p]:ideals[p];}
 q->flex_allowed=!q->hs_reserved && (total==83 || total==84) && covered<85 &&
  q->team_excess<DRI_TEAM_OPTIONAL_LIMIT && dri_portal_extra_position(q) && q->count==q->ideal;
}
/* Optional quality work must leave previously selected vacancy plans usable,
   including their fallback if the first choice loses. This is a planning check
   with the existing admission policy, not an extra signing restriction. */
static int dri_portal_preserves_plans(const PortalChoice *candidate,const PortalChoice *a,
                                     const PortalOpening *needs,const int *fallback,int nn,
                                     const PortalQuality rooms[22],int capacity,int prestige) {
 for(int z=0;z<nn;z++) {
  if(needs[z].first<0 || a[needs[z].first].priority<=0 ||
     a[needs[z].first].priority>capacity)continue;
  PortalQuality alternative;dri_portal_alternative_room(&alternative,rooms,needs[z].pos,needs,nn,z,a,capacity,prestige,candidate);
  const PortalQuality *q=&alternative;
  int indices[2]={needs[z].first,fallback[z]};
  for(int k=0;k<2;k++)if(indices[k]>=0) {
   const PortalChoice *p=&a[indices[k]];
   if(p->priority>0 && p->priority<=capacity && dri_quality_block_candidate(q,prestige,p->ovr,p->raw_ovr))return 0;
  }
 }
 return 1;
}
/* Test14: cover true vacancies first, preserve competitive vacancy fallbacks,
   then cover other vacancy fallbacks before optional upgrades. Reprice every
   candidate against the actual opening as preceding primaries change the room.
   Original scores and all native budgets remain untouched. */
static void dri_portal_order(PortalChoice *a,int n,int capacity,const PortalQuality rooms[22],int prestige) {
 PortalOpening needs[682];PortalQuality projected[22],base[22];
 int fallback[682];
 memcpy(base,rooms,sizeof(base));dri_portal_refresh_budget(base,prestige);
 memcpy(projected,base,sizeof(projected));
 int nn=dri_portal_openings(base,prestige,needs),order=0;
 nn=dri_portal_upgrade_openings(a,n,base,prestige,needs,nn);
 nn=dri_portal_extra_openings(a,n,base,prestige,needs,nn);
 for(int z=0;z<nn;z++)fallback[z]=-1;
 for(int j=0;j<n;j++){
  a[j].priority=0;a[j].slot=0;a[j].selected=0;a[j].round=1000;
  a[j].plan_score=dri_portal_value(&a[j],&base[a[j].pos],prestige);
 }
 /* Genuine vacancy primaries and first fallbacks lead. Flex depth follows
    those needs; optional quality work follows flex. No extra native budget. */
 for(int phase=0;phase<10;phase++)for(int z=0;z<nn;z++) {
  PortalOpening *need=&needs[z];int primary=phase==0 || phase==3 || phase==4;
  int eligible_phase=need->flex?(phase==3 || phase==6 || phase==8):
   need->vacancy?(phase==0 || phase==1 || phase==2 || phase==5):(phase==4 || phase==7 || phase==9);
  if(!eligible_phase)continue;
  if(need->flex && primary && dri_portal_flex_budget(projected)<=0)continue;
  if(!primary && (need->first<0 || a[need->first].priority>capacity))continue;
  if((phase==1 || phase==2) && fallback[z]>=0)continue;
  PortalQuality alternative;
  const PortalQuality *q=&projected[need->pos];
  if(!primary){dri_portal_alternative_room(&alternative,base,need->pos,needs,nn,z,a,capacity,prestige,0);q=&alternative;}
  int best=-1,best_value=0,max_elite_ovr=-1;
  int compare_elites=primary && need->pos==0 && (need->vacancy || need->role==1);
  int elite_index[100],elite_merit[100];
  if(compare_elites)memset(elite_merit,0,sizeof(elite_merit));
  for(int relax=0;relax<2 && best<0;relax++) {
   /* A genuine vacancy keeps a usable lower fallback when no candidate meets
      its preferred quality. Optional full-room upgrades never relax. */
   if(relax && !need->vacancy)break;
   for(int j=0;j<n;j++) {
    PortalChoice *p=&a[j];
    if(p->priority || p->pos!=need->pos || p->score<=0 || dri_quality_block_candidate(q,prestige,p->ovr,p->raw_ovr))continue;
    /* Frozen affordable roles survive the smaller native BOARD/FUNDING
       pools. A globally conditional backup cannot silently become a primary. */
    if(p->market_known && !p->market_protected && (!p->market_planned || (primary && !p->market_primary)))continue;
    if(!primary && dri_portal_backup_defer(p,a,n,q,prestige,relax?0:need->min_ovr,need->flex))continue;
    if(phase==1 && !dri_portal_competitive(p))continue;
    if(!relax && p->ovr<need->min_ovr)continue;
    if(need->flex && !dri_portal_flex_candidate(q,p->ovr))continue;
    if(primary && dri_portal_crowd_defer(p,a,n,q,prestige,relax?0:need->min_ovr,need->flex,need->ordinal))continue;
    if(phase==4 && order<capacity && !dri_portal_preserves_plans(p,a,needs,fallback,nn,base,capacity,prestige))continue;
    int value=primary?dri_portal_value(p,q,prestige):(int)((int64_t)dri_portal_value_unpenalized(p,q,prestige)*(100-p->backup_penalty)/100);if(value<=0)continue;
    if(compare_elites && p->raw_ovr>=0 && p->raw_ovr<=99 && DRI_RC4_ELITE(p->rec)){
     int rating=p->raw_ovr;
     if(value>elite_merit[rating] || (value==elite_merit[rating] && p->rid<a[elite_index[rating]].rid)){
      elite_index[rating]=j;elite_merit[rating]=value;
      if(rating>max_elite_ovr)max_elite_ovr=rating;
     }
    }
    int before=best<0;
    if(best>=0) {
     PortalChoice *previous=&a[best];
     int protected_p=dri_portal_competitive(p),protected_previous=dri_portal_competitive(previous);
     /* A globally verified uncovered-elite primary gets early attention only
        within a real legal opening. It cannot displace a competitive earned
        offered race, and it is not a signing or manufactured scholarship. */
     if(primary && (p->coverage_priority || previous->coverage_priority) && protected_p!=protected_previous)
      before=protected_p>protected_previous;
     else if(primary && p->coverage_priority!=previous->coverage_priority)
      before=p->coverage_priority>previous->coverage_priority;
     else before=dri_portal_value_before(p,value,previous,best_value,phase==1);
    }
    if(before){best=j;best_value=value;}
   }
  }
  if(best<0)continue;
  int elite=-1,elite_value=0;
  /* Compare every eligible rating band with the completed first choice.
     A highly valued lower-rated elite must not hide a viable upgrade. */
  for(int rating=max_elite_ovr;rating>=0 && rating>=a[best].raw_ovr+2;rating--){
   if(elite_merit[rating]>elite_value && rc4_competes(rating,elite_merit[rating],a[best].raw_ovr,best_value)){
    elite=elite_index[rating];elite_value=elite_merit[rating];
   }
  }
  if(elite>=0){
   DRI_RC4_CHANGED(need,&a[best],&a[elite],best_value,elite_value);
   best=elite;best_value=elite_value;
  }
  PortalChoice *p=&a[best];p->priority=++order;p->slot=need->ordinal;
  p->round=primary?1:(phase==5 || phase==8 || phase==9)?3:2;
  p->role=dri_portal_plan_role_candidate(q,prestige,p->ovr,p->raw_ovr);p->deficit=need->deficit;p->plan_score=best_value;
  if(primary){need->first=best;if(order<=capacity){dri_portal_project_add_candidate(&projected[p->pos],p->ovr,p->raw_ovr);dri_portal_refresh_budget(projected,prestige);}}
  else if(p->round==2)fallback[z]=best;
 }
 /* A later coexisting primary can change the earlier recruit's role. Keep
    the final displayed/funded role consistent with every selected addition. */
 for(int j=0;j<n;j++)if(a[j].priority && a[j].priority<=capacity) {
  for(int z=0;z<nn;z++)if(needs[z].pos==a[j].pos && needs[z].ordinal==a[j].slot) {
   PortalQuality q;dri_portal_alternative_room(&q,base,a[j].pos,needs,nn,z,a,capacity,prestige,0);
   a[j].plan_score=a[j].round==1?dri_portal_value(&a[j],&q,prestige):(int)((int64_t)dri_portal_value_unpenalized(&a[j],&q,prestige)*(100-a[j].backup_penalty)/100);
   a[j].role=dri_portal_plan_role_candidate(&q,prestige,a[j].ovr,a[j].raw_ovr);break;
  }
 }
 /* Optional candidates follow covered needs. No score or interest is inflated. */
 for(int j=1;j<n;j++) {
  PortalChoice v=a[j];int k=j;
  while(k>0) {
   PortalChoice *p=&a[k-1];int before;
   if(!!v.priority!=!!p->priority)before=!!v.priority;
   else if(v.priority)before=v.priority<p->priority;
   else before=dri_portal_fit_before(&v,p);
   if(!before)break;
   a[k]=*p;k--;
  }
  a[k]=v;
 }
 /* Physical unplanned entries remain in the native vector. Frozen new-entry
    authorization is enforced separately by admission and the funding scorer;
    leftovers cannot masquerade as covered openings or retained primaries. */
 for(int j=0;j<n;j++)a[j].selected=a[j].priority>0 && a[j].priority<=capacity &&
  a[j].role>0 && a[j].plan_score>0;
}
#endif
