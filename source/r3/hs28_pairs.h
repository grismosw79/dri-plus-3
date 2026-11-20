#ifndef DRI_HS28_PAIRS_H
#define DRI_HS28_PAIRS_H
#include <stdint.h>
/* Planning only. A selected alternative never consumes another roster place.
   The live admission gate rechecks ownership and the shared exception budget. */
typedef struct {
 uint64_t table,rec,target;
 uint32_t rid;
 int pos,ovr,stars,stage,rank,points,leader,cut_points,commit_score,native_score;
 int legal,offered,existing,viable,urgent,index,final;
 int selected,slot,choice,weight,order;
 int crowd_penalty;
 int backup_penalty,market_roles,market_known,market_planned,market_primary,market_protected;
 /* Scratch evidence: 0 unknown/conservative, 1 tentative, 2 paid/earned.
    Never serialized; an offered earned pursuit survives native hour resets. */
 int pursuit_state;
 uint64_t player_table;uint32_t player_row;
 int academy_preference; /* Occupies prior trailing alignment padding. */
} Hs28Choice;
/* R5: native fit remains a gate and a major part of selection, while a
   material HS talent difference can outweigh a small fit-score difference.
   The bonus is selection utility only; native scores/influence are untouched.
   Center at65 so ordinary prospects retain the previous score scale. */
#define HS29_QUALITY_POINT_VALUE 8
static inline int hs28_quality_value(const Hs28Choice *p) {
 return p->ovr>65?(p->ovr-65)*HS29_QUALITY_POINT_VALUE:0;
}
static inline int64_t hs28_selection_value(const Hs28Choice *p) {
 return (int64_t)p->native_score*(100-p->crowd_penalty+p->academy_preference)+
  (int64_t)hs28_quality_value(p)*(100-p->crowd_penalty);
}
static inline int hs28_better(const Hs28Choice *a,const Hs28Choice *b) {
 if(a->viable!=b->viable)return a->viable>b->viable;
 /* Every HS phase uses the same fit-plus-quality selection utility. */
 int64_t avalue=hs28_selection_value(a);
 int64_t bvalue=hs28_selection_value(b);
 if(avalue!=bvalue)return avalue>bvalue;
 /* Native fit still controls admission to the shortlist. Within a legal
    position pool compare current quality; a funded pursuit gets a small
    continuity preference, not an unlimited protection from better talent. */
 int av=a->ovr*10+a->stars*3+(!a->final && a->existing && a->offered?15:0);
 int bv=b->ovr*10+b->stars*3+(!b->final && b->existing && b->offered?15:0);
 if(av!=bv)return av>bv;
 if(!a->final && a->points!=b->points)return a->points>b->points;
 return a->rid<b->rid;
}
static inline int hs28_material_upgrade(const Hs28Choice *a,const Hs28Choice *b) {
 return a->legal && a->viable && (!a->market_known || a->market_protected || (a->market_planned && a->market_primary)) && !a->crowd_penalty && a->stars>=2 && b->stage<4 &&
  a->ovr>=b->ovr+4 && a->native_score>0 &&
  hs28_selection_value(a)>=hs28_selection_value(b)+500;
  /* Test31: the existing utility already prices native fit and quality.
     Do not reject the same fit difference a second time with a90% ratio.
     Four OVR and five net utility points retain a bounded continuity margin;
     a useful three-star may qualify too. Native eligibility, market entry
     and soft-commit protection remain independent requirements. */
}
static inline int hs28_market_primary_allowed(const Hs28Choice *p) {
 return !p->market_known || p->market_protected || (p->market_planned && p->market_primary);
}
/* Test34: an unearned proposal does not have the same continuity claim as
   a paid pursuit. Unknown evidence keeps the conservative historical rule.
   This compares existing utility; it writes no native score or influence. */
static inline int hs34_primary_upgrade(const Hs28Choice *a,const Hs28Choice *b) {
 if(b->pursuit_state!=1)return hs28_material_upgrade(a,b);
 return b->stage<4 && a->pos==b->pos && a->legal && a->viable &&
  hs28_market_primary_allowed(a) && !a->crowd_penalty && a->stars>=2 &&
  a->native_score>0 && hs28_better(a,b);
}
static inline int hs28_best_backup(Hs28Choice *p,int n,int pos) {
 int best=-1,scarce=-1;
 for(int i=0;i<n;i++) {
  int penalty=p[i].market_roles?p[i].backup_penalty:p[i].crowd_penalty;
  if(!p[i].legal || !p[i].viable || p[i].native_score<=0 || p[i].selected || p[i].pos!=pos ||
    (p[i].market_known && !p[i].market_protected && !p[i].market_planned))continue;
  if(penalty>=45) {
   if(p[i].market_roles && p[i].viable && p[i].native_score>0 &&
      (scarce<0 || hs28_better(&p[i],&p[scarce])))scarce=i;
   continue;
  }
  Hs28Choice candidate=p[i];candidate.crowd_penalty=penalty;
  Hs28Choice previous={0};if(best>=0){previous=p[best];previous.crowd_penalty=previous.market_roles?previous.backup_penalty:previous.crowd_penalty;}
  if(best<0 || hs28_better(&candidate,&previous))best=i;
 }
 /* Secondary attention is a soft preference too: a sole credible fallback
    remains available after the primary consumes the only close alternative. */
 if(scarce>=0 && best>=0 && (p[best].ovr<p[scarce].ovr-3 ||
    (int64_t)p[best].native_score*100<(int64_t)p[scarce].native_score*70))return scarce;
 return best>=0?best:scarce;
}
static inline int hs28_best(Hs28Choice *p,int n,int pos) {
 int best=-1;
 for(int i=0;i<n;i++)if(p[i].legal && p[i].viable && p[i].native_score>0 && hs28_market_primary_allowed(&p[i]) && p[i].crowd_penalty<45 && !p[i].selected && p[i].pos==pos &&
  (best<0 || hs28_better(&p[i],&p[best])))best=i;
 return best;
}
/* A soft crowding deferral cannot erase a genuine quantity vacancy after
   its sole close alternative was consumed by another primary. This fallback
   is never used for conditional backups or discretionary overage places. */
static inline int hs28_best_quantity(Hs28Choice *p,int n,int pos) {
 int best=hs28_best(p,n,pos);
 if(best>=0 && p[best].viable && p[best].native_score>0)return best;
 int fallback=-1;
 for(int i=0;i<n;i++)if(p[i].legal && hs28_market_primary_allowed(&p[i]) && p[i].viable && p[i].native_score>0 &&
   !p[i].selected && p[i].pos==pos && (fallback<0 || hs28_better(&p[i],&p[fallback])))fallback=i;
 return fallback>=0?fallback:best;
}
static inline int hs28_plan(Hs28Choice *p,int n,const int vacancy[22],
 const int optional[22],int general,int capacity) {
 int chosen=0,primaries[50],pn=0,optional_used[22]={0};
 if(capacity>50)capacity=50;
 if(capacity<0)capacity=0;
 for(int i=0;i<n;i++)p[i].selected=p[i].slot=p[i].choice=p[i].weight=p[i].order=0;
 /* All positions get their first primary before any gets its next seat. */
 for(int seat=0;seat<30 && chosen<capacity;seat++)for(int pos=0;pos<22 && chosen<capacity;pos++) {
  if(seat>=vacancy[pos])continue;
  int k=hs28_best_quantity(p,n,pos);if(k<0)continue;
  p[k].selected=1;p[k].slot=pos*32+seat+1;p[k].choice=1;p[k].order=++chosen;
  primaries[pn++]=k;
 }
 /* At most one discretionary HS opening per position, authorized only in
    an inherited full room. A filled vacancy cannot become another opening. */
 while(general>0 && chosen<capacity) {
  int best=-1;
  for(int pos=0;pos<22;pos++)if(optional[pos] && !optional_used[pos]) {
   int k=hs28_best(p,n,pos);
   if(k>=0 && (best<0 || hs28_better(&p[k],&p[best])))best=k;
  }
  if(best<0)break;
  int pos=p[best].pos;optional_used[pos]=1;general--;
  p[best].selected=1;p[best].slot=pos*32+31;p[best].choice=1;p[best].order=++chosen;
  primaries[pn++]=best;
 }
 for(int i=0;i<pn && chosen<capacity;i++) {
  int primary=primaries[i],k=hs28_best_backup(p,n,p[primary].pos);if(k<0)continue;
  p[k].selected=1;p[k].slot=p[primary].slot;p[k].choice=2;p[k].order=++chosen;
 }
 for(int i=0;i<pn;i++) {
  int a=primaries[i],b=-1;
  for(int k=0;k<n;k++)if(p[k].selected && p[k].choice==2 && p[k].slot==p[a].slot){b=k;break;}
  p[a].weight=40;
  if(b>=0) {
   p[b].weight=20;
   if(p[b].urgent && !p[a].urgent){p[a].weight=30;p[b].weight=30;}
   if(!p[a].viable && p[b].viable){p[a].weight=20;p[b].weight=40;}
  }
 }
 return chosen;
}
/* Previous plan contains numeric identities only. A closed opening cannot
   donate its fallback as a new primary ahead of another retained pair. */
typedef struct {
 uint64_t table; uint32_t rid;
 int pos,slot,choice,order,native_score,closed,reserved;
} Hs28Prior;
static inline int hs28_prior_index(const Hs28Choice *p,int n,const Hs28Prior *old) {
 for(int i=0;i<n;i++)if(p[i].table==old->table && p[i].rid==old->rid)return i;
 return -1;
}
static inline int hs28_prior_live(const Hs28Choice *p,int n,const Hs28Prior *old) {
 int k=hs28_prior_index(p,n,old);
 return k>=0 && (old->choice!=1 || hs28_market_primary_allowed(&p[k])) &&
  (!p[k].market_known || p[k].market_protected || p[k].market_planned) &&
  (p[k].existing || old->reserved) && !(old->choice==2 && p[k].market_roles?p[k].backup_penalty:p[k].crowd_penalty) &&
  p[k].legal && p[k].viable && p[k].native_score>0 && !p[k].selected?k:-1;
}
static inline int hs28_keep_pair(Hs28Choice *p,int n,const Hs28Prior *old,int no,
 int pos,int optional,int used[50],int *previous_slot) {
 int best=-1,best_order=0x7fffffff,pair=-1,promoted=0;
 for(int j=0;j<no;j++)if(!used[j] && old[j].choice==1 && old[j].pos==pos &&
    !old[j].closed && ((old[j].slot%32==31)==optional)) {
  int k=hs28_prior_live(p,n,&old[j]),from_backup=k<0;
  if(k<0)for(int b=0;b<no;b++)if(old[b].slot==old[j].slot && old[b].choice==2 && !old[b].closed) {
   k=hs28_prior_live(p,n,&old[b]);if(k>=0 && !hs28_market_primary_allowed(&p[k]))k=-1;break;
  }
  if(k>=0 && old[j].order<best_order){best=k;best_order=old[j].order;pair=j;promoted=from_backup;}
 }
 if(pair>=0){used[pair]=1;*previous_slot=old[pair].slot;}
 int incumbent=best;
 for(int i=0;incumbent>=0 && i<n;i++)if(!p[i].selected && p[i].pos==pos &&
   (promoted ? p[incumbent].stage<4 && p[i].legal && p[i].viable && p[i].native_score>0 &&
     hs28_market_primary_allowed(&p[i]) && p[i].crowd_penalty<45 && hs28_better(&p[i],&p[incumbent]) :
     hs34_primary_upgrade(&p[i],&p[incumbent]))){
  int other_primary=0;
  for(int j=0;j<no;j++)if(old[j].choice==1 && !old[j].closed && old[j].slot!=*previous_slot &&
    old[j].table==p[i].table && old[j].rid==p[i].rid && hs28_prior_live(p,n,&old[j])>=0)other_primary=1;
  if(!other_primary && (best==incumbent || hs28_better(&p[i],&p[best])))best=i;
 }
 /* After a primary is lost, the fallback competes for the newly open role.
    Its prior backup reservation is not incumbent-primary protection. */
 return best;
}
static inline int hs28_plan_prior(Hs28Choice *p,int n,const int vacancy[22],
 const int optional[22],int general,int capacity,const Hs28Prior *old,int no) {
 if(!no)return hs28_plan(p,n,vacancy,optional,general,capacity);
 int chosen=0,primaries[50],previous[50],pn=0,used[50]={0},optional_used[22]={0};
 if(no>50)no=50;
 if(capacity>50)capacity=50;
 if(capacity<0)capacity=0;
 for(int i=0;i<n;i++)p[i].selected=p[i].slot=p[i].choice=p[i].weight=p[i].order=0;
 for(int seat=0;seat<30 && chosen<capacity;seat++)for(int pos=0;pos<22 && chosen<capacity;pos++) {
  if(seat>=vacancy[pos])continue;
  int prev=0,k=hs28_keep_pair(p,n,old,no,pos,0,used,&prev);
  if(k<0)k=hs28_best_quantity(p,n,pos);
  if(k<0)continue;
  p[k].selected=1;p[k].slot=pos*32+seat+1;p[k].choice=1;p[k].order=++chosen;
  primaries[pn]=k;previous[pn++]=prev;
 }
 while(general>0 && chosen<capacity) {
  int best=-1,best_prev=0,best_pair=-1;
  for(int pos=0;pos<22;pos++)if(optional[pos] && !optional_used[pos]) {
   int probe[50];for(int j=0;j<50;j++)probe[j]=used[j];int prev=0;
   int k=hs28_keep_pair(p,n,old,no,pos,1,probe,&prev);
   if(k<0)k=hs28_best(p,n,pos);
   if(k>=0 && (best<0 || (prev && !best_prev) || ((!!prev==!!best_prev) && hs28_better(&p[k],&p[best])))) {
    best=k;best_prev=prev;best_pair=-1;
    for(int j=0;j<no;j++)if(probe[j]!=used[j])best_pair=j;
   }
  }
  if(best<0)break;
  int pos=p[best].pos;optional_used[pos]=1;general--;if(best_pair>=0)used[best_pair]=1;
  p[best].selected=1;p[best].slot=pos*32+31;p[best].choice=1;p[best].order=++chosen;
  primaries[pn]=best;previous[pn++]=best_prev;
 }
 for(int i=0;i<pn && chosen<capacity;i++) {
  int a=primaries[i],b=-1;
  if(previous[i])for(int j=0;j<no;j++)if(old[j].slot==previous[i] && old[j].choice==1 && !old[j].closed) {
   b=hs28_prior_live(p,n,&old[j]);if(b>=0)break;
  }
  if(b<0 && previous[i])for(int j=0;j<no;j++)if(old[j].slot==previous[i] && old[j].choice==2 && !old[j].closed) {
   b=hs28_prior_live(p,n,&old[j]);break;
  }
  if(b<0)for(int j=0;j<no;j++)if(old[j].reserved && old[j].choice==2 &&
    old[j].slot==p[a].slot && !old[j].closed) {
   b=hs28_prior_live(p,n,&old[j]);if(b>=0)break;
  }
  if(b<0)b=hs28_best_backup(p,n,p[a].pos);
  if(b>=0){p[b].selected=1;p[b].slot=p[a].slot;p[b].choice=2;p[b].order=++chosen;}
 }
 for(int i=0;i<pn;i++) {
  int a=primaries[i],b=-1;p[a].weight=40;
  for(int j=0;j<n;j++)if(p[j].selected && p[j].slot==p[a].slot && p[j].choice==2){b=j;break;}
  if(b>=0){p[b].weight=20;if(p[b].urgent && !p[a].urgent)p[a].weight=p[b].weight=30;}
 }
 return chosen;
}
#endif
