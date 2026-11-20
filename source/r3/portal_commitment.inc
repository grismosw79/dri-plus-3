/* Test25: acceptance reservations for the most recent per-opening plan.
   Numeric identities only; game records are reacquired from live boards. No
   destination, offer, stage, influence, player or roster is written here. */
typedef struct {U table;uint32_t rid;int pos,slot,round,priority;} PortalCommitEntry;
typedef struct {
 U franchise_table,franchise_handle,team_table;uint32_t franchise_row,team_row;
 unsigned year,week,type;int valid,count;
 PortalCommitEntry entries[MAX_BOARD];
} PortalCommitPlan;
static PortalCommitPlan commitment_plans[138];
static SRWLOCK commitment_lock=SRWLOCK_INIT;
static int portal_commit_scope(PortalCommitPlan *p,Rec team,U eval) {
 Rec f=context_ref(eval,6),s=f?reference(f,44):0;
 if(!f || !s){release(s);release(f);return 0;}
 p->franchise_table=q((U)f+0x10);p->franchise_handle=(U)f;p->franchise_row=row(f);
 p->team_table=q((U)team+0x10);p->team_row=row(team);
 p->year=scalar(s,2,0x100);p->week=scalar(s,4,0x100);p->type=scalar(s,5,0x108);
 release(s);release(f);return 1;
}
static void portal_commit_plan_store(const PortalChoice *pool,int n,Rec team,U eval,const char *site) {
 if(mode!=1 || faulted)return;
 unsigned tid=scalar(team,383,0x100);if(tid>=138)return;
 PortalCommitPlan plan={0};if(!portal_commit_scope(&plan,team,eval))return;
 if(plan.type!=6 || plan.week<2 || plan.week>6)return;
 for(int j=0;j<n;j++)if(pool[j].selected && pool[j].slot>0) {
  if(plan.count>=MAX_BOARD){portal_fault("commit_plan_capacity");return;}
  Rec rec=(Rec)pool[j].rec;
  plan.entries[plan.count++]=(PortalCommitEntry){q((U)rec+0x10),row(rec),pool[j].pos,pool[j].slot,pool[j].round,pool[j].priority};
 }
 plan.valid=1;
 AcquireSRWLockExclusive(&commitment_lock);commitment_plans[tid]=plan;ReleaseSRWLockExclusive(&commitment_lock);
 log_line("PORTAL_COMMIT_PLAN,%s,%u,%u,%d",site,tid,plan.week,plan.count);
}
static int portal_commit_plan_matches(const PortalCommitPlan *p,const PortalCommitPlan *now) {
 /* The final signing reaction may advance the week after its last funding
    pass. Permit that one transition, never a different year/save/team. */
 return p->valid && p->franchise_table==now->franchise_table && p->franchise_row==now->franchise_row &&
  p->team_table==now->team_table && p->team_row==now->team_row &&
  p->year==now->year && p->type==6 && now->type==6 && now->week>=p->week && now->week<=p->week+1;
}
static int portal_commit_index(const PortalCommitPlan *plan,Rec recruit) {
 for(int j=0;j<plan->count;j++)if(plan->entries[j].table==q((U)recruit+0x10) && plan->entries[j].rid==row(recruit))return j;
 return -1;
}
/* Close, unlocked top-three races can reserve an opening. Final NSD uses
   its existing threshold-free eligibility: positive earned progress and
   first preference here, rather than the weekly commitment-score threshold. */
static int portal_commit_viable(Rec recruit,Rec target,Rec team,U eval,int deadline,unsigned *rank_out,PortalRoomIssue *optional) {
 PortalChoice p={.rec=(U)recruit,.existing=1};
 if(!portal_choice_race(&p,target,team,eval))return -1;
 if(p.stage>=5 || !p.offered || !p.race_ok || p.influence<=0 || p.leader<=0)return 0;
 unsigned tid=scalar(team,383,0x100),rank=1;int found=0;
 Rec top=array_field(recruit,13);if(!top)return -1;
 U n=array_count(top);if(n>64){release(top);return -1;}
 for(U j=0;j<n;j++) {
  Rec e=array_at(top,j);if(!e){release(top);return -1;}
  unsigned school=scalar(e,0,0x100),points=scalar(e,1,0x100);release(e);
  if(school==tid)found=1;else if(points>(unsigned)p.influence)rank++;
 }
 release(top);*rank_out=rank;
 if(!found && p.stage!=0)return 0;
 if(rank>3 || (int64_t)p.influence*4<(int64_t)p.leader*3)return 0;
 if(deadline && rank!=1)return 0;
 if(!native_recruit_eligible(eval,team,recruit))return 0;
 return !portal_decision_checked(recruit,team,eval,"PRIORITY_CHECK",optional) && !faulted;
}
static int portal_commit_room(PortalRoster *room,Rec team,Rec recruit,U eval,int pos,
                              const char *site,const char *reason,PortalRoomIssue *optional) {
 PortalRoomIssue local={0},*issue=optional?optional:&local;
 if(portal_rosters_collect(room,1,team,eval,pos,issue))return 1;
 portal_room_unavailable(issue,optional!=0,reason,site,scalar(team,383,0x100),row(recruit),pos);
 return 0;
}
#include "rc4_qb_pairs.inc"
static int portal_commitment_block_checked(Rec recruit,Rec team,U eval,const char *site,PortalRoomIssue *optional) {
 if(mode!=1 || faulted || !transfer_recruit(recruit) || scalar(recruit,9,0x108)>=5)return 0;
 Rec user=reference(team,407);if(user){release(user);return 0;}
 unsigned tid=scalar(team,383,0x100);if(tid>=138 || !portal_academy_allowed(recruit,tid))return 0;
 int pair_decision=rc4_pair_gate(recruit,team,eval,site,optional);
 if(pair_decision)return pair_decision<0?0:pair_decision;
 PortalCommitPlan plan,now={0};
 AcquireSRWLockExclusive(&commitment_lock);plan=commitment_plans[tid];ReleaseSRWLockExclusive(&commitment_lock);
 if(!plan.valid || !portal_commit_scope(&now,team,eval) || !portal_commit_plan_matches(&plan,&now))return 0;
 int own=portal_commit_index(&plan,recruit),pos=position(recruit),deadline=strncmp(site,"SIGN_",5)==0 || strncmp(site,"SELECT_",7)==0;
 if(pos<0 || pos>=POSITIONS)return 0;
 Rec own_player=reference(recruit,5);int own_ovr=own_player?(int)scalar(own_player,191,0x100):-1;release(own_player);
 int closed[129]={0},best[129],ratings[MAX_BOARD];unsigned ranks[MAX_BOARD]={0};
 for(int k=0;k<MAX_BOARD;k++)ratings[k]=-1;
 for(int k=0;k<129;k++)best[k]=-1;
 Rec targets=board_array(team);if(!targets)return 0;
 U n=array_count(targets);if(n>MAX_BOARD){release(targets);portal_fault("commit_board_bounds");return 0;}
 int error=0;
 for(U j=0;j<n;j++) {
  Rec target=array_at(targets,j),r=target?reference(target,10):0;
  if(!target || !r){release(r);release(target);error=1;break;}
  int k=portal_commit_index(&plan,r);
  if(k>=0 && plan.entries[k].pos==pos) {
   PortalCommitEntry *e=&plan.entries[k];int slot=e->slot;
   if(slot<1 || slot>128){release(r);release(target);error=1;break;}
   unsigned stage=scalar(r,9,0x108);
   if(stage>=5) {
    int owner=-1,secured=0;
    if(stage==5)secured=portal_hard_owner(r,eval,&owner);
    else {
     Rec player=reference(r,5);
     if(player)secured=portal_signed_destination(r,player,&owner);
     release(player);
     if(!secured)secured=portal_published_owner(r,eval,&owner);
    }
    if(secured<0)error=1;
    if(secured>0 && owner==(int)tid)closed[slot]=1;
   } else if(!portal_same_record(r,recruit)) {
    int viable=portal_commit_viable(r,target,team,eval,deadline,&ranks[k],optional);
    if(faulted || (optional && optional->source)){release(r);release(target);break;}
    if(viable<0)error=1;
    /* Do not let an older preference for an inferior fallback create a cycle
       with the material-upgrade capacity guard. Both gates use the same gap. */
    Rec player=reference(r,5);int other_ovr=player?(int)scalar(player,191,0x100):-1;release(player);
    ratings[k]=other_ovr;
    if(own_ovr>=0 && other_ovr>=0 && own_ovr>=other_ovr+4)viable=0;
    if(viable>0 && (best[slot]<0 || e->round<plan.entries[best[slot]].round ||
       (e->round==plan.entries[best[slot]].round && e->priority<plan.entries[best[slot]].priority)))best[slot]=k;
   }
  }
  release(r);release(target);if(error || faulted)break;
 }
 release(targets);
 if(error){portal_fault("commit_plan_read");return 0;}if(faulted || (optional && optional->source))return 0;
 int blocker=-1,reason=0;
 if(own>=0) {
  PortalCommitEntry *e=&plan.entries[own];int slot=e->slot;
  if(slot<1 || slot>128){portal_fault("commit_plan_slot");return 0;}
  if(closed[slot]) {
   PortalRoster room;if(!portal_commit_room(&room,team,recruit,eval,pos,site,"commit_closed_room",optional))return 0;
   int reserved=0;for(int k=1;k<129;k++)if(!closed[k] && best[k]>=0)reserved++;
   /* R2: the plan describes the old opening, not a permanent veto. A filled
      chain can use independently legal current optional capacity when no
      other live same-position opening owns it. Re-read the complete current
      role/shared allowance policy: an old plan never creates an exception.
      A separate unreserved ordinary vacancy retains its existing path. */
   if(room.ideal-room.count<=reserved &&
      (reserved>0 || portal_decision_checked(recruit,team,eval,"CLOSED_SLOT_RECHECK",optional)))reason=10;
   if(faulted || (optional && optional->source))return 0;
  }
  else if(best[slot]>=0) {
   int b=best[slot];PortalCommitEntry *better=&plan.entries[b];
   if(better->round<e->round || (better->round==e->round && better->priority<e->priority)){reason=9;blocker=b;}
  }
 } else {
  /* Native leftovers may use unreserved vacancies, but cannot consume the
     final place reserved for another opening's viable first choice. */
  PortalRoster room;if(!portal_commit_room(&room,team,recruit,eval,pos,site,"commit_room",optional))return 0;
  int remaining=room.ideal-room.count,reserved=0;
  for(int s=1;s<129;s++)if(!closed[s] && best[s]>=0){reserved++;blocker=best[s];}
  if(reserved && remaining<=reserved)reason=9;
 }
 if(reason && own_ovr>=0 && pos>0 && pos<=18) {
  /* R4: the old slot is not a veto when the candidate can coexist with all
     viable same-position primaries as prestige-appropriate useful depth.
     Model both sides, so the new permission cannot steal a promised role.
     Only numeric projections change here; all native records stay read-only. */
  PortalRoster room;
  if(!portal_commit_room(&room,team,recruit,eval,pos,site,"commit_quality_room",optional))return 0;
  int step=portal_projection_step(eval),value=portal_projected_ovr(own_ovr,step);
  PortalQuality combined=room.projected;
  for(int slot=1;slot<129 && combined.count<192;slot++)if(!closed[slot] && best[slot]>=0 && ratings[best[slot]]>=0)
   dri_portal_project_add(&combined,portal_projected_ovr(ratings[best[slot]],step));
  int coexists=dri_portal_quality_depth_rank(&combined,room.prestige,value)>0;
  for(int slot=1;slot<129 && coexists;slot++)if(!closed[slot] && best[slot]>=0 && ratings[best[slot]]>=0) {
   PortalQuality other=room.projected;
   if(other.count>=192){coexists=0;break;}
   dri_portal_project_add(&other,value);
   for(int s=1;s<129 && other.count<192;s++)if(s!=slot && !closed[s] && best[s]>=0 && ratings[best[s]]>=0)
    dri_portal_project_add(&other,portal_projected_ovr(ratings[best[s]],step));
   int raw=ratings[best[slot]];
   if(dri_quality_block_candidate(&other,room.prestige,portal_projected_ovr(raw,step),raw))coexists=0;
  }
  if(coexists)reason=0;
 }
 if(reason)log_line("PORTAL_COMMIT_DEFER,%s,%u,%u,%d,%d,%d,%u,%u",site,tid,row(recruit),pos,own>=0?plan.entries[own].slot:0,reason,blocker>=0?plan.entries[blocker].rid:0,blocker>=0?ranks[blocker]:0);
 return reason;
}
static int portal_commitment_block(Rec recruit,Rec team,U eval,const char *site) {
 return portal_commitment_block_checked(recruit,team,eval,site,0);
}
#ifdef DRI_EMULATION_TEST
__declspec(dllexport) void DriTestCommitPlan(const PortalChoice *pool,int n,Rec team,U eval){portal_commit_plan_store(pool,n,team,eval,"FIXTURE");}
__declspec(dllexport) int DriTestCommitBlock(Rec r,Rec team,U eval,int deadline){return portal_commitment_block(r,team,eval,deadline?"SIGN_FIXTURE":"WEEKLY");}
__declspec(dllexport) void DriTestCommitReset(void){memset(commitment_plans,0,sizeof(commitment_plans));}
#endif
