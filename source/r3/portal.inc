/* Portal Test02. Native schema ordinals verified against supplied capture. */
#include "portal_policy.h"
#include "test34_diagnostics.h"
#include "class34r2_plan.h"
static void test29_class_invalidate(Rec team);
static void test31_offer_register(U eval);
static unsigned test31_productive_allocated(Rec target,Rec team,U eval,unsigned *mask);
typedef struct {U table;uint32_t row;int ovr,portal,signed_here,develop,qb_young,qb_priority;} PortalPlayer;
typedef struct {int pos,ideal,starters,count,best,role,incoming_best,prestige;PortalPlayer players[192];PortalQuality quality,projected;} PortalRoster;
typedef struct {U table;unsigned count;uint32_t ids[2048];} PortalDepartures;
static Rec context_ref(U context,int n) {
 Rec out=0;U t=q(context+0x10),f=field(q(context+0x30),n);
 ((U(*)(U,Rec*,uint32_t,U))q(q(t)+0x128))(t,&out,*(uint32_t*)(context+0x18),f);return out;
}
static int transfer_recruit(Rec recruit) {unsigned c=scalar(recruit,2,0x108);return c>=4 && c<=7;}
#include "rc4_portal_state.inc"
static void portal_fault(const char *why) {
 InterlockedExchange(&faulted,1);InterlockedExchange(&mode,-1);
 log_line("FAULT,portal_%s,behavior_disabled_restart_game",why);
}
/* Native short string: 15 inline characters plus metadata, then allocator.
   The game's HS caller initializes tag 15 and a null/default allocator. */
/* Test34 diagnostics are scoped to an existing candidate evaluation. Nested
   board planning cannot overwrite a different recruit/team's evidence. */
typedef struct {
 U team_table,recruit_table;uint32_t team_row,rid,tid;int active,code,raw;
 int pos,annual,secured,projected,ideal,score,slot,role,cost,available,class_secured,class_target,roster_total;
 char native_reason[96];
} Hs34Trace;
static Hs34Trace hs34_trace;
static int hs34_trace_matches(Rec team,Rec recruit) {
 return hs34_trace.active && team && recruit && q((U)team+0x10)==hs34_trace.team_table &&
  row(team)==hs34_trace.team_row && q((U)recruit+0x10)==hs34_trace.recruit_table && row(recruit)==hs34_trace.rid;
}
static void hs34_trace_begin(Rec team,Rec recruit) {
 memset(&hs34_trace,0,sizeof(hs34_trace));
 hs34_trace.active=1;hs34_trace.code=HS34_DATA;hs34_trace.pos=-1;
 hs34_trace.team_table=q((U)team+0x10);hs34_trace.team_row=row(team);
 hs34_trace.recruit_table=q((U)recruit+0x10);hs34_trace.rid=row(recruit);
 hs34_trace.tid=scalar(team,383,0x100);hs34_trace.available=-1;
 memcpy(hs34_trace.native_reason,"NOT_EVALUATED",14);
}
static void hs34_trace_note(Rec team,Rec recruit,int code) {
 if(hs34_trace_matches(team,recruit))hs34_trace.code=code;
}
static void hs34_trace_block(Rec team,Rec recruit,const Hs15Snapshot *s,int pos,int blocked) {
 if(!hs34_trace_matches(team,recruit))return;
 hs34_trace.raw=blocked;hs34_trace.pos=pos;
 if(pos<0 || pos>=22){hs34_trace.code=HS34_DATA;return;}
 hs34_trace.annual=s->annual_valid?s->annual[pos]:-1;hs34_trace.secured=s->hs[pos];
 hs34_trace.projected=s->count[pos];hs34_trace.ideal=s->ideal[pos];
 hs34_trace.class_target=s->annual_target;hs34_trace.roster_total=s->total;hs34_trace.class_secured=0;
 for(int p=0;p<22;p++)hs34_trace.class_secured+=s->hs[p];
 if(!blocked){hs34_trace.code=HS34_UNCLASSIFIED;return;}
 hs34_trace.code=blocked<0?HS34_DATA:blocked==8?HS34_EXCEPTION:blocked==2?HS34_STAR_LIMIT:
  blocked==4?HS34_QB_LIMIT:(s->annual_valid && s->hs[pos]>=s->annual[pos])?HS34_ANNUAL:HS34_PHYSICAL;
 if(hs34_trace.code==HS34_ANNUAL && pos>=1 && pos<=18 && s->count[pos]<s->ideal[pos]) {
  if(s->hs[pos]>s->annual[pos])hs34_trace.code=HS34_VACANCY_USED;
  else if(hs34_trace.class_secured>=class34r2_capacity(s->annual_valid,s->annual_target,0,2))hs34_trace.code=HS34_CLASS_FILLED;
 }
}
static void hs34_trace_emit(const Hs34Trace *t,const char *site) {
 if(!log30_deep)return;
 log_line("TEST34_RECRUIT_GATE,%s,%u,%u,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%s",
  site,t->tid,t->rid,hs34_reason_label(t->code),t->raw,t->pos,t->annual,t->secured,
  t->projected,t->ideal,t->score,t->slot,t->role,t->cost,t->available,t->class_secured,t->class_target,t->roster_total,t->native_reason);
}
static void hs34_trace_end(void){hs34_trace.active=0;}
typedef struct { unsigned char data[16]; U allocator; } NativeEligibilityReason;
_Static_assert(sizeof(NativeEligibilityReason)==24 && offsetof(NativeEligibilityReason,allocator)==16,"Native eligibility reason ABI");
static void native_eligibility_reason_release(NativeEligibilityReason *s) {
 if(s->data[15]&0x80) {
  U data=q((U)s);uint32_t capacity=*(uint32_t *)(s->data+12)&0x7fffffffU;
  if(data)((void(*)(U*,U,U))(game_base+0x019ea6a0))(&s->allocator,data,(U)capacity+1);
 }
}
static int native_eligibility_exception(EXCEPTION_POINTERS *p) {
 EXCEPTION_RECORD *r=p?p->ExceptionRecord:0;
 U address=r?(U)r->ExceptionAddress:0;
 log_line("ELIGIBILITY_EXCEPTION,0x%08lx,0x%llx,game_rva_0x%llx,%llu,0x%llx",
  r?r->ExceptionCode:0,(unsigned long long)address,
  (unsigned long long)(address>=game_base?address-game_base:0),
  (unsigned long long)(r && r->NumberParameters>0?r->ExceptionInformation[0]:0),
  (unsigned long long)(r && r->NumberParameters>1?r->ExceptionInformation[1]:0));
 return fail_exception("portal_eligibility",r?r->ExceptionCode:0);
}
/* R9: the native eligibility receiver is evaluator->manager, not evaluator.
   Check record recognition before calling the native permissive fallback. */
static int native_recruit_eligible(U eval,Rec team,Rec recruit) {
 U manager=eval?q(eval+0xc8):0;
 if(!manager || !team || !recruit){portal_fault("eligibility_dependencies");return 0;}
 U player_type=q(manager+0x80),recruit_type=q(manager+0xa0);
 if(!player_type || !recruit_type || !q(manager+0x180) || !q(manager+0x1b0)) {
  portal_fault("eligibility_manager_layout");return 0;
 }
 U desc=schema(recruit);
 unsigned char (*matches)(U,U)=(void*)(game_base+0x08059c60);
 if(!matches(desc,player_type) && !matches(desc,recruit_type)) {
  portal_fault("eligibility_record_type");return 0;
 }
 NativeEligibilityReason reason={0};reason.data[15]=15;int allowed=0;
 Rec prev_eval=rc4_evaluating;rc4_evaluating=rc4_member(recruit)?recruit:0;
 __try {
  __try {
   allowed=((unsigned char(*)(U,Rec*,Rec*,NativeEligibilityReason*))(game_base+0x0816b190))(manager,&team,&recruit,&reason)!=0;
   if(hs34_trace_matches(team,recruit)) {
    if(allowed)memcpy(hs34_trace.native_reason,"ALLOWED",8);
    else {
     hs34_trace.code=HS34_NATIVE;
     __try {
      const unsigned char *text=0;size_t size=0;
      if(hs34_reason_payload(reason.data,&text,&size))hs34_reason_text(hs34_trace.native_reason,text,size);
      else memcpy(hs34_trace.native_reason,"REASON_LAYOUT_INVALID",22);
     } __except(EXCEPTION_EXECUTE_HANDLER) {memcpy(hs34_trace.native_reason,"REASON_UNREADABLE",18);}
    }
   }
  } __finally {
   rc4_evaluating=prev_eval;native_eligibility_reason_release(&reason);}
 } __except(native_eligibility_exception(GetExceptionInformation())) {return 0;}
 return allowed;
}
/* Native NSD appends a signing to a per-team temporary vector before publishing
   Team.CommittedPlayers. TopSchoolsList is not rewritten on a redirection.
   Remember only the destination entering the existing native signing path;
   use it ONLY when the recruit subsequently has the native Signed state.
   Nothing here changes a recruit, interest list, offer, or destination. */
#define PORTAL_SIGN_SLOTS 16384
/* Table identities are compared as values. Their records belong to the
   current dynasty and must never be retained across a menu or save change. */
typedef struct {U table,player_table;uint32_t rid,pid;unsigned tid,pool_field;} PortalSign;
typedef struct {U table;uint32_t rid;unsigned league,year,week,type;PortalSign entries[PORTAL_SIGN_SLOTS];} PortalOwnership;
static PortalOwnership signing_owners;
static SRWLOCK ownership_lock=SRWLOCK_INIT;
static uint64_t portal_scope_generation;
static __declspec(thread) uint64_t portal_callback_generation;
static void portal29_budget_reset(void);
static void hs_cleanup_step(const char *phase,int team,int recruit,int index);
static void portal_ownership_clear_locked(void) {
 hs_cleanup_step("ownership_budget_reset",-1,-1,-1);
 portal29_budget_reset();
 hs_cleanup_step("clear_ownership_index",-1,-1,-1);
 memset(&signing_owners,0,sizeof(signing_owners));
 ++portal_scope_generation;
 pt28_invalidate();hs28_nsd_reset();
}
static int portal_ownership_scope(Rec franchise) {
 Rec season=reference(franchise,44);if(!season)return 0;
 unsigned year=scalar(season,2,0x100),week=scalar(season,4,0x100),type=scalar(season,5,0x108);
 release(season);U table=q((U)franchise+0x10);uint32_t rid=row(franchise);
 unsigned league=scalar(franchise,28,0x100);
 AcquireSRWLockExclusive(&ownership_lock);
 __try {
  if(signing_owners.table!=table || signing_owners.rid!=rid || signing_owners.league!=league ||
     signing_owners.year!=year || signing_owners.week!=week || signing_owners.type!=type) {
   /* Only the dynasty identity is needed to scope accepted signings. */
   portal_ownership_clear_locked();
   signing_owners.table=table;signing_owners.rid=rid;signing_owners.league=league;
   signing_owners.year=year;signing_owners.week=week;signing_owners.type=type;
  }
  portal_callback_generation=portal_scope_generation;
 } __finally {ReleaseSRWLockExclusive(&ownership_lock);}
 return 1;
}
static unsigned portal_sign_hash(U table,uint32_t rid){
 return (unsigned)((table>>4)^((U)rid*2654435761u))&(PORTAL_SIGN_SLOTS-1);
}
static PortalSign *portal_sign_slot(U table,uint32_t rid,int create) {
 if(!table)return 0;
 unsigned k=portal_sign_hash(table,rid);
 for(unsigned i=0;i<PORTAL_SIGN_SLOTS;i++,k=(k+1)&(PORTAL_SIGN_SLOTS-1)) {
  PortalSign *s=&signing_owners.entries[k];
  if(s->table==table && s->rid==rid)return s;
  if(!s->table)return create?s:0;
 }
 return 0;
}
static int portal_signed_destination(Rec recruit,Rec player,int *tid) {
 if(!recruit||!player||!tid)return 0;
 U table=q((U)recruit+0x10),player_table=q((U)player+0x10);
 uint32_t rid=row(recruit),pid=row(player);
 int found=0;AcquireSRWLockExclusive(&ownership_lock);
 PortalSign *s=portal_sign_slot(table,rid,0);
 if((!portal_callback_generation||portal_callback_generation==portal_scope_generation) &&
    s && player_table && s->player_table==player_table && s->pid==pid && s->tid<138){*tid=(int)s->tid;found=1;}
 ReleaseSRWLockExclusive(&ownership_lock);return found;
}
/* Preserve known ownership across the native hard-commit -> Signed path too.
   That bulk path bypasses both late signing callbacks and can defer publishing
   Team.CommittedPlayers. Only native Signed records consult this ledger. */
static int portal_store_owner(Rec recruit,Rec player,unsigned tid) {
 if(!recruit||!player||tid>=138)return 0;
 U table=q((U)recruit+0x10),player_table=q((U)player+0x10);
 if(!table||!player_table)return 0;
 uint32_t rid=row(recruit),pid=row(player);
 unsigned stage=scalar(recruit,9,0x108),field=scalar(recruit,2,0x108)<=3?39u:46u;
 int ok=0;AcquireSRWLockExclusive(&ownership_lock);
 /* A completed callback for a previous calendar cannot write into the next. */
 if(portal_callback_generation!=portal_scope_generation){
  ReleaseSRWLockExclusive(&ownership_lock);return 1;
 }
 PortalSign *s=portal_sign_slot(table,rid,1);
 if(s && !(s->player_table==player_table && s->pid==pid && s->tid!=tid && stage>=5)) {
  if(s->player_table!=player_table||s->pid!=pid||s->tid!=tid||s->pool_field!=field){
   *s=(PortalSign){table,player_table,rid,pid,tid,field};pt28_invalidate();
  }
  ok=1;
 }
 ReleaseSRWLockExclusive(&ownership_lock);return ok;
}
static void portal_note_owner(Rec recruit,Rec team,U context,const char *site,int hard_commit) {
 if(mode!=1 || faulted || !recruit || !team || scalar(recruit,9,0x108)>=6)return;
 unsigned tid=scalar(team,383,0x100);if(tid>=138){portal_fault("signing_team");return;}
 Rec franchise=context_ref(context,6),player=reference(recruit,5);
 if(!franchise || !player || !portal_ownership_scope(franchise)) {
  release(franchise);release(player);portal_fault("signing_identity");return;
 }
 release(franchise);int ok=portal_store_owner(recruit,player,tid);release(player);
 if(!ok){portal_fault("signing_capacity");return;}
 hs28_nsd_reset();
 test29_class_invalidate(team); /* Accepted native path invalidates a prior source-capacity snapshot. */
 if(hard_commit)log_line("PORTAL_COMMIT_PENDING,%s,%u,%u,AWAIT_NATIVE_HARD_COMMIT",site,tid,row(recruit));
 else log_line("PORTAL_SIGN_PENDING,%s,%u,%u,AWAIT_NATIVE_SIGNED",site,tid,row(recruit));
}
static void portal_note_signing(Rec recruit,Rec team,U context,const char *site) {portal_note_owner(recruit,team,context,site,0);}
#include "portal_integrity.inc"
/* A published acquisition can still have a stale Top10/SoftCommitted stage in
   loaded saves. It is already owned, not a new recruiting opportunity. Preserve
   that owner before any accepted transition, including user-school paths.
   Published HS acquisitions must not enter another native publication queue;
   their stage is finalized once at the NSD boundary. Portal paths are unchanged. */
static int portal_existing_admission(Rec recruit,Rec team,U context,const char *site,int *known) {
 *known=0;
 if(mode!=1 || faulted || !recruit || !team || scalar(recruit,2,0x108)>7)return 0;
 unsigned stage=scalar(recruit,9,0x108),candidate_tid=scalar(team,383,0x100);
 /* Native hard/Signed completion activates the retained destination. Prevent
    a second acceptance while the first native publication is still queued. */
 if(stage>=5) {
  Rec franchise=context_ref(context,6),player=reference(recruit,5);int accepted=-1;
  if(!franchise || !player || !portal_ownership_scope(franchise)) {
   release(player);release(franchise);portal_fault("pending_admission_identity");return 0;
  }
  int has=portal_signed_destination(recruit,player,&accepted);release(player);release(franchise);
  if(has && (stage>=6 || accepted!=(int)candidate_tid)) {
   *known=1;log_line("TEST31_PENDING_ADMISSION,%s,%u,%u,%d,NATIVE_ACCEPTANCE_ALREADY_COMPLETED",site,candidate_tid,row(recruit),accepted);return 13;
  }
 }
 int owner=-1,published=portal_published_owner(recruit,context,&owner);
 if(published<0){portal_fault("published_admission_owner");return 0;}
 if(!published)return 0;
 *known=1;unsigned tid=scalar(team,383,0x100);
 int hs=scalar(recruit,2,0x108)<=3;
 int blocked=hs || owner!=(int)tid;
 log_line("PUBLISHED_ADMISSION,%s,%u,%u,%d,%s",site,tid,row(recruit),owner,
          owner!=(int)tid?"RIVAL_BLOCKED":hs?"HS_ALREADY_ACQUIRED":"OWNER_RETAINED");
 return blocked?13:0;
}
/* RecruitingTunables.OVRProjection is the native per-college-year increment.
   Native 0818d5c7..0818d5da projects to remaining-career horizon. This planner
   deliberately uses ONE step for next season, not the whole remaining career.
   It is an estimate, not a prediction of the randomized development engine. */
static int portal_projection_step(U context) {
 Rec tuning=(Rec)q(context+0xd0);
 if(!tuning)return 0; /* Conservative known-rating fallback for absent context. */
 unsigned value=scalar(tuning,39,0x100);
 if(value>20){portal_fault("projection_tuning");return 0;}
 return (int)value;
}
static int portal_projected_ovr(int ovr,int step) {return ovr+step>99?99:ovr+step;}
/* Only break equal projected QB ratings with actual recent usage. Development
   potential alone must not demote an established equal-rated QB1. */
static int r3_qb_priority(Rec p) {
 Rec a=array_field(p,243);U n=a?array_count(a):0;int latest=-1,starts=0,downs=0;
 unsigned tid=scalar(p,261,0x100);
 for(U j=0;j<n && n<=128;j++) {
  Rec st=array_at(a,j);U desc=st?schema(st):0;const char *name=desc?(const char *)q(desc+0x18):0;
  if(name && !strcmp(name,"SeasonOffensiveStats") && scalar(st,2,0x100)==tid) {
   int year=(int)scalar(st,0,0x100),s=(int)scalar(st,6,0x100),d=(int)scalar(st,3,0x100);
   if(year>=0 && s>=0 && s<=31 && d>=0 && d<=2047 && year>=latest) {
    if(year>latest){latest=year;starts=0;downs=0;}
    if(s>starts)starts=s;if(d>downs)downs=d;
   }
  }
  release(st);
 }
 release(a);return 2+4*(starts*2048+downs);
}
static int portal_add(PortalRoster *s,Rec player,int incoming,int signed_here,int develop) {
 int pos=(int)scalar(player,3,0x108);if(pos<0 || pos>=POSITIONS) return 0;
 if(pos!=s->pos) return 1;
 int ovr=(int)scalar(player,191,0x100);if(ovr<0 || ovr>99) return 0;
 U table=q((U)player+0x10);uint32_t id=row(player);
 for(int j=0;j<s->count;j++) if(s->players[j].table==table && s->players[j].row==id) {
  s->players[j].portal|=incoming;s->players[j].signed_here|=signed_here;s->players[j].develop|=develop;return 1;
 }
 if(s->count>=192) return 0;
 int young=pos==0 && dri_qb_young(ovr,(int)scalar(player,242,0x108),(int)scalar(player,271,0x108),develop && !incoming);
 s->players[s->count++]=(PortalPlayer){table,id,ovr,incoming,signed_here,develop,young,
   pos==0 && develop && !incoming?r3_qb_priority(player)+young:0};return 1;
}
static int portal_signed_here(const PortalRoster *s,Rec player) {
 for(int i=0;i<s->count;i++)if(s->players[i].table==q((U)player+0x10) && s->players[i].row==row(player))return s->players[i].signed_here;
 return 0;
}
/* Test30: callback-local batch projection. There is no persistent acceptance
   cache: a fresh batch is collected whenever prepare/funding needs all rooms. */
static int portal_add_many(PortalRoster *rooms,int count,Rec player,int incoming,int signed_here,int develop) {
 if(count==1)return portal_add(rooms,player,incoming,signed_here,develop);
 int pos=(int)scalar(player,3,0x108);if(pos<0 || pos>=POSITIONS)return 0;
 return portal_add(&rooms[pos],player,incoming,signed_here,develop);
}
static int portal_signed_here_many(const PortalRoster *rooms,int count,Rec player) {
 if(count==1)return portal_signed_here(rooms,player);
 int pos=(int)scalar(player,3,0x108);
 return pos>=0 && pos<POSITIONS?portal_signed_here(&rooms[pos],player):0;
}
/* Franchise -> FranchiseEval -> PlayersLeavingEval -> final DraftClass.
   These are captured native ordinals, separately verified from serialized fields. */
static int portal_departures(Rec franchise,PortalDepartures *d) {
 memset(d,0,sizeof(*d));Rec fe=reference(franchise,9);if(!fe)return 0;
 Rec pl=reference(fe,13);release(fe);if(!pl)return 0;
 Rec a=array_field(pl,7);release(pl);if(!a)return 0;
 U n=array_count(a);if(n>2048){release(a);return 0;}int ok=1;
 for(U i=0;i<n;i++) {
  Rec e=array_at(a,i);if(!e){ok=0;break;}
  unsigned type=scalar(e,2,0x108),status=scalar(e,1,0x108);
  if(type>=1 && type<=7 && status!=1) {
   Rec p=reference(e,4);if(!p){release(e);ok=0;break;}
   U table=q((U)p+0x10);
   if(d->table && d->table!=table){release(p);release(e);ok=0;break;}
   d->table=table;d->ids[d->count++]=row(p);release(p);
  }
  release(e);
 }
 release(a);return ok;
}
static int portal_departed(Rec p,const PortalDepartures *d) {
 if(q((U)p+0x10)!=d->table)return 0;
 for(unsigned i=0;i<d->count;i++)if(d->ids[i]==row(p))return 1;
 return 0;
}
typedef struct {const char *source;int member;unsigned checked_rid;} PortalRoomIssue;
static int portal_room_issue(PortalRoomIssue *issue,const char *source,int member) {
 if(issue && !issue->source){issue->source=source;issue->member=member;}
 return 0;
}
static int portal_players(PortalRoster *s,int rooms,Rec team,int ordinal,int returning,const PortalDepartures *d,PortalRoomIssue *issue) {
 const char *source=returning?"RETURNING_ROSTER":"INCOMING_ROSTER";
 Rec a=array_field(team,ordinal);if(!a)return portal_room_issue(issue,source,-1);
 U n=array_count(a);if(n>192){release(a);return portal_room_issue(issue,source,-1);}
 unsigned tid=scalar(team,383,0x100);int ok=1;
 for(U i=0;i<n;i++) {
  Rec p=array_at(a,i);if(!p){ok=portal_room_issue(issue,source,(int)i);break;}
  int year=(int)scalar(p,242,0x108);
  if(year<0 || year>3) ok=0;
  /* Confirmed NFL departures can still carry their old TeamIndex in TP1. */
  if(ok && (!returning || (year<3 && scalar(p,261,0x100)==tid && !portal_departed(p,d)))) ok=portal_add_many(s,rooms,p,0,!returning,returning);
  if(!ok)portal_room_issue(issue,source,(int)row(p));
  release(p);if(!ok) break;
 }
 release(a);return ok;
}
static int portal_commits(PortalRoster *s,int rooms,Rec franchise,Rec team,int ordinal,int portal,U context,PortalRoomIssue *issue) {
 const char *source=portal?"PORTAL_COMMITMENTS":"HS_COMMITMENTS";
 Rec a=array_field(franchise,ordinal);if(!a)return portal_room_issue(issue,source,-1);
 U n=array_count(a);if(n>12000){release(a);return portal_room_issue(issue,source,-1);}
 int ok=1;
 for(U i=0;i<n;i++) {
  Rec r=array_at(a,i);if(!r){ok=portal_room_issue(issue,source,(int)i);break;}
  unsigned stage=scalar(r,9,0x108);
  if(stage==5 || stage==6) {
   Rec p=reference(r,5);if(!p) ok=0;
   else {
    int owns=0,tid=-1;
    if(stage==5) {
     if(portal) {
      int valid=portal_hard_owner(r,context,&tid);
      if(valid<0)ok=portal_room_issue(issue,"PORTAL_HARD_OWNER",(int)row(r));
      owns=valid>0 && tid==(int)scalar(team,383,0x100);
     } else {
      owns=own_commit(r,team);
      if(owns && mode==1 && !portal_store_owner(r,p,scalar(team,383,0x100)))
       ok=portal_room_issue(issue,"HS_COMMIT_OWNER",(int)row(r));
     }
    }
    else if(portal_signed_destination(r,p,&tid))owns=tid==(int)scalar(team,383,0x100);
    else owns=portal_signed_here_many(s,rooms,p);
    if(ok && owns)ok=portal_add_many(s,rooms,p,portal,stage==6,portal);
    release(p);
   }
  }
  if(!ok)portal_room_issue(issue,source,(int)row(r));
  release(r);if(!ok) break;
 }
 release(a);return ok;
}
static int portal_roster_finish(PortalRoster *s,int step) {
 PortalQuality *v=&s->quality;
 v->pos=s->pos;v->ideal=s->ideal;v->starters=s->starters;v->count=s->count;v->incoming_best=-1;
 for(int j=0;j<s->count;j++) {
  PortalPlayer p=s->players[j];v->rating[j]=p.ovr;v->incoming[j]=p.portal;
  if(!p.portal)v->base_count++;
  if(!p.portal && !p.develop)v->hs_count++;
  if(p.ovr>s->best)s->best=p.ovr;
  if(p.portal && p.ovr>s->incoming_best)s->incoming_best=p.ovr;
  dri_qb_observe(v,p.ovr,p.ovr,p.qb_young && !p.portal,p.qb_priority);
 }
 v->incoming_best=s->incoming_best;
 s->projected=*v;
 s->projected.incoming_best=-1;
 s->projected.qb_leader_rating=s->projected.qb_leader_priority=s->projected.qb_young_leader=0;
 for(int j=0;j<s->count;j++) {
  PortalPlayer *p=&s->players[j];
  s->projected.rating[j]=portal_projected_ovr(p->ovr,p->develop?step:0);
  if(p->portal && s->projected.rating[j]>s->projected.incoming_best)s->projected.incoming_best=s->projected.rating[j];
  dri_qb_observe(&s->projected,p->ovr,s->projected.rating[j],p->qb_young && !p->portal,p->qb_priority);
 }
 for(int which=0;which<2;which++) {
  PortalQuality *a=which?&s->projected:v;
  for(int j=1;j<s->count;j++) {
   int value=a->rating[j],incoming=a->incoming[j],k=j;
   while(k>0 && (a->rating[k-1]<value || (a->rating[k-1]==value && a->incoming[k-1]<incoming))) {
    a->rating[k]=a->rating[k-1];a->incoming[k]=a->incoming[k-1];k--;
   }
   a->rating[k]=value;a->incoming[k]=incoming;
  }
 }
 s->role=s->count?v->rating[(s->starters<s->count?s->starters:s->count)-1]:0;
 return 1;
}
static int portal_rosters_collect(PortalRoster *s,int rooms,Rec team,U context,int single_pos,PortalRoomIssue *issue) {
 static const int fields[POSITIONS]={5,2,1,9,8,4,3,0,6,7,3,6,1,4,5,7,0,2,8,1,6,4};
 if((rooms!=1 && rooms!=POSITIONS) || (rooms==1 && (single_pos<0 || single_pos>=POSITIONS)))
  return portal_room_issue(issue,"POSITION",single_pos);
 memset(s,0,sizeof(*s)*(size_t)rooms);
 Rec cfg=reference(team,64);if(!cfg)return portal_room_issue(issue,"SCHEME",-1);
 for(int j=0;j<rooms;j++) {
  int pos=rooms==1?single_pos:j;PortalRoster *r=&s[j];r->pos=pos;r->incoming_best=-1;
  Rec group=pos<10?reference(cfg,5):(pos<19?reference(cfg,0):0);
  Rec parent=pos<19?group:cfg;
  Rec entry=parent?reference(parent,fields[pos]):0;
  if(!entry){release(group);release(cfg);return portal_room_issue(issue,"SCHEME_POSITION",pos);}
  r->ideal=(int)scalar(entry,2,0x100);
  r->starters=(int)scalar(entry,4,0x100)+(int)scalar(entry,0,0x100);
  release(entry);release(group);
  if(r->ideal<0 || r->ideal>30 || r->starters<0 || r->starters>11){
   release(cfg);return portal_room_issue(issue,"SCHEME_LIMITS",pos);
  }
  if(!r->starters)r->starters=1;
 }
 release(cfg);
 Rec f=context_ref(context,6);if(!f)return portal_room_issue(issue,"FRANCHISE",-1);
 PortalDepartures d;int prestige=(int)scalar(team,384,0x100);
 int ok;
 if(prestige<0 || prestige>10)ok=portal_room_issue(issue,"PRESTIGE",prestige);
 else if(!portal_ownership_scope(f))ok=portal_room_issue(issue,"OWNERSHIP_SCOPE",-1);
 else if(!portal_departures(f,&d))ok=portal_room_issue(issue,"DRAFT_DEPARTURES",-1);
 else ok=portal_players(s,rooms,team,239,1,&d,issue) && portal_players(s,rooms,team,33,0,&d,issue) &&
         portal_commits(s,rooms,f,team,39,0,context,issue) && portal_commits(s,rooms,f,team,46,1,context,issue);
 release(f);if(!ok)return 0;
 int step=portal_projection_step(context),annual[POSITIONS]={0};if(faulted)return 0;
 if(class31_team_annual(team,context,annual)<0)return portal_room_issue(issue,"ANNUAL_CLASS_TARGET",-1);
 for(int j=0;j<rooms;j++){
  s[j].prestige=prestige;if(!portal_roster_finish(&s[j],step))return 0;
  s[j].quality.hs_normal_limit=annual[s[j].pos];s[j].projected.hs_normal_limit=annual[s[j].pos];
  s[j].quality.hs_annual_valid=s[j].projected.hs_annual_valid=1;
 }
 return 1;
}
static int portal_roster(PortalRoster *s,Rec team,U context,int pos) {
 return portal_rosters_collect(s,1,team,context,pos,0);
}
static int portal_quality_rooms(PortalQuality *projected,PortalQuality *current,int *prestige,Rec team,U context) {
 PortalRoster *rooms=HeapAlloc(GetProcessHeap(),0,sizeof(*rooms)*POSITIONS);if(!rooms)return 0;
 int ok=portal_rosters_collect(rooms,POSITIONS,team,context,0,0);
 if(ok)for(int p=0;p<POSITIONS;p++){
  projected[p]=rooms[p].projected;if(current)current[p]=rooms[p].quality;
 }
 if(ok && prestige)*prestige=rooms[0].prestige;
 HeapFree(GetProcessHeap(),0,rooms);return ok;
}
/* Opening/follower authorization deliberately bypasses ordinary attention
   rules. It must still honor the R3 QB role guard, on a fresh complete room. */
static int r3_qb_admission(Rec recruit,Rec team,U context,const char *site) {
 if(mode!=1 || faulted || !recruit || !team || !transfer_recruit(recruit) || scalar(recruit,9,0x108)>=5)return 0;
 Rec human=reference(team,407);if(human){release(human);return 0;}
 Rec p=reference(recruit,5);if(!p)return 7;
 int pos=(int)scalar(p,3,0x108),raw=(int)scalar(p,191,0x100);release(p);
 if(pos!=0)return 0;
 PortalRoster room;
 if(raw<0 || raw>99 || !portal_roster(&room,team,context,0)){portal_fault("R3_QB_admission_room");return 7;}
 int block=dri_qb_block(&room.projected,portal_projected_ovr(raw,portal_projection_step(context)),raw);
 if(block)log_line("R3_QB_ADMISSION_BLOCK,%s,%u,%u,%d,%d,%d,%d",site,scalar(team,383,0x100),row(recruit),raw,
  room.projected.count?room.projected.rating[0]:0,room.projected.qb_young_leader,block);
 return block;
}
#include "hs15.inc"
static int test29_hs_reserved(Rec team,U eval,int reserved[22]);
static int portal29_admit(Rec team,Rec recruit,U eval,int verified_score);
static U academy29_return_eval;
static Rec academy29_return_destination(Rec recruit,U eval);
/* Only the optional survey of existing funded offers supplies an issue.
   All admission and signing callers keep the established failure behavior. */
static int portal_room_unavailable(PortalRoomIssue *issue,int optional,const char *reason,
                                   const char *site,unsigned tid,unsigned rid,int pos) {
 if(faulted || mode<0)return 7;
 if(!issue->source)portal_room_issue(issue,reason,-1);
 if(optional){issue->checked_rid=rid;return 7;}
 log_line("RM18R22_PORTAL_ROOM_FAILURE,%s,%u,%u,%d,%s,%d",site,tid,rid,pos,issue->source,issue->member);
 portal_fault(reason);
 return 0;
}
static int portal_decision_checked(Rec recruit,Rec team,U context,const char *site,PortalRoomIssue *optional) {
 if(!recruit || !team || mode<0 || faulted || !transfer_recruit(recruit)) return 0;
 unsigned tid=scalar(team,383,0x100);
 if(tid>=138){portal_fault("candidate_team");return mode==1?7:0;}
 /* Applies equally to CPU and user academies, before secured-stage bypasses. */
 if(!portal_academy_allowed(recruit,tid)) {
  log_line("PORTAL_ACADEMY_BLOCK,%s,%u,%u,%s",site,tid,row(recruit),mode?"ACTIVE":"DIAGNOSTIC");
  return mode==1?5:0;
 }
 unsigned stage=scalar(recruit,9,0x108);
 if(stage<5 && portal_academy(tid))return mode==1?5:0;
 if(stage==5) {
  int owner=-1,valid=portal_hard_owner(recruit,context,&owner);
  if(valid<0){portal_fault("commitment_owner");return mode==1?7:0;}
  int blocked=valid>0?(owner==(int)tid?0:6):7;
  if(blocked)log_line("PORTAL_COMMIT_BLOCK,%s,%u,%u,%d,%d,%s",site,tid,row(recruit),owner,blocked,mode?"ACTIVE":"DIAGNOSTIC");
  return mode==1?blocked:0;
 }
 if(stage>=6)return 0; /* Published ownership is handled by cleanup, not reassigned. */
 Rec user=reference(team,407);if(user){release(user);return 0;}
 Rec p=reference(recruit,5);if(!p){portal_fault("candidate_player");return 0;}
 int pos=(int)scalar(p,3,0x108),ovr=(int)scalar(p,191,0x100);release(p);
 if(pos<0 || pos>=POSITIONS || ovr<0 || ovr>99){portal_fault("candidate_values");return 0;}
 PortalRoster s;
 PortalRoomIssue local={0},*issue=optional?optional:&local;
 if(!portal_rosters_collect(&s,1,team,context,pos,issue))
  return portal_room_unavailable(issue,optional!=0,"roster_decode",site,tid,row(recruit),pos);
 int projected=portal_projected_ovr(ovr,portal_projection_step(context));
 if(faulted || mode<0)return 0;
 int reserved[22]={0};
 test29_hs_reserved(team,context,reserved);if(faulted)return 0;
 if(s.ideal>0 && (s.count>=s.ideal || reserved[pos]>0)) {
  Hs15Snapshot hs;
  if(!hs15_collect(&hs,team,context))
   return portal_room_unavailable(issue,optional!=0,"team_budget_snapshot",site,tid,row(recruit),pos);
  PortalQuality rooms[22];
  if(!hs15_portal_rooms(&hs,rooms,s.prestige,portal_projection_step(context)))
   return portal_room_unavailable(issue,optional!=0,"team_budget_rooms",site,tid,row(recruit),pos);
  dri_portal_set_hs_reservations(rooms,reserved,s.prestige);
  s.projected=rooms[pos];
 }
 int blocked=dri_quality_block_candidate(&s.projected,s.prestige,projected,ovr);
 log_line("PORTAL_ROLE,%s,%u,%u,%d,%d,%d,%d,%d,%d,%d,%d,%s",site,scalar(team,383,0x100),row(recruit),pos,ovr,s.ideal,s.count,s.role,s.best,s.incoming_best,blocked,mode?"ACTIVE":"DIAGNOSTIC");
 return mode==1?blocked:0;
}
static int portal_decision(Rec recruit,Rec team,U context,const char *site) {
 return portal_decision_checked(recruit,team,context,site,0);
}
static int portal_commitment_block(Rec recruit,Rec team,U context,const char *site);
static int hs28_commit_block(Rec recruit,Rec team,U context,const char *site);
static void hs28_note(Rec recruit,Rec team);
static int hs29_stage_block(Rec recruit,U eval,unsigned next,Wrap *transition);
static int test29r1_capacity_block(Rec recruit,Rec team,U eval,const char *site);
static int test29r2_portal_committable(Rec recruit,Rec team,U eval,const char *site);
static int cpu_recruit_decision(Rec recruit,Rec team,U context,const char *site,int accepting) {
 if(!recruit || !team || !context || faulted || mode<0)return 0;
 if(accepting) {
  int known=0,blocked=portal_existing_admission(recruit,team,context,site,&known);
  if(faulted || mode<0)return 0;
  if(known)return blocked;
 }
 int blocked=transfer_recruit(recruit)?portal_decision(recruit,team,context,site):
        hs15_decision(recruit,team,context,site,accepting);
 if(!blocked && accepting && transfer_recruit(recruit))blocked=portal_commitment_block(recruit,team,context,site);
 if(!blocked && accepting && hs15_is_recruit(recruit))blocked=hs28_commit_block(recruit,team,context,site);
 if(!blocked && accepting)blocked=test29r1_capacity_block(recruit,team,context,site);
 /* A helper may disable the runtime while decoding its inputs. Its error
    result is not a native recruiting veto, even if it captured active mode
    before the failure. Every caller must preserve native behavior then. */
 return !faulted && mode==1?blocked:0;
}
static Rec portal_leader(Rec recruit,U context) {
 Rec top=array_field(recruit,13);if(!top) return 0;
 Rec e=array_count(top)?array_at(top,0):0;release(top);if(!e)return 0;
 unsigned tid=scalar(e,0,0x100);release(e);if(tid>=138)return 0;
 Rec f=context_ref(context,6);if(!f)return 0;
 Rec teams=array_field(f,45);release(f);if(!teams)return 0;
 Rec team=tid<array_count(teams)?array_at(teams,tid):0;release(teams);
 if(team && scalar(team,383,0x100)!=tid){release(team);portal_fault("leader_team");return 0;}
 return team;
}
int DriOnPortalStars(Registers *r) {
 cache_hook_entry(CACHE_HOOK_OTHER);
 if(mode<0 || faulted)return 0;
 __try {
  Rec p=(Rec)q(r->rdx);if(!p){portal_fault("star_player");return 0;}
  int ovr=(int)scalar(p,191,0x100),year=(int)scalar(p,242,0x108),pos=(int)scalar(p,3,0x108);
  if(ovr<0 || ovr>99 || year<0 || year>3 || pos<0 || pos>=POSITIONS){portal_fault("star_values");return 0;}
  int star=dri_portal_star(ovr,year),old=(int)scalar(p,231,0x108);
  log_line("PORTAL_STAR,%u,%d,%d,%d,%d,%d,%s",row(p),pos,year,ovr,old+1,star+1,mode?"APPLIED":"WOULD_APPLY");
  if(!mode)return 0;
  U t=q((U)p+0x10),f=field(schema(p),231);
  ((void(*)(U,uint32_t,U,int))q(q(t)+0x148))(t,row(p),f,star);cache_note_write();
  ((void(*)(U))(game_base+0x080595e0))(t);
  if(scalar(p,231,0x108)!=(unsigned)star){portal_fault("star_write");return 0;}
  return 1;
 } __except(fail_exception("portal_stars",GetExceptionCode())){return 0;}
}
/* Called immediately after the native transition condition. A veto makes the
   original condition false, so its existing loop cleanup runs normally. */
void DriOnPortalWeekly(Registers *r) {
 cache_hook_entry(CACHE_HOOK_OTHER);
 if(mode<0 || faulted || !(r->rax&255))return;
 __try {
  U sp=(U)(r+1);Rec transition=(Rec)q(sp+0x38);
  if(scalar(transition,1,0x108)!=5)return;
  Rec recruit=(Rec)q(r->rbp+0x68);if(!recruit)return;
  U context=q(r->rbp+0x60);Rec team=portal_leader(recruit,context);
  if(!team){portal_fault("weekly_leader");return;}
  int blocked=cpu_recruit_decision(recruit,team,context,"WEEKLY",1);
  if(faulted || mode<0){release(team);return;}
  if(!blocked && transfer_recruit(recruit)) {
   int offer=portal_active_offer(recruit,team);
   if(offer<0){portal_fault("weekly_offer");release(team);return;}
   if(offer<=0){blocked=7;log_line("PORTAL_INVALID_COMMIT,WEEKLY,%u,%u,NO_ACTIVE_OFFER",scalar(team,383,0x100),row(recruit));}
  }
  if(blocked && mode==1 && !faulted)r->rax&=~(U)255;
  else portal_note_owner(recruit,team,context,"WEEKLY",1);
  release(team);
 } __except(fail_exception("portal_weekly",GetExceptionCode())){}
}
/* Native HS transitions run before the portal loop. A CPU policy rejection
   uses the original failed-condition cleanup; accepted ownership is observed
   before publication and counted only after the native stage actually changes. */
int DriOnHsOwnership(Registers *r) {
 cache_hook_entry(CACHE_HOOK_OTHER);
 if(mode!=1 || faulted)return 0;
 __try {
  U sp=(U)(r+1);Rec transition=(Rec)q(sp+0x38);
  if(!transition)return 0;
  unsigned next=scalar(transition,1,0x108);
  Rec recruit=(Rec)q(r->rbp+0x68);
  if(!recruit || transfer_recruit(recruit) || scalar(recruit,9,0x108)>=6)return 0;
  U context=q(r->rbp+0x60);
  if(next>=1 && next<=4)return hs29_stage_block(recruit,context,next,(Wrap*)(sp+0x30));
  if(next!=5 && next!=6)return 0;
  Rec team=portal_leader(recruit,context);
  if(!team){portal_fault("HS_transition_leader");return 0;}
  int blocked=cpu_recruit_decision(recruit,team,context,"HS_WEEKLY",1);
  if(next==5 && scalar(recruit,9,0x108)<5 && !blocked && !faulted){
   int offer=portal_active_offer(recruit,team);
   if(offer<0)portal_fault("HS_transition_offer");
   else if(!offer){blocked=18;log_line("HS29_INVALID_COMMIT,%u,%u,NO_ACTIVE_OFFER",scalar(team,383,0x100),row(recruit));}
  }
  if(faulted || mode!=1){release(team);return 0;}
  if(!blocked){portal_note_owner(recruit,team,context,"HS_WEEKLY",next==5);hs28_note(recruit,team);}
  release(team);
  return !faulted && mode==1 && blocked!=0;
 } __except(fail_exception("HS_transition_owner",GetExceptionCode())){return 0;}
}
/* Native NSD bulk finalization signs both SoftCommitted and HardCommitted
   recruits. At 0815F3B1 the native collector and leader/team check have already
   selected the destination. Observe that exact Team record, before Signed=6
   hides native ownership and before Team.CommittedPlayers is published.
   No policy veto or stage write; the ledger is used only after native signing. */
void DriOnBulkOwnership(Registers *r) {
 cache_hook_entry(CACHE_HOOK_OTHER);
 if(mode!=1 || faulted)return;
 __try {
  portal_note_signing((Rec)q(r->rbp-0x19),(Rec)r->r12,r->r13,"SIGN_BULK");
 } __except(fail_exception("bulk_signing_owner",GetExceptionCode())){}
}
int DriOnPortalInstant(Registers *r) {
 cache_hook_entry(CACHE_HOOK_OTHER);
 if(mode<0 || faulted)return 0;
 __try {
  Rec recruit=(Rec)q(r->rbp-0x41),team=(Rec)q(q(r->rbp+0x77));U context=q(r->rbp+0x67);
  int designated=t36_designated && t36_active_eval && portal_same_record(recruit,t36_designated) && portal_same_record(team,t36_destination);
  if(designated)context=t36_active_eval;
  int blocked=designated?r3_qb_admission(recruit,team,context,"OPENING_INSTANT"):cpu_recruit_decision(recruit,team,context,"INSTANT",1);
  if(faulted || mode<0)return 0;
  if(!blocked && transfer_recruit(recruit)) {
   int offer=portal_active_offer(recruit,team);
   if(offer<0){portal_fault("instant_offer");return 0;}
   if(offer<=0 && mode==1)blocked=7;
  }
  if(!blocked){portal_note_owner(recruit,team,context,"INSTANT",1);hs28_note(recruit,team);}
  return !faulted && mode==1?blocked:0;
 }
 __except(fail_exception("portal_instant",GetExceptionCode())){return 0;}
}
/* Before native NSD inserts a soft commitment into its destination map. */
int DriOnPortalBulkGuard(Registers *r) {
 cache_hook_entry(CACHE_HOOK_OTHER);
 if(mode<0 || faulted)return 0;
 __try {
  Rec recruit=(Rec)q(r->rbp-0x19),team=(Rec)r->r12;U context=r->r13;
  if(!recruit || !team)return 0;
  int known=0,existing=portal_existing_admission(recruit,team,context,"SIGN_BULK_GUARD",&known);
  if(faulted || mode<0)return 0;
  /* Published HS players are finalized separately without another append.
     Existing portal owners retain their native finalization path. */
  if(known)return existing;
  int blocked=cpu_recruit_decision(recruit,team,context,"SIGN_BULK_GUARD",1);
  if(faulted || mode<0)return 0;
  if(!blocked && transfer_recruit(recruit) && scalar(recruit,9,0x108)==4) {
   int offer=portal_active_offer(recruit,team);
   if(offer<0){portal_fault("bulk_offer");return 0;}
   if(offer<=0 && mode==1)blocked=7;
  }
  /* Test31: a valid native soft choice may sign at its selected school.
     Rejected soft choices remain eligible for offered-school fallback. The
     pre-publication validator removes their earlier, unaccepted collection. */
  return !faulted && mode==1?blocked:0;
 } __except(fail_exception("bulk_guard",GetExceptionCode())){return 0;}
}
int DriOnPortalSigning(Registers *r) {
 cache_hook_entry(CACHE_HOOK_OTHER);
 if(mode<0 || faulted)return 0;
 __try {
  Rec recruit=(Rec)q((U)(r+1)+0x30),team=(Rec)r->rbx;U context=q(r->rbp+0x57);
  int blocked=cpu_recruit_decision(recruit,team,context,"SIGN_BOARD",1);
  if(faulted || mode<0)return 0;
  if(!blocked)portal_note_signing(recruit,team,context,"SIGN_BOARD");
  return !faulted && mode==1?blocked:0;
 }
 __except(fail_exception("portal_signing",GetExceptionCode())){return 0;}
}
int DriOnPortalDeadline(Registers *r) {
 cache_hook_entry(CACHE_HOOK_OTHER);
 /* Moved guard includes native null-winner branch, including diagnostic mode. */
 if(mode<0 || faulted)return r->r15?0:-1;
 __try {
  U sp=(U)(r+1),context=q(r->rbp+0x40);Rec recruit=(Rec)q(sp+0x30),team=(Rec)r->r15;
  if(mode==1 && academy29_return_eval==context) {
   Rec origin=academy29_return_destination(recruit,context);
   if(faulted){release(origin);return team?1:-1;}
   if(origin) {
    release(team);team=origin;r->r15=(U)origin;*(U*)(sp+0x38)=(U)origin;
    portal_note_signing(recruit,team,context,"ACADEMY_RETURN");
    log_line("TEST29R4_ACADEMY_RETURN,%u,%u,NATIVE_FINALIZATION_PENDING",row(recruit),scalar(team,383,0x100));
    return faulted?1:0;
   }
  }
  if(!team)return -1;
  int blocked=cpu_recruit_decision(recruit,team,context,"SIGN_PORTAL",1);
  if(faulted || mode<0)return 0;
  if(!blocked)portal_note_signing(recruit,team,context,"SIGN_PORTAL");
  return !faulted && mode==1?blocked:0;
 } __except(fail_exception("portal_deadline",GetExceptionCode())){return r->r15?0:-1;}
}
/* Runs before EvaluateTeamRecruitingBoard obtains its native iterators. */
static void portal_prune(Rec team,U eval) {
 Rec a=board_array(team);if(!a){portal_fault("board");return;}
 U n=array_count(a);if(n>MAX_BOARD){release(a);portal_fault("board_size");return;}
 Rec held[MAX_BOARD]={0};
 for(U i=0;i<n;i++){Rec t=array_at(a,i);if(t){held[i]=reference(t,10);release(t);}}
 release(a);
 for(U i=0;i<n;i++) {
  Rec recruit=held[i];if(!recruit)continue;
  int blocked=!faulted && mode>=0?cpu_recruit_decision(recruit,team,eval,"BOARD",0):0;
  /* Recheck after decoding: a fault must never become a destructive removal. */
  if(blocked && !faulted && mode==1) {
   U candidate[6]={(U)recruit,0,0,0,0,0};
   int rc=((int(*)(U,Rec*,void*))(game_base+0x081a7b10))(q(eval+0xc8),&team,candidate);cache_note_removal();
   log_line("PORTAL_DROP,%u,%u,%d",scalar(team,383,0x100),row(recruit),rc);
   if(rc)portal_fault("native_remove");
  }
  release(recruit);
 }
}
