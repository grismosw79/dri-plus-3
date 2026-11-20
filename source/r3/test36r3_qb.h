#ifndef DRI_TEST36R3_QB_H
#define DRI_TEST36R3_QB_H
#include "portal_policy.h"
#define DRI_R3_QB_LIMIT 24
#define DRI_R3_QB_ENTRY_MIN 80
#define DRI_R3_QB_BACKUP_MIN 70
typedef struct {
 unsigned id;
 int raw,projected,year,dev,starts,downs,depth,returning,available,protected_stay;
} R3Qb;
static inline int r3_qb_before(const R3Qb *a,const R3Qb *b) {
 if(a->projected!=b->projected)return a->projected>b->projected;
 if(a->starts!=b->starts)return a->starts>b->starts;
 if(a->downs!=b->downs)return a->downs>b->downs;
 if(a->returning!=b->returning)return a->returning>b->returning;
 if(a->depth && b->depth && a->depth!=b->depth)return a->depth<b->depth;
 if(a->dev!=b->dev)return a->dev>b->dev;
 return a->id<b->id;
}
static inline int r3_qb_leader(const R3Qb *v,int n,int exclude) {
 int best=-1;
 for(int i=0;i<n;i++)if(i!=exclude && v[i].available && (best<0 || r3_qb_before(v+i,v+best)))best=i;
 return best;
}
/* This supplement is independent of position overage. Preserve two healthy,
   viable options after the departure, and never send the projected QB1. */
static inline int r3_qb_entry(const R3Qb *v,int n,int k) {
 if(k<0 || k>=n || n>DRI_R3_QB_LIMIT)return 0;
 const R3Qb *p=v+k;int best=r3_qb_leader(v,n,-1),left=0;
 if(!p->returning || !p->available || p->protected_stay || p->raw<DRI_R3_QB_ENTRY_MIN ||
    p->year<0 || p->year>2 || best<0 || best==k || p->starts>=3 ||
    (v[best].downs>0 && p->downs*5>=v[best].downs))return 0;
 for(int i=0;i<n;i++)if(i!=k && v[i].available && v[i].raw>=DRI_R3_QB_BACKUP_MIN)left++;
 return left>=2;
}
static inline int r3_qb_entry_before(const R3Qb *a,const R3Qb *b) {
 if((a->year==2)!=(b->year==2))return a->year==2;
 if(a->raw!=b->raw)return a->raw>b->raw;
 if(a->year!=b->year)return a->year>b->year;
 return a->id<b->id;
}
static inline int r3_qb_destination(const R3Qb *v,int n,const R3Qb *candidate) {
 int best=r3_qb_leader(v,n,-1);if(best<0)return 1;
 const R3Qb *p=v+best;
 if(dri_qb_young(p->raw,p->year,p->dev,p->returning))return 0;
 if(p->raw>=DRI_PORTAL_ELITE_QB)return 0;
 return candidate->projected>=p->projected+3;
}
#endif
