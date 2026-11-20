/* Test31: the native initial-interest initializer has its own field-9 writer.
   It does not call the weekly stage writer or the weekly ownership hook. At
   081550fc the original native condition already passed and EAX is the next
   stage. A hold jumps to the original cleanup at 0815513b; it never edits
   the recruit, initial points, offers or ownership. The native empty-list
   check confines this callback to creation of the initial preference list. */
static int test31_initial_commit_block(Rec recruit,U eval,unsigned next) {
 if(mode!=1 || faulted || !recruit || !eval || next<4 || next>6 || !hs15_is_recruit(recruit))return 0;
 unsigned stage=scalar(recruit,9,0x108);
 if(stage>=5 || next<=stage)return 0; /* existing secured choices are preserved */
 Rec leader=portal_leader(recruit,eval);
 /* Missing initialization-time board/leader data cannot prove an earned
    commitment. Defer this initial transition; ordinary later recruiting uses
    its existing, fully checked native path once actual offers exist. */
 int state=leader?hs29_offer_state(recruit,leader,eval):0;
 unsigned tid=leader?scalar(leader,383,0x100):255;
 release(leader);
 int blocked=state!=2 || faulted || mode!=1;
 log_line("TEST31_INITIAL_COMMIT,%u,%u,%u,%u,%d,%s",row(recruit),stage,next,tid,state,
  blocked?"HOLD_NO_CURRENT_FIRM_OFFER_INITIAL_POINTS_PRESERVED":"ALLOW_CURRENT_FIRM_OFFER_NATIVE_CONDITION_PASSED");
 return blocked;
}
int DriOnInitialHsStage(Registers *r) {
 if(mode!=1 || faulted)return 0;
 __try {
  return test31_initial_commit_block((Rec)q(r->rbp+7),r->r13,(unsigned)r->rax);
 } __except(fail_exception("initial_HS_stage31",GetExceptionCode())){return 1;}
}
#ifdef DRI_EMULATION_TEST
__declspec(dllexport) int DriTest31InitialStage(Registers *r){return DriOnInitialHsStage(r);}
__declspec(dllexport) int DriTest31InitialCommit(Rec r,U e,unsigned next){return test31_initial_commit_block(r,e,next);}
#endif
