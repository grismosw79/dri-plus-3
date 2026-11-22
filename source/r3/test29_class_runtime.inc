/* Included after hs28_scope; plans and caches own numeric identities only. */
#include "test29_class.h"
typedef struct {
 Hs28TeamPlan scope;
 int valid,building,reserve[22];
 int target,secured,planned,shortfall,replacement_exception,protected_over_target;
} Test29ClassCache;
static Test29ClassCache test29_class_cache[138];
static void test29_class_invalidate(Rec team) {
 if(!team)return;hs31_native_score_invalidate(team);unsigned tid=scalar(team,383,0x100);
 if(tid<138)test29_class_cache[tid].valid=0;
 cache_note_class();
}
static int test29_class_cohorts(Rec team,const Hs15Snapshot *s,int junior[22]) {
 memset(junior,0,22*sizeof(*junior));Rec a=array_field(team,239);if(!a)return 0;
 U count=array_count(a);if(count>192){release(a);return 0;}
 int ok=1;
 for(U i=0;i<count;i++) {
  Rec p=array_at(a,i);if(!p){ok=0;break;}
  int year=(int)scalar(p,242,0x108),pos=(int)scalar(p,3,0x108);
  if(year<0 || year>3 || pos<0 || pos>=22)ok=0;
  if(ok && year==2)for(int j=0;j<s->used;j++)if(s->players[j].returning &&
    s->players[j].table==q((U)p+0x10) && s->players[j].rid==row(p)){junior[pos]++;break;}
  release(p);if(!ok)break;
 }
 release(a);return ok;
}
static void test29_class_preferences(Hs28Choice *pool,int n,Rec team,const Hs15Snapshot *s) {
 int academy=test29_class_academy(scalar(team,383,0x100));
 for(int i=0;i<n;i++) {
  Hs28Choice *p=&pool[i];int need=p->pos>=0 && p->pos<22 &&
   (s->count[p->pos]<s->ideal[p->pos] || !s->hs[p->pos]);
  p->academy_preference=p->legal && p->viable?test29_academy_preference(academy,p->stars,p->rank,p->points,p->leader,need,p->native_score):0;
 }
}
static int test29_class_prepare(Hs28Choice *pool,int n,const int vacancy[22],const int optional[22],int general,
 Rec team,U eval,const Hs15Snapshot *s,Hs28Prior *prior,int np,int approved[22],int extra[22]) {
 Test29ClassInput in={0};Test29ClassPlan out;
 in.academy=test29_class_academy(scalar(team,383,0x100));in.general=general;
 in.annual_valid=s->annual_valid;in.annual_target=s->annual_target;
 if(!test29_class_cohorts(team,s,in.junior)){portal_fault("class_cohort_read");return -1;}
 for(int p=0;p<22;p++) {
  in.ideal[p]=s->ideal[p];in.returning[p]=s->returning[p];in.secured[p]=s->hs[p];
  in.vacancy[p]=vacancy[p];in.optional[p]=optional[p];
 }
 Rec budget=reference(team,224);unsigned assigned=budget?scalar(budget,1,0x100):0,total=budget?scalar(budget,3,0x100):0;release(budget);
 for(int i=0;i<n;i++) {
  Hs28Choice *p=&pool[i];if(p->pos<0 || p->pos>=22 || !p->legal || !p->viable || p->native_score<=0)continue;
  in.candidates[p->pos]++;
  if(hs28_market_primary_allowed(p) && p->crowd_penalty<45) {
   int value=(int)(hs28_selection_value(p)/100);
   if(value>in.optional_value[p->pos])in.optional_value[p->pos]=value;
  }
  for(int j=0;j<np;j++)if(!prior[j].closed && prior[j].choice==1 && prior[j].table==p->table && prior[j].rid==p->rid) {
   /* A verified recovery reservation keeps its planned positional place
      through admission; this never classifies it as a funded source seat. */
   int protect=prior[j].reserved;
   if(!protect && p->existing && p->offered && p->target && assigned<=total) {
    unsigned mask=0,allocated=hs28_productive_allocated((Rec)p->target,eval,&mask);
    protect=allocated>0 && allocated<=assigned;
   }
   /* Preserve an earned, still-legal elite primary across the native weekly
      hour reset. This protects its pursuit plan only: source reservations
      below still require current productive native funding. No offer alone
      becomes a player, paid reservation or extra allowance. */
   if(!protect && p->stars>=3 && p->existing && p->offered && p->points>0)
    protect=1;
   if(!protect)break;
   /* A permitted portal upgrade may fill a paid primary's quantity seat.
      Preserve that incumbent only through an already legal optional HS
      opening and the same shared general allowance. Its conditional partner
      shares this seat; no new roster room or exception is created. */
   if(!vacancy[p->pos] && optional[p->pos] && prior[j].slot%32!=31) {
    int old_slot=prior[j].slot,new_slot=p->pos*32+31;
    for(int k=0;k<np;k++)if(prior[k].slot==old_slot && !prior[k].closed)prior[k].slot=new_slot;
   }
   /* A formerly optional primary can now occupy an ordinary annual slot.
      Reassign its paired identity once; do not charge both permissions. */
   if(vacancy[p->pos]>0 && prior[j].slot%32==31) {
    int old_slot=prior[j].slot,new_slot=0;
    for(int seat=1;seat<=vacancy[p->pos] && !new_slot;seat++) {
     int candidate=p->pos*32+seat,occupied=0;
     for(int k=0;k<np;k++)if(!prior[k].closed && prior[k].choice==1 && prior[k].slot==candidate)occupied=1;
     if(!occupied)new_slot=candidate;
    }
    if(new_slot)for(int k=0;k<np;k++)if(prior[k].slot==old_slot && !prior[k].closed)prior[k].slot=new_slot;
   }
   if(prior[j].slot%32==31)in.protected_optional[p->pos]=1;
   else in.protected_vacancy[p->pos]++;
   break;
  }
 }
 int repair[22]={0};
 for(int p=1;p<=18;p++)if(optional[p] && hs34_vacancy_extra(s,p)) {
  for(int i=0;i<n;i++)if(pool[i].pos==p && pool[i].legal && pool[i].viable &&
    pool[i].native_score>0 && hs28_market_primary_allowed(&pool[i]) && pool[i].crowd_penalty<45) {
   repair[p]=1;break;
  }
 }
 int secured_extra=class34r2_secured_extra(s->annual_valid,s->annual,s->hs);
 int repair_places=class34r2_plan(&in,repair,secured_extra,&out);
 memcpy(approved,out.vacancy,sizeof(out.vacancy));memcpy(extra,out.optional,sizeof(out.optional));
 unsigned tid=scalar(team,383,0x100);if(tid<138) {
  Test29ClassCache *c=&test29_class_cache[tid];
  if(c->target!=out.target || c->secured!=out.secured || c->planned!=out.planned || c->shortfall!=out.shortfall ||
    c->replacement_exception!=out.replacement_exception || c->protected_over_target!=out.protected_over_target) {
   log_line("TEST29_CLASS_PLAN,%u,%d,%d,%d,%d,%d,%d,%d,%d,PLANNED_NOT_SIGNED",tid,out.target,out.secured,
    out.replacement,out.available,out.planned,out.shortfall,out.replacement_exception,out.protected_over_target);
   log_line("TEST34R2_CLASS_CAPACITY,%u,%d,%d,%d,%d,%d,BASE_SECURED_EXTRA_PENDING_REPAIR_AUTHORIZED_GENERAL",
    tid,s->annual_target,secured_extra,repair_places,out.target,general);
   c->target=out.target;c->secured=out.secured;c->planned=out.planned;c->shortfall=out.shortfall;
   c->replacement_exception=out.replacement_exception;c->protected_over_target=out.protected_over_target;
  }
 }
 return out.general;
}
static int test29_class_new_primary_room(const Hs15Snapshot *s,unsigned tid,int planned,int pending_repair) {
 Test29ClassInput in={0};Test29ClassPlan out;in.academy=test29_class_academy(tid);
 in.annual_valid=s->annual_valid;
 in.annual_target=class34r2_capacity(s->annual_valid,s->annual_target,
  class34r2_secured_extra(s->annual_valid,s->annual,s->hs),pending_repair);
 for(int p=0;p<22;p++){in.ideal[p]=s->ideal[p];in.returning[p]=s->returning[p];in.secured[p]=s->hs[p];}
 test29_class_plan(&in,&out);return planned<out.remaining;
}
