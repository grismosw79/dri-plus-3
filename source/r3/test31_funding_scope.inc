/* Test31: selection forecasts, retained action flags and native current-pass
   allocation are different evidence. Keep numeric identities only; no game
   record is retained or edited by these ledgers. */
typedef struct {U table;uint32_t row;} Test31TargetIdentity;
typedef struct {
 Hs28TeamPlan scope;U eval;int valid,count;
 Test31TargetIdentity targets[MAX_BOARD];
} Test31FundingPass;
static Test31FundingPass test31_funding_pass[138];
static SRWLOCK test31_funding_lock=SRWLOCK_INIT;
static void test31_funding_record(Rec team,U eval,PortalFundingCandidate *a,int n) {
 unsigned tid=scalar(team,383,0x100);if(tid>=138)return;
 Test31FundingPass pass={0};pass.eval=eval;
 if(n<0 || n>MAX_BOARD || !hs28_scope(&pass.scope,team,eval))return;
 for(int j=0;j<n;j++)if(a[j].target && a[j].candidate.values[8]>0) {
  Rec target=(Rec)a[j].target,linked=reference(target,10);
  int match=portal_same_record(linked,(Rec)a[j].candidate.rec);release(linked);
  if(!match){portal_fault("funding31_target_identity");return;}
  pass.targets[pass.count++]=(Test31TargetIdentity){q((U)target+0x10),row(target)};
 }
 pass.valid=1;pass.scope.valid=1;
 AcquireSRWLockExclusive(&test31_funding_lock);test31_funding_pass[tid]=pass;ReleaseSRWLockExclusive(&test31_funding_lock);
}
static unsigned test31_productive_allocated_detail(Rec target,Rec team,U eval,unsigned *mask,unsigned *raw) {
 *mask=0;if(raw)*raw=0;if(!target || !team)return 0;
 unsigned tid=scalar(team,383,0x100);if(tid>=138)return 0;
 Test31FundingPass pass;AcquireSRWLockExclusive(&test31_funding_lock);pass=test31_funding_pass[tid];ReleaseSRWLockExclusive(&test31_funding_lock);
 Hs28TeamPlan now={0};
 if(pass.valid && pass.eval==eval && hs28_scope(&now,team,eval) && hs28_scope_matches(&pass.scope,&now) &&
    pass.scope.type==now.type && pass.scope.week==now.week) {
  int current=0;U table=q((U)target+0x10);uint32_t id=row(target);
  for(int j=0;j<pass.count;j++)if(pass.targets[j].table==table && pass.targets[j].row==id){current=1;break;}
  /* A retained pitch on a target absent from the native allocation vector
     belongs to an older pass. Do not erase it or charge its cost again. */
  if(!current){if(raw){unsigned old_mask=0;*raw=hs28_productive_allocated(target,eval,&old_mask);}return 0;}
 }
 unsigned value=hs28_productive_allocated(target,eval,mask);if(raw)*raw=value;return value;
}
static unsigned test31_productive_allocated(Rec target,Rec team,U eval,unsigned *mask) {
 return test31_productive_allocated_detail(target,team,eval,mask,0);
}

typedef struct {U eval,manager,generation;} Test31OfferContext;
static Test31OfferContext test31_offer_contexts[PORTAL_FUNDING_EVALUATORS];
static SRWLOCK test31_offer_lock=SRWLOCK_INIT;
static U test31_offer_generation;
static void test31_offer_register(U eval) {
 if(!eval)return;U manager=q(eval+0xc8);if(!manager)return;
 AcquireSRWLockExclusive(&test31_offer_lock);
 int at=0;
 for(int j=0;j<PORTAL_FUNDING_EVALUATORS;j++) {
  if(test31_offer_contexts[j].manager==manager){at=j;break;}
  if(test31_offer_contexts[j].generation<test31_offer_contexts[at].generation)at=j;
 }
 test31_offer_contexts[at]=(Test31OfferContext){eval,manager,++test31_offer_generation};
 ReleaseSRWLockExclusive(&test31_offer_lock);
}
static U test31_offer_evaluator(U manager) {
 U eval=0;AcquireSRWLockExclusive(&test31_offer_lock);
 for(int j=0;j<PORTAL_FUNDING_EVALUATORS;j++)if(test31_offer_contexts[j].manager==manager){eval=test31_offer_contexts[j].eval;break;}
 ReleaseSRWLockExclusive(&test31_offer_lock);return eval;
}
/* Test33: rank provisional places separately from the live hard ceiling.
   Count actual CPU scholarships at the native action boundary; neither a
   derived counter nor a provisional permit authorizes a same-pass overflow. */
static int test32_hs_offer_capacity(Rec recruit,Rec applicant,U eval) {
 Rec f=context_ref(eval,6),teams=f?array_field(f,45):0;
 U nt=teams?array_count(teams):0;int result=-1,count=0,own=0,applicant_seen=0;
 uint64_t seen[3]={0};
 if(!recruit || !applicant || !f || !teams || !nt || nt>138)goto done;
 unsigned applicant_id=scalar(applicant,383,0x100);
 if(applicant_id>=138)goto done;
 for(U i=0;i<nt;i++) {
  Rec team=array_at(teams,i);unsigned tid=team?scalar(team,383,0x100):255;
  if(!team || tid>=138 || market29_has(seen,tid)){release(team);goto done;}
  market29_bit(seen,tid,1);
  if(tid==applicant_id) {
   if(!portal_same_record(team,applicant)){release(team);goto done;}
   applicant_seen=1;
  }
  Rec user=reference(team,407);
  if(user){release(user);release(team);continue;}
  Rec board=board_array(team);U n=board?array_count(board):MAX_BOARD+1;
  if(!board || n>MAX_BOARD){release(board);release(team);goto done;}
  int found=0,offered=0,valid=1;
  for(U j=0;j<n;j++) {
   Rec target=array_at(board,j),linked=target?reference(target,10):0;
   unsigned status=target?scalar(target,12,0x108):255;
   if(!target || !linked || status>4)valid=0;
   if(linked && portal_same_record(linked,recruit)) {
    if(found++)valid=0;
    if(status>=2 && status<=4)offered=1;
   }
   release(linked);release(target);if(!valid)break;
  }
  release(board);release(team);if(!valid)goto done;
  count+=offered;if(tid==applicant_id)own=offered;
 }
 if(!applicant_seen)goto done;
 Rec player=reference(recruit,5);if(!player)goto done;
 int portal=transfer_recruit(recruit);
 int limit=portal?market30_limit(1):hs33_limit_stars((int)scalar(player,231,0x108));release(player);
 result=own || count<limit;
 if(!result)log_line("TEST36_ACTUAL_CAP,%s,%u,%u,%d,%d,NEW_OFFER_HELD_BEFORE_PAYMENT",portal?"PORTAL":"HS",applicant_id,row(recruit),count,limit);
done:
 release(teams);release(f);return result;
}
/* Preserve published incoming ownership when a native recruit stage lags.
   This bounded local check runs only on a proposed unoffered productive action. */
static int test33_hs_published(Rec recruit,Rec team) {
 Rec player=reference(recruit,5),incoming=array_field(team,33);int found=0;
 U n=incoming?array_count(incoming):0;
 if(player && n<=192)for(U j=0;j<n;j++) {
  Rec p=array_at(incoming,j);found=portal_same_record(p,player);release(p);if(found)break;
 }
 release(incoming);release(player);return found;
}
/* Entry to the captured native action eligibility routine, before native
   payment or scholarship publication. The same routine is called by both
   normal and special CPU offer/payment routes. Returning1 asks the hook to
   return native false; permitted calls execute the unchanged native body. */
int DriOnOfferAction(Registers *r) {
 if(mode!=1 || faulted)return 0;
 __try {
  int action=i32((U)(r+1)+0x28),productive=action==2 || (action>=5 && action<=9);
  if((action!=3 && !productive) || !r->rdx || !r->r8)return 0;
  Rec team=(Rec)q(r->rdx),target=(Rec)q(r->r8);if(!team || !target)return 0;
  Rec user=reference(team,407);
  if(user) {
   release(user);
   /* User offers keep their exemption, but an actual new HS scholarship
      must be visible to the next weekly destination/firm-coverage scan. */
   if(action==3 && scalar(target,12,0x108)<2) {
    Rec recruit=reference(target,10);if(hs15_is_recruit(recruit))hs29_offer_invalidate();release(recruit);
   }
   return 0;
  }
  unsigned status=scalar(target,12,0x108);
  if(status>=2 && status<=4)return 0; /* established offers are grandfathered */
  Rec recruit=reference(target,10);if(!recruit)return 0;
  int hs=hs15_is_recruit(recruit),portal=transfer_recruit(recruit);
  if((!hs && !portal) || scalar(recruit,9,0x108)>=5){release(recruit);return 0;}
  /* The native indexed pass attempts scholarships before the physical
     productive pass. A failed or refused new scholarship must not purchase
     points/pitches. Existing offers, user teams and portal actions keep their
     native behavior. No target flags or budget counters are synthesized. */
  if(productive) {
   if(hs && test33_hs_published(recruit,team)){release(recruit);return 0;}
   if(hs)log_line("TEST33_UNOFFERED_ACTION_VETO,%u,%u,%d,NATIVE_BEFORE_PAYMENT",scalar(team,383,0x100),row(recruit),action);
   release(recruit);return hs || portal;
  }
  U eval=test31_offer_evaluator(r->rcx);
  int blocked=!eval || (hs?hs32_admission_block(recruit,team,eval):market29_portal_admission_block(recruit,team,eval));
  if(portal && t36_active_eval==eval && t36_designated && portal_same_record(t36_designated,recruit) && portal_same_record(t36_destination,team))
   blocked=r3_qb_admission(recruit,team,eval,"OPENING_OFFER");
  const char *reason=eval?(hs?"CURRENT_MARKET_SCOPE":"FROZEN_AUTHORIZATION"):"NO_CURRENT_EVALUATOR";
  if(!blocked && (hs || portal)) {
   int capacity=test32_hs_offer_capacity(recruit,team,eval);
   if(capacity<0){portal_fault("HS32_actual_offer_count");blocked=1;reason="ACTUAL_OFFER_READ_FAILED";}
   else if(!capacity){blocked=1;reason="ACTUAL_CPU_STAR_CAP";}
  }
  if(blocked)log_line("TEST32_NEW_OFFER_VETO,%s,%u,%u,%s,NATIVE_ACTION3_BEFORE_PAYMENT",hs?"HS":"PORTAL",scalar(team,383,0x100),row(recruit),reason);
  if(hs && eval && blocked)hs33_release_offer(recruit,team,eval);
  if(!blocked && hs)hs29_offer_invalidate();
  release(recruit);return blocked;
 } __except(fail_exception("new_offer31",GetExceptionCode())){return 1;}
}
#ifdef DRI_EMULATION_TEST
__declspec(dllexport) void DriTest31FundingRecord(Rec t,U e,PortalFundingCandidate *a,int n){test31_funding_record(t,e,a,n);}
__declspec(dllexport) unsigned DriTest31ProductiveAllocated(Rec t,Rec s,U e,unsigned *m){return test31_productive_allocated(t,s,e,m);}
__declspec(dllexport) void DriTest31OfferRegister(U e){test31_offer_register(e);}
__declspec(dllexport) int DriTest31OfferAction(Registers *r){return DriOnOfferAction(r);}
__declspec(dllexport) int DriTest32HsOfferCapacity(Rec r,Rec t,U e){return test32_hs_offer_capacity(r,t,e);}
__declspec(dllexport) void DriTest31FundingReset(void){memset(test31_funding_pass,0,sizeof(test31_funding_pass));memset(test31_offer_contexts,0,sizeof(test31_offer_contexts));}
#endif
