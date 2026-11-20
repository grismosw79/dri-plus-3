/* Test26 final destination selection. Only existing active offers compete.
   Native commit scores and shortlist ranks do not restrict the final NSD pass.
   Eligibility and the current roster budget are rechecked after each signing.
   Equal influence uses the smaller TeamIndex for a reproducible tie break. */
#include "hs28_nsd_match.inc"
static Rec portal_nsd_winner(Rec recruit,U eval,int prune) {
 if(mode!=1 || faulted || !recruit || scalar(recruit,9,0x108)>=5)return 0;
 if(!hs15_is_recruit(recruit) && !transfer_recruit(recruit))return 0;
 int owner=-1,published=portal_published_owner(recruit,eval,&owner);
 if(published<0){portal_fault("NSD_published_owner");return 0;}
 if(published) {
  log_line("NSD_ALREADY_ACQUIRED,%u,%d,%u",row(recruit),owner,scalar(recruit,9,0x108));
  return 0;
 }
 if(transfer_recruit(recruit))rc4_restore_if_cold(eval);
 unsigned points[138]={0},listed[138]={0},drop[138]={0};
 Rec top=array_field(recruit,13),winner=0;unsigned best=0,best_tid=138;
 if(!top){portal_fault("NSD_interest");return 0;}
 U n=array_count(top);int bad=n>138;
 for(U i=0;i<n && !bad;i++) {
  Rec e=array_at(top,i);if(!e){bad=1;break;}
  unsigned tid=scalar(e,0,0x100),value=scalar(e,1,0x100);release(e);
  if(tid>=138 || value>65535 || listed[tid])bad=1;
  else {listed[tid]=1;points[tid]=value;}
 }
 release(top);
 if(bad){portal_fault("NSD_interest_values");return 0;}
 int hs=hs15_is_recruit(recruit),resolved=-1;
 if(hs){Rec matched=hs28_nsd_destination(recruit,eval);if(matched)resolved=(int)scalar(matched,383,0x100);release(matched);}
 for(unsigned tid=0;tid<138 && !faulted;tid++) {
  Rec team=portal_team_by_id(eval,tid);
  if(!team){portal_fault("NSD_team");break;}
  unsigned value=0;int offer=portal_offer_total(recruit,team,&value);
  if(offer<0){release(team);portal_fault("NSD_offer");break;}
  if(!offer){release(team);continue;}
  if(listed[tid])value=points[tid];
  int allowed=!cpu_recruit_decision(recruit,team,eval,"NSD_OFFER",0);
  if(allowed && !faulted)
   allowed=native_recruit_eligible(eval,team,recruit);
  int pair_block=0;
  if(allowed && !faulted && !hs)pair_block=portal_commitment_block(recruit,team,eval,"SELECT_NSD");
  if(allowed && !faulted && !pair_block)pair_block=test29r1_capacity_block(recruit,team,eval,"SELECT_NSD");
  if(allowed && !faulted && hs && resolved!=(int)tid) {
   pair_block=17;
   /* Nonwinning valid offers remain available. Only the safe pruning path
      needs the extra room read to distinguish a permanently filled opening;
      winner lookups must not rebuild the plan recursively for every offer. */
   if(prune) {
    Rec user=reference(team,407);int cpu=!user;release(user);
    if(cpu) {
     Hs15Snapshot shot;int pos=position(recruit);
     if(pos<0 || pos>=22 || !hs15_collect(&shot,team,eval))portal_fault("NSD_pair_prune_room");
     else if(hs28_recruit_opening_block(&shot,recruit,pos))pair_block=16;
    }
   }
  }
  if(!faulted) {
   drop[tid]=!allowed || pair_block==16;
   /* A temporary primary preference is not an invalid scholarship. Keep it
      available for the next native pass if the primary goes elsewhere. */
   if(pair_block)allowed=0;
   if(allowed && (best_tid==138 || value>best || (value==best && tid<best_tid))) {
    release(winner);winner=team;team=0;best=value;best_tid=tid;
   }
  }
  release(team);
 }
 if(faulted || mode!=1){release(winner);return 0;}
 /* This is called with prune only at the native fallback entry, before it
    creates board iterators. Validate the complete set before removing offers. */
 if(prune)for(unsigned tid=0;tid<138 && !faulted;tid++)if(drop[tid]) {
  Rec team=portal_team_by_id(eval,tid);U manager=q(eval+0xc8);
  if(!team || !manager){release(team);portal_fault("NSD_prune");break;}
  U candidate[6]={(U)recruit,0,0,0,0,0};
  int result=((int(*)(U,Rec*,U*))(game_base+0x081a7b10))(manager,&team,candidate);
  int still_offered=portal_active_offer(recruit,team);release(team);
  if(result || still_offered){portal_fault("NSD_prune_result");break;}
  log_line("NSD_OFFER_WITHDRAW,%u,%u",row(recruit),tid);
 }
 if(faulted || mode!=1){release(winner);return 0;}
 log_line("NSD_OFFER_WINNER,%u,%u,%u,%s",row(recruit),best_tid,best,transfer_recruit(recruit)?"PORTAL":"HS");
 return winner; /* Retained native Team reference transfers to the caller. */
}
static int portal_nsd_hs_pending(U eval,unsigned needed[138],int *unsigned_count) {
 memset(needed,0,138*sizeof(*needed));*unsigned_count=0;
 Rec franchise=context_ref(eval,6),a=franchise?array_field(franchise,39):0;release(franchise);
 if(!a){portal_fault("NSD_HS_pool");return 0;}
 U n=array_count(a);int pending=0;
 if(n>12000){release(a);portal_fault("NSD_HS_bounds");return 0;}
 for(U i=0;i<n && !faulted;i++) {
  Rec recruit=array_at(a,i);
  if(!recruit){portal_fault("NSD_HS_record");break;}
  if(scalar(recruit,9,0x108)<5) {
   (*unsigned_count)++;
   if(scalar(recruit,14,0x100)>0) {
    Rec winner=portal_nsd_winner(recruit,eval,0);
    if(winner){needed[scalar(winner,383,0x100)]=1;pending++;}
    release(winner);
   }
  }
  release(recruit);
 }
 release(a);return pending;
}
/* The native HS pass visits schools in TeamIndex order. If a later school
   fills up, a recruit can fall back to an earlier school's still-valid offer.
   Re-run only needed school passes while the native destination vectors/maps
   are live, before the original publication/event loop. Every productive pass
   consumes at least one unsigned recruit; no stage or roster is synthesized. */
/* Native 0x08168a40 returns an iterator (node + bucket pointers) followed by
   an insertion flag. Both found and inserted paths write through offset 16.
   A single pointer output corrupts adjacent locals. Keep the full native ABI. */
typedef struct { U node,bucket; unsigned char inserted,padding[7]; } NsdTeamLookup;
_Static_assert(sizeof(NsdTeamLookup)==24 && offsetof(NsdTeamLookup,inserted)==16,
               "NSD team lookup result ABI");
#include "hs_finalize.inc"
#include "test31_nsd_queue.inc"
#include "test29r2_portal_retry.inc"
#include "academy29_return.inc"
void DriBeforeNsdPublish(Registers *r) {
 if(mode!=1 || faulted)return;
 __try {
  U eval=r->r13;if(!eval || !*(unsigned char*)(eval+0x1a0))return;
  /* A fresh native finalization phase must not inherit a prior loaded NSD
     component, even when the calendar and table identities were reused. */
  hs28_nsd_reset();
  int finalized=hs_finalize_published(eval);
  if(finalized<0 || faulted)return;
  int previous=12001,changed=finalized>0;
  for(unsigned pass=0;pass<=12000 && !faulted;pass++) {
   unsigned needed[138];int remaining=0,pending=portal_nsd_hs_pending(eval,needed,&remaining);
   if(previous<12001 && remaining<previous)changed=1;
   if(faulted || !pending)break;
   if(remaining>=previous) {
    log_line("NSD_HS_UNRESOLVED,%d,%d,NO_NATIVE_PROGRESS",remaining,pending);break;
   }
   previous=remaining;
   log_line("NSD_HS_FINAL_PASS,%u,%d",pass+1,pending);
   for(unsigned tid=0;tid<138 && !faulted;tid++)if(needed[tid]) {
    Rec team=portal_team_by_id(eval,tid);NsdTeamLookup result={0};
    if(!team){portal_fault("NSD_HS_team");break;}
    log_line("NSD_HS_TEAM_BEGIN,%u,%u",pass+1,tid);
    ((void(*)(U,NsdTeamLookup*,unsigned,unsigned*,U))(game_base+0x08168a40))(r->rbp-0x78,&result,0,&tid,tid);
    if(!result.node){release(team);portal_fault("NSD_HS_vector");break;}
    ((void(*)(U,Rec*,U,U,U,unsigned))(game_base+0x081b4150))
      (eval,&team,result.node+8,r->rbp+0x18,r->rbp-0x18,0);
    release(team);
    log_line("NSD_HS_TEAM_END,%u,%u",pass+1,tid);
   }
  }
  int portal_added=test29r2_portal_retry(r);
  if(portal_added<0 || faulted)return;
  if(portal_added>0)changed=1;
  int restored=academy29_restore(r);
  if(restored<0 || faulted)return;
  if(restored>0)changed=1;
  int queue_result=test31_nsd_queue_validate(r->rbp-0x78,eval);
  if(!queue_result)return;
  if(queue_result>1)changed=1;
  /* The native caller already refreshed recruiting-class statistics before
     this boundary. Recompute once if native HS or portal retries added players. */
  if(changed && !faulted)((void(*)(U))(game_base+0x081bb5e0))(eval);
 } __except(fail_exception("NSD_HS_finalize",GetExceptionCode())){}
}
#ifdef DRI_EMULATION_TEST
__declspec(dllexport) Rec DriTestNsdWinner(Rec recruit,U eval,int prune){return portal_nsd_winner(recruit,eval,prune);}
#endif
