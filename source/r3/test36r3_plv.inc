/* Additive QB-only review. Existing native transfer/stay records are never
   deleted. Recheck at the existing pre-pool hook using FINAL draft decisions. */
#include "test36r3_qb.h"
typedef struct {
 Rec team,players[DRI_R3_QB_LIMIT];R3Qb qb[DRI_R3_QB_LIMIT];
 int n,valid,user,academy,outgoing;
} R3QbSchool;
static int r3_qb_add(R3QbSchool *s,Rec p,int incoming,const int *status,unsigned tid,unsigned year,unsigned stat_year) {
 if(scalar(p,3,0x108)!=0)return 1;
 unsigned id=row(p);if(id>=T36_MAX_PLAYERS)return 0;
 int school_year=(int)scalar(p,242,0x108);
 if(!incoming && (school_year>=3 || scalar(p,261,0x100)!=tid))return 1;
 if(!incoming && status[id]==1){s->outgoing=1;return 1;}
 for(int i=0;i<s->n;i++)if(portal_same_record(p,s->players[i]))return 1;
 if(s->n>=DRI_R3_QB_LIMIT || school_year<0 || school_year>3)return 0;
 T36Player usage={.p=p};if(!incoming)t36_read_usage(&usage,tid,stat_year);
 int raw=(int)scalar(p,191,0x100),dev=(int)scalar(p,271,0x108);
 unsigned injury=scalar(p,140,0x100);
 if(raw<0 || raw>99 || dev<0 || dev>5)return 0;
 T36Person *h=t36_person+id;int paid=(int)scalar(p,139,0x100);
 int known=h->known && h->year==year && h->team==(int)tid && h->table==q((U)p+0x10);
 int retained=known && paid>h->pay;
 if(t36_eos_pay[id].valid && t36_eos_pay[id].tid==(int)tid && t36_eos_pay[id].table==q((U)p+0x10) &&
    t36_eos_pay[id].after==paid && paid>t36_eos_pay[id].before)retained=1;
 int k=s->n++;
 s->qb[k]=(R3Qb){id,raw,portal_projected_ovr(raw,incoming?0:2),school_year,dev,usage.starts,usage.downs,0,
   !incoming,injury==0 || injury==30,incoming || status[id]==2 || retained || usage.protect || (!incoming && scalar(p,155,0x108)!=1)};
 ((void(*)(Rec))vfunc(p,0))(p);s->players[k]=p;return 1;
}
static void r3_qb_supplement(U eval,Rec f,Rec requests,const int *status,unsigned year,unsigned stat_year,const char *site) {
 R3QbSchool *schools=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,138*sizeof(*schools));
 if(!schools)return;
 Rec teams=array_field(f,45);U count=teams?array_count(teams):0;int valid=count>0 && count<=138;
 for(U i=0;i<count && valid;i++) {
  Rec t=array_at(teams,i);unsigned tid=t?scalar(t,383,0x100):138;
  if(tid>=138 || schools[tid].team){release(t);valid=0;break;}
  R3QbSchool *s=schools+tid;s->team=t;s->valid=1;s->academy=portal_academy(tid);
  Rec human=reference(t,407);s->user=human!=0;release(human);
  for(int source=0;source<2 && s->valid;source++) {
   Rec a=array_field(t,source?33:239);U n=a?array_count(a):193;
   if(n>192)s->valid=0;
   for(U j=0;j<n && s->valid;j++){Rec p=array_at(a,j);s->valid=p && r3_qb_add(s,p,source,status,tid,year,stat_year);release(p);}
   release(a);
  }
 }
 /* Secured HS not yet published in Team.CommittedPlayers count too. */
 Rec recruits=valid?array_field(f,39):0;U nr=recruits?array_count(recruits):12001;
 if(nr>12000)valid=0;
 for(U i=0;i<nr && valid;i++) {
  Rec r=array_at(recruits,i);unsigned stage=r?scalar(r,9,0x108):0;
  Rec p=stage>=5?reference(r,5):0;
  if(p && scalar(p,3,0x108)==0)for(unsigned tid=0;tid<138;tid++) {
   R3QbSchool *s=schools+tid;
   if(s->valid && own_commit(r,s->team))s->valid=r3_qb_add(s,p,1,status,tid,year,stat_year);
  }
  release(p);release(r);
 }
 release(recruits);
 int added=0;
 for(unsigned tid=0;tid<138 && valid && !faulted;tid++) {
  R3QbSchool *s=schools+tid;
  if(!s->valid || s->user || s->academy || s->outgoing)continue;
  int pick=-1,opportunities=0;
  for(int k=0;k<s->n;k++)if(r3_qb_entry(s->qb,s->n,k)) {
   int options=0;
   for(unsigned other=0;other<138;other++) {
    R3QbSchool *d=schools+other;
    if(other!=tid && d->valid && !d->user && !d->academy && r3_qb_destination(d->qb,d->n,s->qb+k))options++;
   }
   log_line("R3_QB_PLV_CANDIDATE,%s,%u,%u,%u,%d,%d,%d,%d",site,year,tid,s->qb[k].id,s->qb[k].raw,s->qb[k].year,s->qb[k].starts,options);
   if(options && (pick<0 || r3_qb_entry_before(s->qb+k,s->qb+pick))){pick=k;opportunities=options;}
  }
  if(pick>=0) {
   R3Qb *p=s->qb+pick;int best=r3_qb_leader(s->qb,s->n,-1);
   if(t36_leave(eval,requests,s->players[pick])) {
    T36Person *h=t36_person+p->id;h->encouraged=1;h->reason=11;h->team=(int)tid;h->table=q((U)s->players[pick]+0x10);
    s->outgoing=1;p->available=0;added++;
    log_line("R3_QB_PLV_ADDED,%s,%u,%u,%u,%d,%d,%u,%d,%d",site,year,tid,p->id,p->raw,p->year,s->qb[best].id,s->qb[best].raw,opportunities);
   }
  }
 }
 log_line("R3_QB_PLV_COMPLETE,%s,%u,%d,%d",site,year,valid,added);
 for(int i=0;i<138;i++){for(int j=0;j<schools[i].n;j++)release(schools[i].players[j]);release(schools[i].team);}
 release(teams);HeapFree(GetProcessHeap(),0,schools);
}
static void r3_qb_initial(U eval,Rec f,Rec requests,int *status,unsigned year,unsigned stat_year) {
 /* DraftClass can contain an unresolved early-NFL projection even when that
    player is absent from EarlyNFLPlayers. Do not send his heir away before
    that projection resolves. The pre-pool pass reads the final list again. */
 Rec draft=array_field((Rec)eval,7);
 if(t36_mark_leaving(draft,status,0) && t36_mark_leaving(requests,status,1))
  r3_qb_supplement(eval,f,requests,status,year,stat_year,"PLV");
 release(draft);
}
static void r3_qb_pre_pool(U eval) {
 Rec f=context_ref(eval,6),season=f?reference(f,44):0;
 unsigned year=season?scalar(season,2,0x100):9999,base=season?scalar(season,0,0x100):0;
 if(season && scalar(season,5,0x108)==6 && scalar(season,4,0x100)<=2 && year<=3500) {
  t36_season(f,year);
  Rec requests=context_ref(eval,37),a=as_array(requests),draft=array_field((Rec)eval,7);
  int *status=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,T36_MAX_PLAYERS*sizeof(int));
  if(status && t36_mark_leaving(draft,status,0) && t36_mark_leaving(a,status,1))
   r3_qb_supplement(eval,f,a,status,year,year>=base?year-base:year,"FINAL_PLV_BEFORE_POOL");
  if(status)HeapFree(GetProcessHeap(),0,status);release(draft);release(a);release(requests);
 }
 release(season);release(f);
}
