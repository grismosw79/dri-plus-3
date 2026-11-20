/* Test29: effective capacity ten and earlier native CPU visit selection.
   Include after log_line/fail_exception and before Hook table. */
#include "visit29_core.h"
U ResumeVisit29Capacity,ResumeVisit29Dates,SkipVisit29Dates,ResumeVisit29Booked,NativeVisit29Account;
extern void HookVisit29Capacity(void),HookVisit29Dates(void),HookVisit29Booked(void);
void DriOnVisit29Capacity(Registers *r) {
    if(mode!=1 || faulted)return;
    r->rsi=(uint32_t)visit29_capacity((int32_t)r->rsi);
}
void DriOnVisit29Dates(Registers *r) {
    if(mode<0 || faulted)return;
    __try {
        U begin=q(r->rbp-0x11),end=q(r->rbp-9),eval=r->r15;
        if(!begin || end<begin || (end-begin)%24 || end-begin>32*24)return;
        size_t count=(end-begin)/24;int week=i32(eval+0x1a4),type=i32(eval+0x1a8);
        Visit29Date copy[32];if(count)memcpy(copy,(void *)begin,count*24);
        if(!visit29_dates(copy,count,week,type))return;
        int old_index=-1,new_index=-1;
        for(size_t n=0;n<count;n++) {
            Visit29Date *old=&((Visit29Date *)begin)[n];
            if(!old->opponent && !old->bye)continue; /* Same native selector guard. */
            if(old_index<0 || old->priority<((Visit29Date *)begin)[old_index].priority)old_index=(int)n;
            if(new_index<0 || copy[n].priority<copy[new_index].priority)new_index=(int)n;
        }
        if(old_index<0 || new_index<0)return;
        Rec team=(Rec)q(q(r->rbp+0x67)),target=(Rec)q(q(r->rbp+0x6f));
        log_line("VISIT29_PLAN,%u,%u,%d,%d,%llu,%d,%d,%s",team?scalar(team,383,0x100):999u,target?row(target):0xffffffffu,type,week,(unsigned long long)count,copy[old_index].week,copy[new_index].week,mode==1?"ACTIVE":"DIAGNOSTIC");
        if(mode==1 && !faulted)for(size_t n=0;n<count;n++)((Visit29Date *)begin)[n].priority=copy[n].priority;
    } __except(fail_exception("visit29_dates",GetExceptionCode())) {}
}
void DriOnVisit29Booked(Registers *r) {
    if(mode<0 || faulted || (int32_t)r->r12<=0)return;
    __try {
        Rec team=(Rec)q(q(r->rbp+0x67)),target=(Rec)q(q(r->rbp+0x6f));
        log_line("VISIT29_BOOKED,%u,%u,%d,%d,%d,%d,%d,native_cost_accounted",team?scalar(team,383,0x100):999u,target?row(target):0xffffffffu,i32(r->r15+0x1a8),i32(r->r15+0x1a4),i32(r->rbp-0x29),i32(r->rbp-0x49),(int32_t)r->r12);
    } __except(fail_exception("visit29_booked",GetExceptionCode())) {}
}
/* Read-only witnesses. Eligibility is observed at a CPU action query, before
   spending/date checks; it is not a claim about an earlier unobserved week.
   Completion runs only after matching stored visit date/type and native
   influence calculation/application. Input gain may be clamped natively. */
U ResumeVisit29Eligible,SkipVisit29Eligible,ResumeVisit29Completed,SkipVisit29Completed;
extern void HookVisit29Eligible(void),HookVisit29Completed(void);
#ifdef DRI_EMULATION_TEST
static int visit29_observed[7];
__declspec(dllexport) void DriTestVisit29Observation(int *out) {memcpy(out,visit29_observed,sizeof(visit29_observed));}
__declspec(dllexport) void DriTestVisit29ObservationReset(void) {memset(visit29_observed,0,sizeof(visit29_observed));}
__declspec(dllexport) void DriTestVisit29ObserveHooks(U *out) {
 ResumeVisit29Eligible=game_base+0x08177b43;SkipVisit29Eligible=game_base+0x08177c76;
 ResumeVisit29Completed=game_base+0x081a401a;SkipVisit29Completed=game_base+0x081a405a;
 out[0]=(U)HookVisit29Eligible;out[1]=(U)HookVisit29Completed;
}
#endif
void DriOnVisit29Eligible(Registers *r) {
 if(mode<0 || faulted || !(r->rax&255))return;
 __try {
  Rec team=(Rec)q(r->r15),target=(Rec)q(r->rsi);U eval=r->rbx;
  if(!team || !target)return;
#ifdef DRI_EMULATION_TEST
  visit29_observed[0]++;
#endif
  log_line("VISIT29_ELIGIBLE,%u,%u,%d,%d,%d,native_action_ok_cost_and_calendar_pending",scalar(team,383,0x100),row(target),i32(eval+0x1a8),i32(eval+0x1a4),i32(r->rbp+0x60));
 } __except(fail_exception("visit29_eligible",GetExceptionCode())) {}
}
void DriOnVisit29Completed(Registers *r) {
 if(mode<0 || faulted)return;
 __try {
  Rec team=(Rec)q(q(r->rbp+0x78)),target=(Rec)q(r->r12);U eval=r->r14;
  if(!team || !target)return;
  int applied=(r->rax&255)!=0,input=i32(r->rbp+0x80);
  unsigned points=scalar(target,8,0x100);
#ifdef DRI_EMULATION_TEST
  visit29_observed[1]++;visit29_observed[2]=applied;visit29_observed[3]=input;visit29_observed[4]=(int)points;
  visit29_observed[5]=i32(eval+0x1a8);visit29_observed[6]=i32(eval+0x1a4);
#endif
  log_line("VISIT29_COMPLETION,%u,%u,%d,%d,%d,%d,%u,native_apply_result_input_and_resulting_influence",scalar(team,383,0x100),row(target),i32(eval+0x1a8),i32(eval+0x1a4),applied,input,points);
 } __except(fail_exception("visit29_completed",GetExceptionCode())) {}
}
