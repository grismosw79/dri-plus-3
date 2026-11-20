/* Test28 projected opportunity. This module reads secured roster facts only.
   It does not write player ratings, recruit stages, board entries or owners. */
#include "pt28_core.h"
U ResumePt28Grade;
extern void HookPt28Grade(void);
extern int NativePt28Grade(U,Rec *,Rec *);
#define PT28_OWNER_SLOTS 65536
#define PT28_PENDING_MAX 16384
typedef struct {U table;uint32_t id;int owner;} Pt28Owner;
typedef struct {PortalSign signing;unsigned ordinal,next;} Pt28Pending;
typedef struct {
 U table;uint32_t id;unsigned league;int year,week,type,stage,offseason;LONG epoch;int valid;
 Pt28Owner owners[PT28_OWNER_SLOTS];
 PortalDepartures departed;
 Pt28Pending pending[PT28_PENDING_MAX];unsigned pending_count;
 unsigned pending_heads[PORTAL_SIGN_SLOTS]; /* One-based indices into pending. */
} Pt28Index;
static Pt28Index pt28_index;
static SRWLOCK pt28_lock=SRWLOCK_INIT;
static volatile LONG pt28_epoch=1;
static volatile LONG pt28_wait_logs=0;
static struct {U table;uint32_t id;int year,week,type,stage,offseason;unsigned samples[2][22];} pt28_log_scope;
static void pt28_invalidate(void){InterlockedIncrement(&pt28_epoch);}
static void pt28_reset(void){
 memset(&pt28_index,0,sizeof(pt28_index));
}
static int pt28_group(int pos){
 /* Captured native fine-position mapping at 07e13560. The same mapping is
    applied to competitors and job counts; broad native groups are not used. */
 static const unsigned char group[22]={0,1,2,3,4,5,6,7,6,5,8,8,9,10,11,10,12,13,14,15,16,17};
 return pos>=0&&pos<22?group[pos]:-1;
}
static Pt28Owner *pt28_owner_slot(U table,uint32_t id,int create){
 unsigned k=(unsigned)((table>>4)^((U)id*2654435761u))&(PT28_OWNER_SLOTS-1);
 for(unsigned i=0;i<PT28_OWNER_SLOTS;i++,k=(k+1)&(PT28_OWNER_SLOTS-1)){
  Pt28Owner *s=&pt28_index.owners[k];
  if(s->table==table&&s->id==id)return s;
  if(!s->table)return create?s:0;
 }return 0;
}
static int pt28_published_owner(Rec player){
 Pt28Owner *s=pt28_owner_slot(q((U)player+0x10),row(player),0);return s?s->owner:-1;
}
static int pt28_ledger_owner(Rec recruit,Rec player){
 U table=q((U)recruit+0x10),player_table=q((U)player+0x10);
 uint32_t rid=row(recruit),pid=row(player);
 int owner=-1;AcquireSRWLockExclusive(&ownership_lock);
 if(signing_owners.table==pt28_index.table&&signing_owners.rid==pt28_index.id&&signing_owners.league==pt28_index.league&&
    signing_owners.year==(unsigned)pt28_index.year&&signing_owners.week==(unsigned)pt28_index.week&&
    signing_owners.type==(unsigned)pt28_index.type){
  PortalSign *s=portal_sign_slot(table,rid,0);
  if(s&&player_table&&s->player_table==player_table&&s->pid==pid&&s->tid<138)owner=(int)s->tid;
 }
 ReleaseSRWLockExclusive(&ownership_lock);return owner;
}
static int pt28_read_departures(Rec franchise){
 /* A missing/unpopulated PLV evaluator is legitimate before PLV starts.
    Once the array exists, malformed entries remain a hard validation failure. */
 PortalDepartures *d=&pt28_index.departed;memset(d,0,sizeof(*d));
 Rec fe=reference(franchise,9);if(!fe)return 1;
 Rec pl=reference(fe,13);release(fe);if(!pl)return 1;
 Rec a=array_field(pl,7);release(pl);if(!a)return 1;
 U n=array_count(a);if(n>2048){release(a);return 0;}int ok=1;
 for(U i=0;i<n;i++){
  Rec e=array_at(a,i);if(!e){ok=0;break;}
  unsigned kind=scalar(e,2,0x108),status=scalar(e,1,0x108);
  if(kind>=1&&kind<=7&&status!=1){
   Rec p=reference(e,4);if(!p){release(e);ok=0;break;}
   U table=q((U)p+0x10);
   if(d->table&&d->table!=table){release(p);release(e);ok=0;break;}
   d->table=table;d->ids[d->count++]=row(p);release(p);
  }release(e);
 }release(a);return ok;
}
static int pt28_ensure(Rec franchise,int year,int week,int type,int stage,int offseason){
 LONG epoch=InterlockedCompareExchange(&pt28_epoch,0,0);U table=q((U)franchise+0x10);uint32_t id=row(franchise);
 unsigned league=scalar(franchise,28,0x100);
 if(pt28_index.valid&&pt28_index.table==table&&pt28_index.id==id&&pt28_index.league==league&&pt28_index.year==year&&
    pt28_index.week==week&&pt28_index.type==type&&pt28_index.stage==stage&&pt28_index.offseason==offseason&&pt28_index.epoch==epoch)return 1;
 pt28_reset();pt28_index.league=league;
 pt28_index.table=table;pt28_index.id=id;pt28_index.year=year;pt28_index.week=week;
 pt28_index.type=type;pt28_index.stage=stage;pt28_index.offseason=offseason;pt28_index.epoch=epoch;
 Rec teams=array_field(franchise,45);if(!teams)return 0;
 U n=array_count(teams);if(n>138){release(teams);return 0;}int ok=1;
 for(U i=0;i<n&&ok;i++){
  Rec team=array_at(teams,i);if(!team){ok=0;break;}
  unsigned tid=scalar(team,383,0x100);Rec a=array_field(team,33);
  if(tid>=138||!a){release(a);release(team);ok=0;break;}
  U count=array_count(a);if(count>192){release(a);release(team);ok=0;break;}
  for(U j=0;j<count;j++){
   Rec p=array_at(a,j);if(!p){ok=0;break;}
   U table_p=q((U)p+0x10);uint32_t id_p=row(p);Pt28Owner *s=pt28_owner_slot(table_p,id_p,1);
   if(!s||(s->table&&s->owner!=(int)tid))ok=0;
   else *s=(Pt28Owner){table_p,id_p,(int)tid};
   release(p);if(!ok)break;
  }release(a);release(team);
 }release(teams);
 if(!ok||!pt28_read_departures(franchise))return 0;
 AcquireSRWLockExclusive(&ownership_lock);
 if(signing_owners.table==table&&signing_owners.rid==id&&signing_owners.league==league&&signing_owners.year==(unsigned)year&&
    signing_owners.week==(unsigned)week&&signing_owners.type==(unsigned)type){
  for(unsigned i=0;i<PORTAL_SIGN_SLOTS;i++){
   PortalSign *s=&signing_owners.entries[i];
   if(s->player_table&&s->tid<138){
    unsigned bucket=portal_sign_hash(s->table,s->rid);
    pt28_index.pending[pt28_index.pending_count++]=(Pt28Pending){*s,UINT32_MAX,pt28_index.pending_heads[bucket]};
    pt28_index.pending_heads[bucket]=pt28_index.pending_count;
   }
  }
 }ReleaseSRWLockExclusive(&ownership_lock);
 pt28_index.valid=1;
 if(pt28_log_scope.table!=table||pt28_log_scope.id!=id||pt28_log_scope.year!=year||
    pt28_log_scope.week!=week||pt28_log_scope.type!=type||pt28_log_scope.stage!=stage||pt28_log_scope.offseason!=offseason){
  memset(&pt28_log_scope,0,sizeof(pt28_log_scope));pt28_log_scope.table=table;pt28_log_scope.id=id;
  pt28_log_scope.year=year;pt28_log_scope.week=week;pt28_log_scope.type=type;pt28_log_scope.stage=stage;pt28_log_scope.offseason=offseason;
  log_line("PT28_INDEX,%d,%d,%d,%d,%d,%u,%u,SECURED_ROSTER_ONLY",year,week,type,stage,offseason,pt28_index.departed.count,pt28_index.pending_count);
 }
 return 1;
}
static int pt28_player(Rec player,int incoming,Pt28Player *p){
 unsigned previous=scalar(player,230,0x100),current=scalar(player,261,0x100);
 p->table=q((U)player+0x10);p->id=row(player);p->pos=(int)scalar(player,3,0x108);
 p->ovr=(int)scalar(player,191,0x100);p->year=(int)scalar(player,242,0x108);
 /* Native IsHS/JUCO is PrevTeamIndex == TeamIndex == 255. Published incoming
    players may already have TeamIndex assigned; previous 255 is still HS/JUCO.
    Their supplied school year is preserved (JUCO is not made a freshman). */
 p->hs=previous==255&&(current==255||incoming);p->develop=!p->hs;
 p->redshirt=scalar(player,234,0x108)==2;p->arrival_delay=0;p->rostered=0;
 return p->pos>=0&&p->pos<22&&p->ovr>=0&&p->ovr<=99&&p->year>=0&&p->year<=3;
}
static int pt28_add(Pt28Room *room,Rec player,int incoming,int group,int hs_override){
 Pt28Player p;if(!pt28_player(player,incoming,&p))return 0;
 if(pt28_group(p.pos)!=group)return 1;
 if(hs_override>=0){p.hs=hs_override;p.develop=!hs_override;}
 p.rostered=!incoming;
 for(int i=0;i<room->count;i++)if(room->players[i].table==p.table&&room->players[i].id==p.id){
  p.rostered|=room->players[i].rostered;
  if(incoming||hs_override>=0)room->players[i]=p;return 1;
 }
 if(room->count>=PT28_MAX_PLAYERS)return 0;
 room->players[room->count++]=p;return 1;
}
static int pt28_jobs(Rec team,int group,Pt28Room *room){
 static const int fields[22]={5,2,1,9,8,4,3,0,6,7,3,6,1,4,5,7,0,2,8,1,6,4};
 Rec cfg=reference(team,64);if(!cfg)return 0;
 Rec offense=reference(cfg,5),defense=reference(cfg,0);int ok=1;
 for(int pos=0;pos<22;pos++)if(pt28_group(pos)==group){
  Rec parent=pos<10?offense:pos<19?defense:cfg;
  Rec entry=parent?reference(parent,fields[pos]):0;if(!entry){ok=0;break;}
  PortalQuality v;memset(&v,0,sizeof(v));v.pos=pos;
  v.ideal=(int)scalar(entry,2,0x100);v.starters=(int)scalar(entry,4,0x100)+(int)scalar(entry,0,0x100);release(entry);
  if(v.ideal<0||v.ideal>30||v.starters<0||v.starters>11||(v.ideal>0&&v.starters>v.ideal)){ok=0;break;}
  /* No fabricated job when a scheme has zero ideal or zero configured slots. */
  if(v.ideal&&v.starters){room->starters+=v.starters;room->useful+=dri_portal_useful_depth(&v);}
  v.starters=0;v.ideal=22;if(dri_portal_useful_depth(&v)>=3)room->rotation=1;
 }
 release(defense);release(offense);release(cfg);return ok;
}
static int pt28_roster_array(Pt28Room *room,Rec team,int ordinal,int incoming,int group,int use_departures){
 Rec a=array_field(team,ordinal);if(!a)return 0;U n=array_count(a);
 if(n>192){release(a);return 0;}unsigned tid=scalar(team,383,0x100);int ok=1;
 for(U i=0;i<n;i++){
  Rec p=array_at(a,i);if(!p){ok=0;break;}
  if(incoming||(scalar(p,261,0x100)==tid&&(!use_departures||!portal_departed(p,&pt28_index.departed))))
   ok=pt28_add(room,p,incoming,group,-1);
  release(p);if(!ok)break;
 }release(a);return ok;
}
static int pt28_read_pending(Pt28Room *room,Rec recruit,Rec team,int group,int offered,const PortalSign *accepted){
 unsigned stage=scalar(recruit,9,0x108);if(stage!=5&&stage!=6)return 1;
 unsigned klass=scalar(recruit,2,0x108);if(klass>7)return 0;
 Rec player=reference(recruit,5);if(!player)return 0;
 if(accepted&&(q((U)player+0x10)!=accepted->player_table||row(player)!=accepted->pid)){release(player);return 1;}
 int tid=(int)scalar(team,383,0x100),owner=pt28_published_owner(player),owns=owner==tid;
 /* An accepted redirect may leave a stale board/leader on the former school. */
 int ledger_owner=accepted?(int)accepted->tid:pt28_ledger_owner(recruit,player);
 if(owner<0&&ledger_owner>=0)owns=ledger_owner==tid;
 if(owner<0&&ledger_owner<0&&stage==5){
  if(klass<=3)owns=own_commit(recruit,team);
  else if(offered&&!portal_academy((unsigned)tid)){
   Rec top=array_field(recruit,13);if(!top){release(player);return 0;}
   U n=array_count(top);if(n>64){release(top);release(player);return 0;}
   Rec leader=n?array_at(top,0):0;
   owns=leader&&scalar(leader,0,0x100)==(unsigned)tid;release(leader);release(top);
  }
 }
 int ok=!owns||pt28_add(room,player,1,group,klass<=3);release(player);return ok;
}
/* Resolve pending commitments from the current dynasty's HS or portal list.
   A cached ordinal is only a hint. Reindex the pool once if a row moved. */
static void pt28_index_pool(Rec pool,U count,unsigned field){
 for(U i=0;i<count;i++){
  Rec recruit=array_at(pool,i);if(!recruit)continue;
  U table=q((U)recruit+0x10);uint32_t rid=row(recruit);
  release(recruit);
  unsigned slot=pt28_index.pending_heads[portal_sign_hash(table,rid)];
  while(slot){
   Pt28Pending *entry=&pt28_index.pending[slot-1];
   if(entry->signing.table==table&&entry->signing.rid==rid&&entry->signing.pool_field==field){
    entry->ordinal=(unsigned)i;break;
   }
   slot=entry->next;
  }
 }
}
static Rec pt28_pending_at(Rec pool,U count,const Pt28Pending *entry){
 if(entry->ordinal>=count)return 0;
 Rec recruit=array_at(pool,entry->ordinal);
 if(recruit&&q((U)recruit+0x10)==entry->signing.table&&row(recruit)==entry->signing.rid)return recruit;
 release(recruit);return 0;
}
static int pt28_pending_pool(Pt28Room *room,Rec franchise,Rec team,int group,unsigned tid,unsigned field){
 Rec pool=0;U count=0;int indexed=0,ok=1;
 for(unsigned i=0;i<pt28_index.pending_count&&ok;i++){
  Pt28Pending *entry=&pt28_index.pending[i];
  if(entry->signing.tid!=tid||entry->signing.pool_field!=field)continue;
  if(!pool){
   pool=array_field(franchise,(int)field);if(!pool)return 1;
   count=array_count(pool);if(count>12000){ok=0;break;}
  }
  Rec recruit=pt28_pending_at(pool,count,entry);
  if(!recruit&&!indexed){
   pt28_index_pool(pool,count,field);indexed=1;recruit=pt28_pending_at(pool,count,entry);
  }
  if(recruit){ok=pt28_read_pending(room,recruit,team,group,0,&entry->signing);release(recruit);}
 }
 release(pool);return ok;
}
static int pt28_board(Pt28Room *room,Rec franchise,Rec team,int group){
 Rec board=reference(team,224);if(!board)return 1;
 Rec a=array_field(board,0);release(board);if(!a)return 1;U n=array_count(a);
 if(n>MAX_BOARD){release(a);return 0;}int ok=1;
 for(U i=0;i<n;i++){
  Rec target=array_at(a,i);if(!target){ok=0;break;}
  unsigned status=scalar(target,12,0x108);Rec recruit=reference(target,10);
  if(status>4||!recruit){release(recruit);release(target);ok=0;break;}
  /* Ledger entries are handled below, including records already removed from
     the board but not yet published by the native bulk signing operation. */
  ok=pt28_read_pending(room,recruit,team,group,status>=2,0);
  release(recruit);release(target);if(!ok)break;
 }release(a);if(!ok)return 0;
 unsigned tid=scalar(team,383,0x100);
 return pt28_pending_pool(room,franchise,team,group,tid,39)&&pt28_pending_pool(room,franchise,team,group,tid,46);
}
/* Scheduler/Appointment/RecurrenceRule are dynamic schemas: use raw field
   indices, not the native-enum field remapper used by franchise records. */
static uint32_t pt28_raw_scalar(Rec r,unsigned n,unsigned method){
 U table=q((U)r+0x10);
 if(method==0xf8)return ((unsigned char(*)(U,uint32_t,U))q(q(table)+method))(table,row(r),(U)n);
 return ((uint32_t(*)(U,uint32_t,U))q(q(table)+method))(table,row(r),(U)n);
}
static Rec pt28_raw_ref(Rec r,unsigned n){
 Rec out=0;U table=q((U)r+0x10);
 ((U(*)(U,Rec*,uint32_t,U))q(q(table)+0x128))(table,&out,row(r),(U)n);return out;
}
/* 1 stable, 0 legitimate in-progress advance, -1 malformed calendar.
   Stage8 is published before year advancement/training; it cannot certify
   completion. The training appointment becomes active after its StartEvent.
   The blocking training transaction holds the scheduler off the stopping
   lattice until every team has progressed. All reads are persisted facts. */
static int pt28_checkpoint(Rec franchise,Rec season){
 int stage=(int)scalar(season,3,0x108),offseason=(int)scalar(season,8,0x100);
 if(stage!=2||offseason!=8)return 1;
 if(scalar(season,4,0x100)!=7)return 0;
 Rec fe=reference(franchise,9),manager=fe?reference(fe,20):0;
 Rec scheduler=manager?reference(manager,8):0;release(manager);release(fe);
 if(!scheduler)return -1;
 int32_t before=(int32_t)pt28_raw_scalar(scheduler,1,0x100);
 Rec raw=pt28_raw_ref(scheduler,2),appointments=as_array(raw);release(raw);
 if(!appointments){release(scheduler);return -1;}
 U n=array_count(appointments);int ok=n>0&&n<=512;
 int stops=0,training=0,active=0;int32_t next=-1;int64_t interval=0;
 for(U i=0;i<n&&ok;i++){
  Rec appointment=array_at(appointments,i);if(!appointment){ok=0;break;}
  Rec event=pt28_raw_ref(appointment,5);
  if(event){
   const char *name=((const char*(*)(U))(game_base+0x08055dc0))(schema(event));
   if(!name)ok=0;
   else if(!strncmp(name,"TrainingResultsStartEvent",sizeof("TrainingResultsStartEvent"))){
    if(++training!=1)ok=0;
    active=(int)pt28_raw_scalar(appointment,2,0xf8);if(active<0||active>1)ok=0;
   }else if(!strncmp(name,"ProcessWeeklyGamesStartEvent",sizeof("ProcessWeeklyGamesStartEvent"))){
    if(++stops!=1)ok=0;
    next=(int32_t)pt28_raw_scalar(appointment,4,0x100);
    Rec rule=pt28_raw_ref(appointment,9);
    if(!rule)ok=0;
    else {
     unsigned pattern=pt28_raw_scalar(rule,0,0x108),unit=pt28_raw_scalar(rule,1,0x108);
     int32_t units=(int32_t)pt28_raw_scalar(rule,2,0x100);
     unsigned term=pt28_raw_scalar(rule,8,0x108);
     if(pattern!=0||unit!=3||units!=7||term!=0)ok=0;
     else interval=(int64_t)units*86400;
     release(rule);
    }
   }
   release(event);
  }
  release(appointment);
 }
 release(appointments);
 int32_t after=(int32_t)pt28_raw_scalar(scheduler,1,0x100);release(scheduler);
 if(!ok||stops!=1||training!=1||before<0||after<0||next<0||interval<=0)return -1;
 if(before!=after||!active)return 0;
 int64_t delta=(int64_t)next-after;
 return delta==0||delta==interval;
}
/* The supplied PLV/portal/NSD records precede class advancement. The phase
   resolver is kept separate so the verified native annual boundary is the
   only source of the current-versus-next-season decision. */
static int pt28_timing(Rec season,const Pt28Player *candidate,int *advance,int *development,int *delay_hs){
 int stage=(int)scalar(season,3,0x108);
 if(stage<0||stage>2)return 0;
 *delay_hs=0;
 if(stage!=2){
  /* During an active/preseason year, rostered college players evaluate the
     current season; a genuine HS/JUCO recruit evaluates his incoming season. */
  *advance=candidate->hs?1:0;*development=*advance;*delay_hs=!candidate->hs;return 1;
 }
 /* RecruitingOffSeasonWeekStartReaction SPEX 86:257 compares CurrentWeek
    to 7, advances every team/portal class, then advances redshirt status.
    The persisted week is valid after native event processing and on reload. */
 int week=(int)scalar(season,4,0x100);if(week<0||week>60)return 0;
 *advance=week<7;
 /* Class and rating timing remain distinct. Stage8 alone is not evidence
    that either callback completed: pt28_checkpoint rejects the intervening
    advance until training is active and the persisted clock reaches a stop. */
 int offseason=(int)scalar(season,8,0x100);if(offseason<0||offseason>15)return 0;
 *development=offseason<8;return 1;
}
static int pt28_grade_impl(U context,Rec team,Rec player,int *grade,Pt28Result *details){
 Rec franchise=context_ref(context,2);if(!franchise)return 0;
 Rec season=reference(franchise,44),tuning=context_ref(context,5);
 if(!season||!tuning){release(tuning);release(season);release(franchise);return 0;}
 int checkpoint=pt28_checkpoint(franchise,season);
 if(checkpoint<=0){
  if(checkpoint==0&&InterlockedIncrement(&pt28_wait_logs)<=8)
   log_line("PT28_WAIT,%u,%u,%u,NATIVE_DURING_TRAINING_ADVANCE",scalar(season,2,0x100),scalar(season,4,0x100),scalar(season,8,0x100));
  release(tuning);release(season);release(franchise);return checkpoint==0?-1:0;
 }
 int year=(int)scalar(season,2,0x100),week=(int)scalar(season,4,0x100),type=(int)scalar(season,5,0x108),stage=(int)scalar(season,3,0x108);
 int offseason=(int)scalar(season,8,0x100),advance=0,development=0,delay_hs=0;
 Pt28Player native_candidate;
 int timing=pt28_player(player,0,&native_candidate)&&pt28_timing(season,&native_candidate,&advance,&development,&delay_hs);
 int step=(int)scalar(tuning,39,0x100);release(tuning);release(season);
 unsigned tid=scalar(team,383,0x100);
 if(tid>=138||year<1900||year>2200||week<0||week>60||step<0||step>20||!timing){release(franchise);return 0;}
 int ok=pt28_ensure(franchise,year,week,type,stage,offseason);
 /* Persuasion can change a departure status without advancing the clock. */
 if(ok&&stage==2&&offseason==2)ok=pt28_read_departures(franchise);
 Pt28Player candidate;
 if(!ok||!pt28_player(player,pt28_published_owner(player)>=0,&candidate)){release(franchise);return 0;}
 Pt28Room room;memset(&room,0,sizeof(room));int group=pt28_group(candidate.pos);
 ok=pt28_jobs(team,group,&room)&&pt28_roster_array(&room,team,239,0,group,stage==2)&&
    pt28_roster_array(&room,team,33,1,group,0)&&pt28_board(&room,franchise,team,group);
 release(franchise);if(!ok)return 0;
 if(delay_hs)for(int i=0;i<room.count;i++){
  Pt28Player *p=&room.players[i];
  if(p->rostered){p->hs=0;p->develop=1;}
  else if(p->hs)p->arrival_delay=1;
 }
 for(int i=0;i<room.count;i++)if(room.players[i].table==candidate.table&&room.players[i].id==candidate.id){candidate=room.players[i];break;}
 Pt28Result result;if(!pt28_compute(&room,&candidate,step,advance,development,&result))return 0;
 int recruiting=scalar(player,261,0x100)!=tid;
 if(pt28_log_scope.samples[recruiting][candidate.pos]<3){
  pt28_log_scope.samples[recruiting][candidate.pos]++;
  log_line("PT28_OPPORTUNITY,%d,%d,%d,%u,%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%.4f,%.4f,%.4f,%d",
   year,week,recruiting,tid,candidate.id,candidate.pos,candidate.ovr,candidate.year,candidate.redshirt,advance,development,candidate.hs,
   result.projected_ovr,result.rank,result.peer_count,result.starters,result.useful,result.following_available,
   result.immediate,result.following,result.score,result.grade);
 }
 *grade=result.grade;if(details)*details=result;return 1;
}
static int pt28_grade(U context,Rec team,Rec player,int *grade){
 if(!context||!team||!player||!grade)return 0;
 int ok=0;AcquireSRWLockExclusive(&pt28_lock);
 __try {ok=pt28_grade_impl(context,team,player,grade,0);}
 __except(fail_exception("PT28_grade",GetExceptionCode())){ok=0;}
 ReleaseSRWLockExclusive(&pt28_lock);
 if(ok==0&&!faulted)portal_fault("PT28_inputs");return ok==1&&!faulted&&mode==1;
}
int DriPt28Grade(U context,Rec *team,Rec *player){
 if(mode==1&&!faulted&&team&&*team&&player&&*player){
  int ready=0;
  __try {
   /* Native UI initialization can request a grade before a Dynasty/SeasonInfo
      exists. Match the explicit league-start gate used by the EPS dispatcher. */
   Rec franchise=context?context_ref(context,2):0;
   Rec season=franchise?reference(franchise,44):0;
   ready=season&&scalar(season,24,0xf8);release(season);release(franchise);
  }__except(fail_exception("PT28_ready",GetExceptionCode())){ready=0;}
  if(ready){int grade;if(pt28_grade(context,*team,*player,&grade))return grade;}
 }
 return NativePt28Grade(context,team,player);
}
#ifdef DRI_EMULATION_TEST
__declspec(dllexport) int DriTestPt28Grade(U context,Rec team,Rec player,Pt28Result *out){
 int grade;return pt28_grade_impl(context,team,player,&grade,out);
}
__declspec(dllexport) void DriTestPt28Reset(void){pt28_reset();pt28_invalidate();}
__declspec(dllexport) int DriTestPt28Group(int pos){return pt28_group(pos);}
__declspec(dllexport) int DriTestPt28Checkpoint(Rec franchise,Rec season){return pt28_checkpoint(franchise,season);}
__declspec(dllexport) int DriTestPt28Timing(Rec season,int hs,int *out){
 Pt28Player p;memset(&p,0,sizeof(p));p.hs=hs;
 return pt28_timing(season,&p,out,out+1,out+2);
}
__declspec(dllexport) int DriTestPt28Hook(U context,Rec *team,Rec *player){return DriPt28Grade(context,team,player);}
__declspec(dllexport) void DriTestPt28Addresses(U *out){out[0]=(U)NativePt28Grade;out[1]=(U)HookPt28Grade;out[2]=(U)DriPt28Grade;}
#endif
