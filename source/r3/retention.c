#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include "policy.h"
#include "signatures.h"
#include "log30_policy.h"

#define POSITIONS 22
#define MAX_BOARD 128
#define EXPECTED_IMAGE_SIZE 302002176u
#define EXPECTED_TIMESTAMP 1794008698u

typedef uintptr_t U;
typedef void *Rec;
/* RecruitRecord wrapper includes the rank-search bound at +0x18. */
typedef struct { U vt; Rec rec; U schema; U rank_search_limit; } Wrap;
typedef struct {
    U r15,r14,r13,r12,r11,r10,r9,r8,rdi,rsi,rbp,rbx,rdx,rcx,rax,flags;
} Registers;
_Static_assert(sizeof(Wrap)==32 && offsetof(Wrap,rank_search_limit)==24,"Recruit wrapper ABI");
_Static_assert(sizeof(Registers)==128 && offsetof(Registers,rbp)==80,"Hook frame ABI");
typedef struct {
    uint32_t rid, target_row;
    int pos, stage, rank, held, eligible, own, native_protected, challenger, chosen;
    int stars,ovr,policy_block;
    uint32_t influence, leader;
} Member;
#define HS15_MAX_PLAYERS 384
typedef struct {
 U table;uint32_t rid;int pos,ovr,returning,incoming,hs,stars,published,qb_young,qb_priority;
} Hs15Player;
typedef struct {
 int ideal[POSITIONS],count[POSITIONS],returning[POSITIONS],hs[POSITIONS];
 int four[POSITIONS],five[POSITIONS],flex[POSITIONS],flex_floor[POSITIONS];
 int starters[POSITIONS];
 int qb85,total,total_ideal,roster_count,departing_specialists,used;
 int prestige,projection_step;
 Hs15Player players[HS15_MAX_PLAYERS];
 int annual[POSITIONS],annual_target,annual_valid;
} Hs15Snapshot;
typedef struct {
    int valid, mode, team, count, year, week, week_type, portal;
    int commits[POSITIONS], ideal[POSITIONS], limit[POSITIONS], closed[POSITIONS];
    int board_commits[POSITIONS],projected[POSITIONS],hs_four[POSITIONS],hs_five[POSITIONS];
    int flex[POSITIONS],flex_floor[POSITIONS],qb85;
    int returning[POSITIONS], roster_count, departing_specialists;
    U frame, eval, team_record;
    uint64_t pass;
    int quota[POSITIONS], native_open[POSITIONS], selected[POSITIONS];
    Member members[MAX_BOARD];
    Hs15Snapshot hs_snapshot;
} Session;

static U game_base;
static HMODULE self_module;
static __declspec(thread) Rec t36_relation,t36_relation_team,t36_designated,t36_destination;
static __declspec(thread) int t36_immediate;
static __declspec(thread) U t36_active_eval;
static void t36_open(U,U,const char*);
static void r2_hc_hire(void *module,Rec coach,Rec dest,int role,int length);
static void t36_pre_contract(void *module, int before_round_clear);
static void t36_arm(Rec f,unsigned year);
static DWORD tls_index = TLS_OUT_OF_INDEXES;
static volatile LONG mode = -1, installed = 0, faulted = 0;
static volatile LONG64 pass_counter = 0;
static SRWLOCK log_lock = SRWLOCK_INIT;
static HANDLE log_file = INVALID_HANDLE_VALUE;
#include "../log_io.h"
#include "cache_stack.h"
static Log30State log30_state;
#ifdef DRI_EMULATION_TEST
static int log30_deep=1; /* Historical fixture evidence remains complete. */
#else
static int log30_deep;
#endif
U ResumeStart, ResumeProtect, ResumeAdmit, SkipAdmit;
U ResumeOfferAction,ResumeInitialHsStage,SkipInitialHsStage;
U NativeHsCondition,ResumeHsCondition;
extern void HookHsCondition(void);
extern void HookInitialHsStage(void);
extern void HookOfferAction(void);
U ResumeStars, OriginalStarAssign, ResumeWeekly, SkipWeekly, ResumeInstant, SkipInstant, NativeField;
U ResumeSigning, SkipSigning, NativeRecordKey, ResumeDeadline, SkipDeadline, NullDeadline;
U ResumeAcademyReturnTarget,SkipAcademyReturnTarget;
extern void HookAcademyReturnTarget(void);
U ResumeHsOwnership,SkipHsOwnership,ResumeBulkOwnership;
extern void HookHsOwnership(void),HookBulkOwnership(void);
extern void HookStars(void),HookWeekly(void),HookInstant(void),HookSigning(void),HookDeadline(void);
extern void HookStart(void), HookProtect(void), HookAdmit(void);
U ResumeHsQuota,AcceptHsQuota;
extern void HookHsQuota(void);
U ResumePrepare,ResumeScore,ResumeQuota,AcceptQuota,ResumeSelect,AcceptSelect;
U ResumeFallbackStart,ResumeFallback,SkipFallback;
extern void HookPrepare(void),HookScore(void),HookQuota(void),HookSelect(void),HookFallbackStart(void),HookFallback(void);
U ResumePortalRetain,SkipPortalRetain,ResumePortalAllocate,NativeBoardOptionalCheck;
extern void HookPortalRetain(void),HookPortalAllocate(void);
U ResumeBulkGuard,SkipBulkGuard,ResumeCleanupProtection,AllowCleanupProtection,NativeCleanupWrapperVT;
U ResumePostWeekly,ResumePostInstant,ResumePostNsd;
U ResumeNsdResolve,SkipNsdResolve;
U ResumeNsdCollect,SkipNsdCollect;
extern void HookNsdCollect(void);
extern void HookNsdResolve(void);
U ResumeFunding,SkipFunding;
extern void HookPortalFunding(void);
U ResumeHsPostFunding;
extern void HookHsPostFunding(void);
U ResumeHsBetweenFunding;
extern void HookHsBetweenFunding(void);
extern void HookBulkGuard(void),HookCleanupProtection(void),HookPostWeekly(void),HookPostInstant(void),HookPostNsd(void);

static U q(U p) { return *(U *)p; }
static int32_t i32(U p) { return *(int32_t *)p; }
static U vfunc(Rec r, unsigned offset) { return q(q((U)r) + offset); }
static U schema(Rec r) { return ((U (*)(Rec))vfunc(r,0x38))(r); }
static U field(U s, int n) { return ((U (*)(U,int))(game_base+0x08055d60))(s,n); }
static void release(Rec r) { if(r) ((void (*)(Rec))vfunc(r,8))(r); }
static Wrap wrap(Rec r) { Wrap w = {game_base+0x0bd2e900,r,schema(r),1}; return w; }
static uint32_t scalar(Rec r, int n, int method) {
    U t=q((U)r+0x10), f=field(schema(r),n);
    /* Native Boolean getters return AL only; packed-field paths leave the
       upper EAX bits unchanged. Use their byte return ABI before widening. */
    if(method==0xf8)
        return ((unsigned char (*)(U,uint32_t,U))q(q(t)+method))(t,*(uint32_t *)((U)r+0x18),f);
    return ((uint32_t (*)(U,uint32_t,U))q(q(t)+method))(t,*(uint32_t *)((U)r+0x18),f);
}
static Rec reference(Rec r, int n) {
    Rec out=0; U t=q((U)r+0x10), f=field(schema(r),n);
    ((U (*)(U,Rec *,uint32_t,U))q(q(t)+0x128))(t,&out,*(uint32_t *)((U)r+0x18),f);
    return out;
}
static Rec as_array(Rec r) {
    if(!r) return 0;
    Rec temp=0;
    Rec *result=((Rec *(*)(Rec,Rec *,uint32_t))vfunc(r,0x28))(r,&temp,0x41525259);
    Rec out=result ? *result : 0;
    if(result) *result=0;
    release(temp);
    return out;
}
static Rec array_field(Rec r, int n) {
    Rec raw=reference(r,n), a=as_array(raw); release(raw); return a;
}
static U array_count(Rec a) { return ((U (*)(Rec))(game_base+0x037f4130))(a); }
static Rec array_at(Rec a, U n) {
    Rec out=0; U t=q((U)a+0x10);
    ((U (*)(U,Rec *,uint32_t,U))q(q(t)+0x128))(t,&out,*(uint32_t *)((U)a+0x18),n);
    return out;
}
/* At generation the game may leave an open HS/JUCO school list unallocated.
   A present record must implement the array interface. The caller owns list
   on success and applies its own bounds and requirements for a leader. */
static int hs_read_school_list(Rec recruit,unsigned stage,Rec *list,U *count) {
    *list = 0;
    *count = 0;
    Rec raw = reference(recruit,13);
    if(!raw)return stage==0 && scalar(recruit,2,0x108)<=3;
    *list = as_array(raw);
    release(raw);
    if(!*list)return 0;
    *count = array_count(*list);
    return 1;
}
static uint32_t row(Rec r) { return *(uint32_t *)((U)r+0x18); }
static int own_commit(Rec r, Rec team) {
    Wrap w=wrap(r);
    return ((unsigned char (*)(Wrap *,Rec *))(game_base+0x0862d730))(&w,&team)!=0;
}
U ResumeHsShortlist;
extern void HookHsShortlist(void);
extern unsigned char NativeHsShortlist(Wrap *,Rec *,U);
unsigned char DriHsShortlist(Wrap *,Rec *,U);
static unsigned char hs_shortlist_advice(Wrap *,Rec *,U);
static int locked_out(Rec r, Rec team, U eval) {
    Wrap w=wrap(r);
    return hs_shortlist_advice(&w,&team,eval+0xd0)!=0;
}
static int position(Rec r) {
    Rec p=reference(r,5); if(!p) return -1;
    int pos=(int)scalar(p,3,0x108); release(p); return pos;
}
static void log_line(const char *fmt, ...) {
    cache_log_calls++;
    if(log_file==INVALID_HANDLE_VALUE) return;
    if(!rm18_deep() && log30_kind(fmt)>=0)return;
    char b[1400]; va_list a; va_start(a,fmt);
    int n=vsnprintf(b,sizeof(b)-3,fmt,a); va_end(a);
    if(n<0) return; if(n>=(int)sizeof(b)-3) n=sizeof(b)-4;
    b[n]=0;
    DWORD written;
    AcquireSRWLockExclusive(&log_lock);
    if(log30_boundary(b)) {
        if(log30_state.suppressed[0] || log30_state.suppressed[1] || log30_state.suppressed[2]) {
            char summary[180];int m=snprintf(summary,sizeof(summary),
              "TEST31_LOG_REPEAT_SUMMARY,%llu,%llu,%llu,unchanged_decision_plan_defer\r\n",
              (unsigned long long)log30_state.suppressed[0],
              (unsigned long long)log30_state.suppressed[1],
              (unsigned long long)log30_state.suppressed[2]);
            if(m>0 && m<(int)sizeof(summary))WriteFile(log_file,summary,(DWORD)m,&written,0);
        }
        log30_reset(&log30_state);
    }
    if(log30_keep(&log30_state,b,(size_t)n,log30_deep)) {
        b[n++]='\r';b[n++]='\n';
        WriteFile(log_file,b,(DWORD)n,&written,0);
    }
    ReleaseSRWLockExclusive(&log_lock);
}
static Session *session(int create) {
    if(tls_index==TLS_OUT_OF_INDEXES) return 0;
    Session *s=TlsGetValue(tls_index);
    if(!s && create) {
        s=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*s));
        if(s && !TlsSetValue(tls_index,s)) { HeapFree(GetProcessHeap(),0,s);s=0; }
    }
    return s;
}
static int fail_exception(const char *where, DWORD code) {
    InterlockedExchange(&faulted,1);InterlockedExchange(&mode,-1);
    log_line("FAULT,%s,0x%08lx,behavior_disabled_restart_game",where,(unsigned long)code);
    return EXCEPTION_EXECUTE_HANDLER;
}
/* R13: the shared native lockout entry serves UI, user and CPU callers.
   Only unsigned HS/JUCO preferences become advisory. This does not grant
   eligibility, an offer, capacity, interest or ownership. */
/* Native entries go through DriHsShortlist; DRI's own lockout checks (locked_out) call the body directly. */
unsigned char DriHsShortlist(Wrap *recruit,Rec *team,U tuning) {
    cache_hook_entry(CACHE_HOOK_TEAM_GRADE);offer_cap_team(team);
    return hs_shortlist_advice(recruit,team,tuning);
}
static unsigned char hs_shortlist_advice(Wrap *recruit,Rec *team,U tuning) {
    if(mode==1 && !faulted && recruit && recruit->rec && team && *team) {
        __try {
            Rec r=recruit->rec;
            if(scalar(r,2,0x108)<=3 && scalar(r,9,0x108)<5) {
                Rec player=reference(r,5);
                unsigned owner=player?scalar(player,261,0x100):0;
                release(player);
                if(owner>=138)return 0;
            }
        } __except(fail_exception("HS31_shortlist",GetExceptionCode())){}
    }
    return NativeHsShortlist(recruit,team,tuning);
}
static int in_scope(Registers *r, Session *s) {
    U sp=(U)(r+1);
    return s && !s->portal && s->valid && !faulted && mode>=0 && s->mode==mode &&
           s->frame==r->rbp && s->team_record==q(r->rbp+0x28) /* pointer-to-Team reference below */ &&
           *(unsigned char *)(sp+0x42)==0;
}
/* In_scope compares the saved Team-reference address, not the heap Team record. */
static Rec team_record(Session *s) { return (Rec)q(s->team_record); }
static Rec board_array(Rec team) {
    Rec board=reference(team,224); if(!board) return 0;
    Rec a=array_field(board,0);release(board);return a;
}
static int fill_member(Rec target, Rec recruit, Rec team, Session *s, Member *m) {
    memset(m,0,sizeof(*m));
    m->rid=row(recruit);m->target_row=target?row(target):0xffffffffu;
    m->stage=(int)scalar(recruit,9,0x108);m->pos=position(recruit);
    if(m->pos<0 || m->pos>=POSITIONS || m->stage<0 || m->stage>6) return 0;
    Rec player=reference(recruit,5);if(!player)return 0;
    m->stars=(int)scalar(player,231,0x108);m->ovr=(int)scalar(player,191,0x100);release(player);
    if(m->stars<0 || m->stars>4 || m->ovr<0 || m->ovr>99)return 0;
    if(target) { unsigned st=scalar(target,12,0x108);m->held=st>=2 && st<=4; }
    m->own=own_commit(recruit,team);
    Rec a;U n;
    if(!hs_read_school_list(recruit,(unsigned)m->stage,&a,&n))return 0;
    if(n>64) { release(a);return 0; }
    for(U j=0;j<n;j++) {
        Rec entry=array_at(a,j);if(!entry) {release(a);return 0;}
        uint32_t tid=scalar(entry,0,0x100), inf=scalar(entry,1,0x100);
        if(j==0) m->leader=inf;
        if(tid==(unsigned)s->team) {m->rank=(int)j+1;m->influence=inf;}
        release(entry);
    }
    release(a);
    m->eligible=!locked_out(recruit,team,s->eval);
    m->native_protected=m->own || ((m->stage==1 || m->stage==2) && m->rank==1 && m->held) ||
                        (m->stage==3 && (m->rank==1 || m->eligible));
    m->challenger=dri_close_challenger(m->stage,m->rank,m->held,m->eligible,m->own,m->influence,m->leader);
    return 1;
}
static int better(Member *a, Member *b) {
    uint64_t left=(uint64_t)a->influence*b->leader, right=(uint64_t)b->influence*a->leader;
    if(left!=right) return left>right;
    if(a->rank!=b->rank) return a->rank<b->rank;
    if(a->influence!=b->influence) return a->influence>b->influence;
    return a->rid<b->rid;
}
static void policy_fault(Session *s, const char *why) {
    s->valid=0;InterlockedExchange(&faulted,1);InterlockedExchange(&mode,-1);
    log_line("FAULT,%s,pass_%llu,team_%d,behavior_disabled_restart_game",why,s->pass,s->team);
}
/* A complete read is required before any removal. The board has not yet been
   enumerated by EvaluateTeamRecruitingBoard at HookStart. Native counts and
   iterators are obtained AFTER this callback returns. */
static int read_members(Session *s, Rec team) {
    Rec a=board_array(team);if(!a) return 0;
    U n=array_count(a);if(n>MAX_BOARD) {release(a);return 0;}
    s->count=0;memset(s->board_commits,0,sizeof(s->board_commits));
    memset(s->native_open,0,sizeof(s->native_open));
    for(U j=0;j<n;j++) {
        Rec target=array_at(a,j);
        if(!target) {release(a);return 0;}
        Rec recruit=reference(target,10);
        if(!recruit) {release(target);release(a);return 0;}
        Member *m=&s->members[s->count];
        int good=fill_member(target,recruit,team,s,m);
        release(recruit);release(target);
        if(!good) {release(a);return 0;}
        for(int k=0;k<s->count;k++) if(s->members[k].rid==m->rid) {release(a);return 0;}
        s->count++;
        if(m->own && m->stage>=5) s->board_commits[m->pos]++;
        if(m->native_protected && !m->own && m->stage<5) s->native_open[m->pos]++;
    }
    release(a);return 1;
}
/* The definition follows portal_roster, allowing HS and portal accounting to
   share exact ownership and confirmed-departure helpers. */
static int hs15_refresh(Session *s,Rec team,U eval);
static int hs15_cached_block(Rec recruit,const Session *s);
static int hs15_snapshot_values(const Hs15Snapshot *s,int pos,int stars,int ovr);
static int read_roster_limits(Session *s,Rec team) {
 return hs15_refresh(s,team,s->eval);
}
static int calculate_closures(Session *s) {
 for(int pos=0;pos<POSITIONS;pos++)
  s->closed[pos]=dri_position_closed(pos,s->ideal[pos],s->limit[pos],s->commits[pos],s->projected[pos],s->flex[pos]);
 for(int j=0;j<s->count;j++) {
  Member *m=&s->members[j];
  m->policy_block=hs15_snapshot_values(&s->hs_snapshot,m->pos,m->stars,m->ovr);
  if(m->policy_block<0)return 0;
 }
 return 1;
}
static int hs15_member_closed(const Member *m) {
 if(m->own && m->stage>=5)return 0;
 /* Candidate policy already includes ordinary closure and the bounded
    quality exception. A coarse position flag must not override that result. */
 return m->policy_block;
}
static int close_filled_positions(Session *s, Rec team) {
    int to_remove=0;
    for(int j=0;j<s->count;j++) if(hs15_member_closed(&s->members[j])) to_remove++;
    for(int p=0;p<POSITIONS;p++)
        log_line("POSITION,%llu,%d,%d,%d,%d,%d,%d,%d,%s",s->pass,s->team,p,s->ideal[p],
                 p==19 || p==20?-1:s->limit[p],s->commits[p],s->returning[p],s->closed[p],s->mode?"ACTIVE":"DIAGNOSTIC");
    log_line("SPECIALISTS,%llu,%d,%d,%d,%d,%d,%d,%d,%s",s->pass,s->team,s->returning[19],s->returning[20],
             s->commits[19],s->commits[20],s->departing_specialists,s->roster_count,s->mode?"ACTIVE":"DIAGNOSTIC");
    if(!to_remove) return 1;
    U manager=q(s->eval+0xc8);
    if(!manager) {policy_fault(s,"missing_recruiting_manager");return 0;}
    int original_commits[POSITIONS];memcpy(original_commits,s->board_commits,sizeof(original_commits));
    uint32_t preserved[MAX_BOARD];int preserved_count=0;
    for(int j=0;j<s->count;j++) if(s->members[j].own && s->members[j].stage>=5) preserved[preserved_count++]=s->members[j].rid;
    for(int j=0;j<s->count;j++) {
        Member *m=&s->members[j];if(!hs15_member_closed(m)) continue;
        if(!s->mode) {
            log_line("DROP,%llu,%d,%u,%d,%d,%d,WOULD_REMOVE,0,0,0",s->pass,s->team,m->rid,m->pos,m->stage,m->held);
            continue;
        }
        Rec a=board_array(team);if(!a) {policy_fault(s,"position_missing_board");return 0;}
        U before=array_count(a);Rec target=0,recruit=0;
        for(U k=0;k<before;k++) {
            Rec t=array_at(a,k);if(!t) {release(a);policy_fault(s,"position_missing_target");return 0;}
            Rec rr=reference(t,10);
            if(!rr) {release(t);release(a);policy_fault(s,"position_missing_recruit");return 0;}
            if(row(rr)==m->rid) {target=t;recruit=rr;break;}
            release(rr);release(t);
        }
        release(a);
        if(!target || !recruit) {release(recruit);release(target);policy_fault(s,"position_target_disappeared");return 0;}
        if(position(recruit)!=m->pos || (scalar(recruit,9,0x108)>=5 && own_commit(recruit,team))) {
            release(recruit);release(target);policy_fault(s,"position_target_changed");return 0;
        }
        uint32_t offers_before=scalar(recruit,14,0x100);
        /* Native RemoveFromBoard takes Recruiting manager, Team reference,
           and a candidate whose first member is a Recruit reference.
           It owns offer-count, visit, hours, target clearing and array removal. */
        U candidate[6]={ (U)recruit,0,0,0,0,0 };
        if(faulted || mode!=1){release(recruit);release(target);return 0;}
        int rc=((int (*)(U,Rec *,void *))(game_base+0x081a7b10))(manager,&team,candidate);cache_note_removal();
        uint32_t offers_after=scalar(recruit,14,0x100);
        Rec remaining=reference(target,10);int target_cleared=!remaining;release(remaining);
        release(recruit);release(target);
        a=board_array(team);if(!a) {policy_fault(s,"position_board_after_remove");return 0;}
        U after=array_count(a);release(a);
        int offer_ok=offers_after==offers_before-(m->held && offers_before>0?1:0);
        log_line("DROP,%llu,%d,%u,%d,%d,%d,%s,%d,%u,%u",s->pass,s->team,m->rid,m->pos,m->stage,m->held,
                 rc==0 && target_cleared && after+1==before && offer_ok?"REMOVED":"FAILED",rc,offers_before,offers_after);
        if(rc!=0 || !target_cleared || after+1!=before || !offer_ok) {
            policy_fault(s,"position_native_removal_check");return 0;
        }
    }
    if(s->mode) {
        if(!read_members(s,team) || memcmp(s->board_commits,original_commits,sizeof(original_commits))!=0) {policy_fault(s,"position_post_remove_read");return 0;}
        for(int k=0;k<preserved_count;k++) {
            int found=0;for(int j=0;j<s->count;j++) if(s->members[j].rid==preserved[k] && s->members[j].own) found=1;
            if(!found) {policy_fault(s,"commitment_identity_changed");return 0;}
        }
        if(!calculate_closures(s)){policy_fault(s,"HS_closure_projection");return 0;}
        for(int j=0;j<s->count;j++) if(hs15_member_closed(&s->members[j])) {
            policy_fault(s,"position_remaining_target");return 0;
        }
    }
    return 1;
}
static void pt28_invalidate(void);
static void hs28_nsd_reset(void);
static int class31_team_annual(Rec team,U eval,int annual[22]);
static Rec t36_target(Rec team,Rec recruit);
static int t36_funds(U eval,Rec team,int *reserved,int *spent,int *left);
static int r2_quote(U eval,Rec team,Rec recruit,Rec target,int *ask,int *paid,int *wallet);
static int r2_portal_live(Rec recruit,Rec team,U eval);
static void r2_funding_tick(Rec team,U eval);
static void r3_closed_recovery(U eval,const char *site);
static void r2_ordinary_funding(U eval,Rec team,unsigned year,unsigned week,unsigned funded);
static int r2_hs_requested(Rec recruit,Rec team,U eval);

#include "cache_stack.inc"
#include "portal.inc"
#include "test29_observe.inc"
#include "hs28_score.inc"
#include "hs28_pairs.inc"
#include "portal_cleanup.inc"
#include "portal_recovery.inc"
#include "test32_battles.inc"
#include "test33_observe.inc"
static __declspec(thread) Rec r2_late_recruit,r2_late_team;
static __declspec(thread) Rec test36_hs_probe;
static int test36_hs_grant(Rec recruit,Rec team,U eval,int score,const uint64_t live[3]);
static void hs33_release_offer(Rec recruit,Rec team,U eval);
#include "hs28_coverage.inc"
#include "hs28_abandoned.inc"
#include "hs29_committable.inc"
#include "test31_initial_interest.inc"
#include "portal_nsd.inc"
#include "hs32_crowding.inc"
#include "hs32_offseason.inc"
#include "hs28_maintenance.inc"
static void portal29_observe_all(U eval,const char *site);
#include "test30_offer_counter.inc"
#include "portal_lifecycle.inc"
#include "quality.inc"
#include "portal_funding.inc"
#include "db28_requirements.inc"
#include "pp28_runtime.inc"
#include "staff28_runtime.inc"
#include "pt28_runtime.inc"
#include "eps_runtime.inc"

void DriOnStart(Registers *r) {
    cache_hook_entry(CACHE_HOOK_OTHER);
    if(mode<0 || faulted) return;
    __try {
        Session *s=session(1);if(!s) return;
        memset(s,0,sizeof(*s));
        U sp=(U)(r+1);

        Rec team=(Rec)q(r->r12);
        if(!team) return;
        Rec user=reference(team,407); if(user) {release(user);return;}
        s->team=(int)scalar(team,383,0x100);
        if(s->team<0 || s->team>=138) return;
        s->mode=mode;s->frame=r->rbp;s->team_record=r->r12;s->eval=q(sp+0x58);
        s->pass=(uint64_t)InterlockedIncrement64(&pass_counter);
        Rec season=(Rec)q(r->rbp+0x58);
        s->year=(int)scalar(season,2,0x100);s->week=(int)scalar(season,4,0x100);
        s->week_type=(int)scalar(season,5,0x108);
        if(*(unsigned char *)(sp+0x42)!=0 || s->week_type>1) {
            s->portal=1;portal_prune(team,s->eval);s->valid=!faulted;return;
        }
        if(s->week_type<0) return;
        log_line("EVAL,%llu,%d,%d,%d,%d",s->pass,s->team,s->year,s->week_type,s->week);
        U need=q(r->rbp+0x180);
        if(!need) return;
        for(int p=0;p<POSITIONS;p++) {
            int base=i32(s->eval+0x9a4+p*4), n=i32(need+p*4);
            if(base<0 || base>50 || n<0 || n>85) {log_line("SKIP,%llu,bad_quota,%d,%d,%d",s->pass,p,base,n);return;}
            s->quota[p]=dri_quota(base,n);
        }
        if(!read_members(s,team)) {policy_fault(s,"board_decode");return;}
        if(!read_roster_limits(s,team)) {policy_fault(s,"ideal_or_roster_decode");return;}
        if(!calculate_closures(s)){policy_fault(s,"HS_closure_projection");return;}
        for(int p=0;p<POSITIONS;p++) {
            int open=dri_hs_remaining(p,s->ideal[p],s->limit[p],s->commits[p],s->projected[p],s->flex[p]);
            if(open>s->quota[p])s->quota[p]=open;
            log_line("HS15_NEED,%llu,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%s",s->pass,s->team,p,
                     s->ideal[p],s->projected[p],s->commits[p],s->hs_four[p],s->hs_five[p],s->flex[p],open,
                     s->closed[p],s->mode?"ACTIVE":"DIAGNOSTIC");
        }
        if(!close_filled_positions(s,team)) return;
        for(int p=0;p<POSITIONS;p++) {
            int closed=s->closed[p];
            for(int j=0;j<s->count && closed;j++) {
                const Member *m=&s->members[j];
                if(m->pos==p && m->stage<5 && m->eligible && !m->policy_block)closed=0;
            }
            s->quota[p]=dri_protection_quota(closed,s->quota[p]);
        }
        /* Select close challengers only from the allowance remaining after native protection. */
        int order[MAX_BOARD], n=0;
        for(int j=0;j<s->count;j++) if(s->members[j].challenger && !s->members[j].native_protected) order[n++]=j;
        for(int j=1;j<n;j++) {
            int x=order[j],k=j;
            while(k>0 && better(&s->members[x],&s->members[order[k-1]])) {order[k]=order[k-1];k--;}
            order[k]=x;
        }
        for(int j=0;j<n;j++) {
            Member *m=&s->members[order[j]];int p=m->pos;
            if(s->native_open[p]+s->selected[p]<s->quota[p]) {m->chosen=1;s->selected[p]++;}
        }
        s->valid=1;
        log_line("PASS,%llu,%lu,%s,%d,%d",s->pass,(unsigned long)GetCurrentThreadId(),s->mode?"ACTIVE":"DIAGNOSTIC",s->team,s->count);
        for(int j=0;j<s->count;j++) {
            Member *m=&s->members[j];
            log_line("TARGET,%llu,%d,%u,%u,%d,%d,%d,%u,%u,%d,%d,%d,%d,%d,%d,%d",s->pass,s->team,m->rid,m->target_row,m->pos,m->stage,m->rank,m->influence,m->leader,m->held,m->eligible,m->own,m->native_protected,m->chosen,s->quota[m->pos],s->native_open[m->pos]);
        }
    } __except(fail_exception("start",GetExceptionCode())) { }
}
void DriOnProtect(Registers *r) {
    cache_hook_entry(CACHE_HOOK_OTHER);
    if(mode<0 || faulted) return;
    __try {
        Session *s=session(0);if(!in_scope(r,s)) return;
        U sp=(U)(r+1);Rec recruit=(Rec)q(r->rbp-0x58), target=(Rec)q(r->rbp-0x38);
        uint32_t rid=row(recruit);
        for(int j=0;j<s->count;j++) if(s->members[j].rid==rid && s->members[j].chosen) {
            Member now;
            if(!fill_member(target,recruit,team_record(s),s,&now) || !now.challenger) return;
            int was_replaceable=*(unsigned char *)(sp+0x44)!=0;
            int selected_prefix=0;
            U count=q(sp+0x68), vec=q(r->rbp-0x68), begin=vec?q(vec):0;
            int desired=i32(r->rbp+0x30);
            if(begin && count<=10000 && desired>=0 && desired<=128) {
                U limit=count<(U)desired?count:(U)desired;
                for(U k=0;k<limit;k++) if(q(begin+k*0x30)==(U)recruit) {selected_prefix=1;break;}
            }
            log_line("PROTECT,%llu,%d,%u,%d,%d,%u,%u,%d,%d,%s",s->pass,s->team,rid,now.pos,now.rank,now.influence,now.leader,was_replaceable,selected_prefix,s->mode?"APPLIED":"WOULD_APPLY");
            if(s->mode==1 && mode==1 && !faulted) *(unsigned char *)(sp+0x44)=0;
            return;
        }
    } __except(fail_exception("protect",GetExceptionCode())) { }
}
static int open_position(Rec recruit, Rec team, Session *s) {
    int stage=(int)scalar(recruit,9,0x108);
    if(stage>=5 || own_commit(recruit,team) || locked_out(recruit,team,s->eval)) return -1;
    return position(recruit);
}
int DriOnAdmit(Registers *r) {
    cache_hook_entry(CACHE_HOOK_OTHER);
    if(mode<0 || faulted) return 0;
    __try {
        Session *s=session(0);
        if(hs28_admit(r,(Rec)q(r->rbx)))return 1;
        if(s && s->valid && s->portal && s->frame==r->rbp && s->team_record==q(r->rbp+0x28)) {
            if(market29_portal_admission_block((Rec)q(r->rbx),team_record(s),s->eval))return 1;
            return cpu_recruit_decision((Rec)q(r->rbx),team_record(s),s->eval,"ADMIT",0)!=0;
        }
        if(!in_scope(r,s)) return 0;
        U sp=(U)(r+1);Rec team=team_record(s), incoming=(Rec)q(r->rbx);
        int p=open_position(incoming,team,s);if(p<0 || p>=POSITIONS) return 0;
        uint32_t rid=row(incoming);int existing=0,current=0;
        Rec a=board_array(team);if(!a) return 0;
        U count=array_count(a);if(count>MAX_BOARD) {release(a);return 0;}
        for(U j=0;j<count;j++) {
            Rec target=array_at(a,j);if(!target) {release(a);return 0;}
            Rec recruit=reference(target,10);
            if(!recruit) {release(target);release(a);return 0;}
            if(row(recruit)==rid) existing=1;
            if(open_position(recruit,team,s)==p) current++;
            release(recruit);release(target);
        }
        release(a);if(existing) return 0; /* leave game's duplicate handling intact */
        U board_count=q(sp+0x50);int desired=i32(r->rbp+0x30);
        U begin=q(sp+0x78), end=q(r->rbp-0x80);
        /* Native replacement-vector end is [rbp-0x80], see 0816F860. */
        int same=0,old_score=0;uint32_t outgoing=0xffffffffu;
        if(board_count>=(U)desired && end>begin && end-begin>=0x30 && (end-begin)%0x30==0) {
            Rec old=(Rec)q(end-0x30);outgoing=row(old);
            old_score=i32(end-0x30+0x28);
            same=open_position(old,team,s)==p;
        }
        int blocked=academy29_hs_block(incoming,team)?20:hs15_cached_block(incoming,s);
        if(blocked<0){policy_fault(s,"HS_admission_candidate");return 0;}
        int allowed=!blocked;
        log_line("ADMISSION,%llu,%d,%u,%u,%d,%d,%d,%d,%d,%d,%d,%s",s->pass,s->team,rid,outgoing,p,s->limit[p],current,same,allowed,i32(r->rbx+0x28),old_score,s->mode?"ACTIVE":"DIAGNOSTIC");
        return s->mode==1 && mode==1 && !faulted && !allowed;
    } __except(fail_exception("admit",GetExceptionCode())) { return 0; }
}

extern void HookPortal29GenericAction(void),HookPortal29Pitch(void);
U resume_portal29_generic_action,resume_portal29_pitch;
#include "visit29_runtime.inc"
#include "car35_common.inc"
#include "car35_emergency.inc"
#include "car35_identity.inc"
#include "car35_hooks.inc"
#include "car35_contract.inc"
#include "car35_hc_mobility.inc"
#include "car35_entrant.inc"
#include "car35_supply.inc"
#include "car35_observe.inc"
#include "test36_roster.inc"
#include "test36_opening.inc"
#include "test36r2_pursuit.inc"
#include "car35_sites.inc"

typedef struct { unsigned rva,size; unsigned char expected[32]; void (*entry)(void); } Hook;
static Hook hooks[]={
    {0x081b9936,15,{0x48,0x89,0x44,0x24,0x30,0x48,0x8d,0x4c,0x24,0x30,0xe8,0x2b,0xe0,0x46,0x00},HookHsCondition},
    {0x0815515c,16,{0x48,0x8b,0x4d,0x0f,0xba,0x09,0x00,0x00,0x00,0x4c,0x8b,0x75,0x07,0x44,0x8b,0xf8},HookInitialHsStage},
    {0x0819ae50,15,{0x48,0x89,0x5c,0x24,0x10,0x4c,0x89,0x4c,0x24,0x20,0x4c,0x89,0x44,0x24,0x18},HookOfferAction},
    {0x08172e89,15,{0xc5,0xf9,0xef,0xc0,0x45,0x33,0xed,0xba,0xe0,0x00,0x00,0x00,0x48,0x8b,0xcb},HookStart},
    {0x081731b1,14,{0x4d,0x8d,0x86,0xd0,0x00,0x00,0x00,0x49,0x8b,0xd4,0x48,0x8d,0x4d,0xa0},HookProtect},
    {0x08173574,18,{0x48,0x8b,0x44,0x24,0x58,0x4c,0x8b,0xc3,0x49,0x8b,0xd4,0x48,0x8b,0x88,0xc8,0x00,0x00,0x00},HookAdmit},
    {0x086168ac,15,{0x83,0xff,0x01,0x49,0x8b,0xcd,0x44,0x0f,0x4e,0xc0,0xe8,0x35,0xf9,0xff,0xff},HookStars},
    {0x081b9d90,18,{0x84,0xc0,0x0f,0x84,0x01,0x01,0x00,0x00,0x48,0x8b,0x4c,0x24,0x40,0xba,0x01,0x00,0x00,0x00},HookWeekly},
    {0x0815679e,18,{0x48,0x8b,0x4d,0xc7,0xba,0x09,0x00,0x00,0x00,0x4c,0x8b,0x75,0xbf,0xe8,0xb0,0xf5,0xef,0xff},HookInstant},
    {0x081b4686,14,{0x48,0x8b,0x4c,0x24,0x30,0xe8,0x40,0x2e,0xea,0xff,0x41,0x8b,0x4e,0x10},HookSigning},
    {0x081b596d,27,{0x4d,0x85,0xff,0x0f,0x84,0x9d,0x05,0x00,0x00,0x49,0x8b,0x07,0x49,0x8b,0xcf,0x4c,0x89,0x7c,0x24,0x70,0xff,0x10,0x48,0x8b,0x4c,0x24,0x38},HookDeadline},
    {0x08172d6f,20,{0x4d,0x8b,0xcf,0xc6,0x44,0x24,0x28,0x00,0x4c,0x8d,0x85,0x80,0x01,0x00,0x00,0xc6,0x44,0x24,0x20,0x00},HookPrepare},
    {0x08185b37,20,{0x48,0x8b,0x9c,0x24,0x98,0x00,0x00,0x00,0x43,0x8d,0x04,0x34,0x41,0x89,0x47,0x28,0x45,0x89,0x67,0x2c},HookScore},
    {0x0818c38c,18,{0x44,0x3b,0xd1,0x0f,0x8c,0x9c,0x01,0x00,0x00,0x45,0x85,0xd2,0x0f,0x8e,0x93,0x01,0x00,0x00},HookQuota},
    {0x081ad05f,15,{0x84,0xc0,0x0f,0x85,0x9d,0x00,0x00,0x00,0x48,0x8b,0x4d,0xc7,0x48,0x85,0xc9},HookSelect},
    {0x081ac922,14,{0x48,0x8b,0x89,0xc8,0x00,0x00,0x00,0x49,0x8b,0xd8,0x48,0x89,0x78,0xd8},HookFallbackStart},
    {0x081aca74,15,{0x48,0x8b,0x45,0x97,0x48,0x8b,0x0c,0xf0,0x48,0x89,0x4d,0x6f,0x48,0x85,0xc9},HookFallback},
    {0x08173253,14,{0x4c,0x3b,0xf8,0x73,0x67,0x48,0x8b,0x4d,0xd0,0xba,0x0a,0x00,0x00,0x00},HookPortalRetain},
    {0x08173533,15,{0x48,0x8d,0x8d,0x98,0x00,0x00,0x00,0x49,0x8b,0xf5,0xe8,0x3e,0x07,0xf9,0xff},HookPortalAllocate},
    {0x081b994d,15,{0x48,0x8b,0x4c,0x24,0x40,0xba,0x01,0x00,0x00,0x00,0x48,0x8b,0x5c,0x24,0x38},HookHsOwnership},
    {0x08163121,18,{0x48,0x8b,0x4d,0xef,0xba,0x09,0x00,0x00,0x00,0x48,0x8b,0x7d,0xe7,0xe8,0x2d,0x2c,0xef,0xff},HookBulkOwnership},
    {0x08162fcf,18,{0x48,0x8b,0x4d,0xef,0xba,0x09,0x00,0x00,0x00,0x48,0x8b,0x5d,0xe7,0xe8,0x7f,0x2d,0xef,0xff},HookBulkGuard},
    {0x081a7e0e,15,{0x84,0xc0,0x74,0x47,0x48,0x8b,0x4d,0xef,0x48,0x8d,0x1d,0xcb,0xff,0x31,0x03},HookCleanupProtection},
    {0x081b9fba,19,{0x4c,0x8b,0x6d,0x60,0x48,0x8d,0x55,0x68,0x4c,0x89,0x7d,0x68,0x49,0x8b,0x8d,0xa0,0x00,0x00,0x00},HookPostWeekly},
    {0x08156a17,14,{0x4d,0x85,0xff,0x74,0x09,0x49,0x8b,0x07,0x49,0x8b,0xcf,0xff,0x50,0x08},HookPostInstant},
    {0x081a34a1,16,{0x48,0x8d,0x4d,0xe8,0x41,0xc6,0x07,0x00,0x41,0xc6,0x85,0xa1,0x01,0x00,0x00,0x00},HookPostNsd},
    {0x08171f3a,19,{0x45,0x33,0xf6,0x4c,0x8d,0x3d,0xa4,0x5e,0x35,0x03,0x4d,0x85,0xe4,0x0f,0x84,0x71,0x09,0x00,0x00},HookPortalFunding},
    {0x081a2ff0,17,{0x45,0x33,0xf6,0x4c,0x89,0x74,0x24,0x48,0x4d,0x85,0xe4,0x0f,0x84,0xa0,0x04,0x00,0x00},HookNsdResolve},
    {0x081a2e3d,16,{0x4d,0x85,0xe4,0x0f,0x84,0x7e,0x01,0x00,0x00,0x0f,0xb6,0x9d,0xc0,0x00,0x00,0x00},HookNsdCollect},
    {0x08407000,17,{0x48,0x89,0x5c,0x24,0x18,0x55,0x57,0x41,0x54,0x41,0x55,0x41,0x57,0x48,0x83,0xec,0x60},HookEpsGrade},
    {0x084073e0,16,{0x48,0x89,0x5c,0x24,0x18,0x55,0x56,0x41,0x56,0x48,0x83,0xec,0x70,0x49,0x63,0xf0},HookEpsGeneric},
    {0x084333d0,17,{0x48,0x89,0x5c,0x24,0x20,0x55,0x56,0x57,0x41,0x55,0x41,0x57,0x48,0x8d,0x6c,0x24,0xc9},HookEpsRequired},
    {0x084215b0,15,{0x48,0x8b,0xc4,0x48,0x89,0x48,0x08,0x55,0x48,0x8d,0xa8,0x38,0xff,0xff,0xff},HookEpsUpdate},
    {0x08422886,15,{0x48,0x8d,0x55,0x6f,0x49,0x8b,0xcd,0xe8,0xee,0xbb,0xfd,0xff,0x45,0x33,0xc9},HookPPBeforeAge},
    {0x08422f9d,15,{0x44,0x8b,0xc6,0x48,0x8d,0x4d,0x87,0x41,0x8b,0xd4,0xe8,0x14,0xe1,0x1e,0x00},HookPPPublish},
    {0x0841f140,16,{0x48,0x8b,0xc4,0x55,0x57,0x48,0x8d,0x68,0xa1,0x48,0x81,0xec,0xa8,0x00,0x00,0x00},HookStaff28Update},
    {0x0840a950,16,{0x40,0x55,0x53,0x56,0x57,0x41,0x54,0x41,0x56,0x41,0x57,0x48,0x8d,0x6c,0x24,0xc0},HookPt28Grade},
    {0x0840b470,14,{0x40,0x55,0x53,0x56,0x57,0x41,0x55,0x41,0x56,0x48,0x8d,0x6c,0x24,0xd1},HookProximity28Grade},
    {0x08407610,15,{0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x6c,0x24,0x18,0x48,0x89,0x74,0x24,0x20},HookPPGroupGrade},
    {0x081728dc,15,{0x8b,0x9e,0x20,0x02,0x00,0x00,0x48,0x8b,0x8e,0xe0,0x00,0x00,0x00,0xff,0xc3},HookHsPostFunding},
    {0x08172351,15,{0x48,0x8b,0x7d,0xe0,0x33,0xc0,0x89,0x86,0x28,0x02,0x00,0x00,0x44,0x8b,0xe8},HookHsBetweenFunding},
    {0x0817c03f,18,{0x44,0x3b,0xd1,0x0f,0x8c,0xb5,0x00,0x00,0x00,0x45,0x85,0xd2,0x0f,0x8e,0xac,0x00,0x00,0x00},HookHsQuota},
    {0x0862d9f0,16,{0x48,0x89,0x5c,0x24,0x18,0x48,0x89,0x6c,0x24,0x20,0x41,0x55,0x41,0x56,0x41,0x57},HookHsShortlist},
    {0x0844a4e2,15,{0x4c,0x8d,0x5c,0x24,0x60,0x8b,0xc6,0x49,0x8b,0x5b,0x28,0x49,0x8b,0x6b,0x30},HookVisit29Capacity},
    {0x0814d911,17,{0x48,0x8b,0x5d,0xef,0x4c,0x8b,0x75,0xf7,0x49,0x3b,0xde,0x0f,0x84,0x77,0x01,0x00,0x00},HookVisit29Dates},
    {0x0814da83,14,{0x48,0x8b,0xd3,0x49,0x8b,0xcf,0x44,0x8b,0xe0,0xe8,0x8f,0xaa,0x06,0x00},HookVisit29Booked},
    {0x08177b34,15,{0x84,0xc0,0x0f,0x84,0x3a,0x01,0x00,0x00,0x48,0x8b,0x8b,0xc8,0x00,0x00,0x00},HookVisit29Eligible},
    {0x081a400c,14,{0x0f,0xb6,0xf8,0x84,0xc0,0x74,0x47,0x44,0x8b,0x8d,0x80,0x00,0x00,0x00},HookVisit29Completed},
{0x0814ee48,15,{0x44,0x8b,0x4d,0x7f,0x48,0x8d,0x45,0xef,0x49,0x8b,0x8e,0xc8,0x00,0x00,0x00},HookPortal29GenericAction},
 {0x0814fb09,15,{0x44,0x8b,0x4d,0x84,0x48,0x8d,0x45,0xb0,0x48,0x8b,0x8e,0xc8,0x00,0x00,0x00},HookPortal29Pitch},
 {0x081b5cb7,14,{0x48,0x8b,0x4c,0x24,0x50,0x48,0x8d,0x54,0x24,0x40,0x48,0x83,0xc1,0x08},HookAcademyReturnTarget},
};
#define HOOK_COUNT ((int)(sizeof(hooks)/sizeof(hooks[0])))
static int verify_build(void) {
    IMAGE_DOS_HEADER *d=(void *)game_base;
    if(d->e_magic!=IMAGE_DOS_SIGNATURE) return 0;
    IMAGE_NT_HEADERS64 *n=(void *)(game_base+d->e_lfanew);
    if(n->Signature!=IMAGE_NT_SIGNATURE || n->FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64 ||
       n->OptionalHeader.SizeOfImage!=EXPECTED_IMAGE_SIZE || n->FileHeader.TimeDateStamp!=EXPECTED_TIMESTAMP) return 0;
    for(unsigned j=0;j<sizeof(guard_signatures)/sizeof(guard_signatures[0]);j++)
        if(memcmp((void *)(game_base+guard_signatures[j].rva),guard_signatures[j].bytes,guard_signatures[j].size)) return 0;
    for(int j=0;j<HOOK_COUNT;j++) if(memcmp((void *)(game_base+hooks[j].rva),hooks[j].expected,hooks[j].size)) return 0;
    return car35_verify();
}
#ifdef DRI_EMULATION_TEST
__declspec(dllexport) int DriTestTU1029VerifyBuild(U base) { game_base=base;return verify_build(); }
#endif
#include "car35_install.inc"
static int install_hooks(void) { return car35_install_all(); }
static void open_log(void) {
    wchar_t deep[8]={0};
    log30_deep=GetEnvironmentVariableW(L"DRI_TEST34_LOG_DEEP",deep,8)==1 && deep[0]==L'1';
    if(rm18_deep())log30_deep=1;
    log30_reset(&log30_state);
    wchar_t path[32768];DWORD n=GetModuleFileNameW(self_module,path,32000);
    if(!n || n>=32000) return;
    while(n && path[n-1]!=L'\\') n--;
    swprintf(path+n,32768-n,L"DRI-Test34-Deep-Logging.txt");
    DWORD attributes=GetFileAttributesW(path);
    if(attributes!=INVALID_FILE_ATTRIBUTES && !(attributes&FILE_ATTRIBUTE_DIRECTORY))log30_deep=1;
    SYSTEMTIME t;GetLocalTime(&t);
    swprintf(path+n,32768-n,L"RM18R22_Recruiting_%04u%02u%02u_%02u%02u%02u_%lu.csv",t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,(unsigned long)GetCurrentProcessId());
    log_file=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ,0,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,0);
}
__declspec(dllexport) DWORD WINAPI DriInitialize(void *requested_mode) {
    LONG wanted=(LONG)(U)requested_mode;
    if(wanted<0 || wanted>1) return 10;
    if(faulted) return 11;
    if(installed) {InterlockedExchange(&mode,wanted);log_line("MODE,%ld",wanted);return 0;}
    game_base=(U)GetModuleHandleW(0);open_log();
    if(log_file==INVALID_HANDLE_VALUE) return 12;
    log_line("TEST31_LOG_MODE,%s,unchanged_candidate_dedup_only",log30_deep?"DEEP":"SUMMARY");
    log_line("DRI_LOG_PROFILE,RC2,recruiting=%s,roster_development=%s",
        log30_deep?"DEEP":"QUIET",rm18_deep()?"DEEP":"QUIET");
    log_line("DRI_BASELINE,DRI_PORTAL_TEST28R14,retained_policy_and_ABI");
    log_line("DRI_PORTAL_TEST34R4,build_2026_11_03_r4,HS_4to5star_soft8_hard12_2to3star_soft3_hard6,PORTAL_primary_soft4_attention6_hard6,academy_HS4_allowed_HS5_blocked_no_portal_pursuit,EPS8_cadence,FTC_Master_R03_unchanged,bounded_vacancy_class_plan_capacity,resumable_class_recovery,post_week_refill,tentative_plan_review,gate_rejection_diagnostics,actual_HS_offer_cap,competitive_primary_renewal,native_two_pass_funding,earned_native_commitment,changed_candidate_log_dedup,checkpoint_primary_continuity,pre_maintenance_plan_restore,ordinary_recovery_with_secured_extras,all_phase_HS_admission");
    if(!verify_build()) {log_line("REFUSED,build_or_signature_mismatch");return 13;}
    tls_index=TlsAlloc();if(tls_index==TLS_OUT_OF_INDEXES) return 14;
    car35_tls_index=TlsAlloc();car35_gate_tls=TlsAlloc();car35_generation_tls=TlsAlloc();car35_emergency_tls=TlsAlloc();
    if(car35_tls_index==TLS_OUT_OF_INDEXES || car35_gate_tls==TLS_OUT_OF_INDEXES || car35_generation_tls==TLS_OUT_OF_INDEXES || car35_emergency_tls==TLS_OUT_OF_INDEXES)return 14;
    ResumeHsShortlist=game_base+0x0862da00;
    NativeHsCondition=game_base+0x08627970;ResumeHsCondition=game_base+0x081b9945;
    ResumeOfferAction=game_base+0x0819ae5f;ResumeInitialHsStage=game_base+0x0815516c;SkipInitialHsStage=game_base+0x0815519b;
    ResumeStart=game_base+0x08172e98;ResumeProtect=game_base+0x081731bf;
    ResumeAdmit=game_base+0x08173586;SkipAdmit=game_base+0x08173663;
    ResumeStars=game_base+0x086168bb;OriginalStarAssign=game_base+0x086161f0;
    ResumeHsOwnership=game_base+0x081b995c;SkipHsOwnership=game_base+0x081b9a61;ResumeBulkOwnership=game_base+0x08163133;
    ResumeWeekly=game_base+0x081b9da2;SkipWeekly=game_base+0x081b9e99;
    ResumeInstant=game_base+0x081567b0;SkipInstant=game_base+0x08156a58;NativeField=game_base+0x08055d60;
    ResumeSigning=game_base+0x081b4694;SkipSigning=game_base+0x081b49ae;NativeRecordKey=game_base+0x080574d0;
    ResumeAcademyReturnTarget=game_base+0x081b5cc5;SkipAcademyReturnTarget=game_base+0x081b5ed8;
    ResumeDeadline=game_base+0x081b5988;SkipDeadline=game_base+0x081b5f05;NullDeadline=game_base+0x081b5f13;
    quality_tls=TlsAlloc();if(quality_tls==TLS_OUT_OF_INDEXES)return 14;
    cleanup_tls=TlsAlloc();if(cleanup_tls==TLS_OUT_OF_INDEXES)return 14;
    ResumeBulkGuard=game_base+0x08162fe1;SkipBulkGuard=game_base+0x08163168;
    ResumeCleanupProtection=game_base+0x081a7e1d;AllowCleanupProtection=game_base+0x081a7e59;NativeCleanupWrapperVT=game_base+0x0b4c7de8;
    ResumeFunding=game_base+0x08171f4d;SkipFunding=game_base+0x081728be;
    ResumeHsPostFunding=game_base+0x081728eb;
    ResumeHsBetweenFunding=game_base+0x08172360;
    ResumePostWeekly=game_base+0x081b9fcd;ResumePostInstant=game_base+0x08156a25;ResumePostNsd=game_base+0x081a34b1;
    ResumeNsdCollect=game_base+0x081a2e4d;SkipNsdCollect=game_base+0x081a2fc4;
    ResumeNsdResolve=game_base+0x081a3001;SkipNsdResolve=game_base+0x081a34a1;
    ResumePrepare=game_base+0x08172d83;
    ResumeScore=game_base+0x08185b4b;
    ResumeHsQuota=game_base+0x0817c051;AcceptHsQuota=game_base+0x0817c0fd;
    ResumeQuota=game_base+0x0818c39e;
    ResumeSelect=game_base+0x081ad06e;
    ResumeFallbackStart=game_base+0x081ac930;
    ResumeFallback=game_base+0x081aca83;
    AcceptQuota=game_base+0x0818c531;AcceptSelect=game_base+0x081ad104;SkipFallback=game_base+0x081acc78;
    ResumePortalRetain=game_base+0x08173261;SkipPortalRetain=game_base+0x081732bf;ResumePortalAllocate=game_base+0x08173542;NativeBoardOptionalCheck=game_base+0x08103c80;
    ResumeEpsGrade=game_base+0x08407011;ResumeEpsGeneric=game_base+0x084073f0;
    ResumeEpsRequired=game_base+0x084333e1;ResumeEpsUpdate=game_base+0x084215bf;
    ResumePPBeforeAge=game_base+0x08422895;NativePPAge=game_base+0x083fe480;
    ResumePPPublish=game_base+0x08422fac;NativePPSetGrade=game_base+0x086110c0;
    ResumeStaff28Update=game_base+0x0841f150;ResumePt28Grade=game_base+0x0840a960;ResumeProximity28Grade=game_base+0x0840b47e;ResumePPGroupGrade=game_base+0x0840761f;
    ResumeVisit29Capacity=game_base+0x0844a4f1;
    ResumeVisit29Dates=game_base+0x0814d922;SkipVisit29Dates=game_base+0x0814da99;
    ResumeVisit29Booked=game_base+0x0814da91;NativeVisit29Account=game_base+0x081b8520;
ResumeVisit29Eligible=game_base+0x08177b43;SkipVisit29Eligible=game_base+0x08177c76;
ResumeVisit29Completed=game_base+0x081a401a;SkipVisit29Completed=game_base+0x081a405a;
    resume_portal29_generic_action=game_base+0x0814ee57;
    resume_portal29_pitch=game_base+0x0814fb18;
    if(!install_hooks()) {log_line("REFUSED,hook_install_failed_or_thread_in_patch_site");return 15;}
    InterlockedExchange(&installed,1);InterlockedExchange(&mode,wanted);
    log_line("RC4_BUILD,ELITE_QB_COMPARE,FUNDED_QB_BACKUP,NIL_ENTRY_ONE_GRADE");
    log_line("TEST36_BUILD,EOS_PLV_TP1_NSD,portal_hours_1000,portal_caps_4_6,HS_abandonment_refresh,backup_NIL_reprice");
    log_line("TEST36R2H1_BUILD,native_opening_destination_publication,rollback_before_acceptance,six_R2_fixes_preserved");
    log_line("TEST36R2_BUILD,six_fixes,native_player_score_input,seed_grace,last_live_offer_recovery,promoted_NIL_wallet,accepted_HC_job_receipt,saved_TP1_owner,HS_board_and_funded_retry");
    log_line("CAR35_BUILD,Test36R3_Portal_2026_11_06,baseline_Test34R4_FTC_Master_R03,paired_FTC_Test36,user_accepted_offer_retention,employed_HC_nonP4_coord_guard,shared_hiring_evaluation,shared_native_available_funds,contract_retention_CAR3T2,identity_saved_specialty,veteran_retention_native_target");
    log_line("READY,%s,game_base_%p",wanted?"ACTIVE":"DIAGNOSTIC",(void *)game_base);
    FlushFileBuffers(log_file);return 0;
}
__declspec(dllexport) DWORD WINAPI DriDisable(void *unused) {
    (void)unused;InterlockedExchange(&mode,-1);
    log_line("DISABLED,behavior_passthrough_until_process_exit");
    if(log_file!=INVALID_HANDLE_VALUE) FlushFileBuffers(log_file);return 0;
}
BOOL WINAPI DllMain(HINSTANCE h,DWORD reason,LPVOID reserved) {
    (void)reserved;
    if(reason==DLL_PROCESS_ATTACH) self_module=h;
    /* No game access, patching, waiting, or logging under the loader lock. */
    return TRUE;
}
#ifdef DRI_EMULATION_TEST
/* Test-build-only entry. Never present in the shipped DLL. */
__declspec(dllexport) void DriTestConfigure(U base,int wanted,DWORD index) {
    game_base=base;mode=wanted;tls_index=index;quality_tls=index+1;cleanup_tls=index+2;faulted=0;
    ResumeHsShortlist=base+0x0862da00;
    ResumeEpsGrade=base+0x08407011;ResumeEpsGeneric=base+0x084073f0;
    ResumeEpsRequired=base+0x084333e1;ResumeEpsUpdate=base+0x084215bf;
    ResumePPBeforeAge=base+0x08422895;NativePPAge=base+0x083fe480;
    ResumePPPublish=base+0x08422fac;NativePPSetGrade=base+0x086110c0;
    ResumeStaff28Update=base+0x0841f150;ResumePt28Grade=base+0x0840a960;ResumeProximity28Grade=base+0x0840b47e;ResumePPGroupGrade=base+0x0840761f;
}
__declspec(dllexport) U DriTest29R4ReturnTargetHook(void){ResumeAcademyReturnTarget=game_base+0x081b5cc5;SkipAcademyReturnTarget=game_base+0x081b5ed8;return (U)HookAcademyReturnTarget;}
__declspec(dllexport) U DriTest29R3DeadlineHook(void){ResumeDeadline=game_base+0x081b5988;SkipDeadline=game_base+0x081b5f05;NullDeadline=game_base+0x081b5f13;return (U)HookDeadline;}
__declspec(dllexport) void DriTestVisit29Capacity(Registers *r){DriOnVisit29Capacity(r);}
__declspec(dllexport) void DriTestVisit29Dates(Registers *r){DriOnVisit29Dates(r);}
__declspec(dllexport) void DriTestVisit29Booked(Registers *r){DriOnVisit29Booked(r);}
__declspec(dllexport) void DriTestVisit29Hooks(U *out){ResumeVisit29Capacity=game_base+0x0844a4f1;ResumeVisit29Dates=game_base+0x0814d922;SkipVisit29Dates=game_base+0x0814da99;ResumeVisit29Booked=game_base+0x0814da91;NativeVisit29Account=game_base+0x081b8520;out[0]=(U)HookVisit29Capacity;out[1]=(U)HookVisit29Dates;out[2]=(U)HookVisit29Booked;}
__declspec(dllexport) void DriTestPortal29ExecutionHooks(U *out){resume_portal29_generic_action=game_base+0x0814ee57;resume_portal29_pitch=game_base+0x0814fb18;out[0]=(U)HookPortal29GenericAction;out[1]=(U)HookPortal29Pitch;}
__declspec(dllexport) U DriTestHsShortlistHook(void){return (U)HookHsShortlist;}
__declspec(dllexport) int DriTestEpsGrade(U ctx,Rec *team,int kind,Rec *p){return DriEpsGrade(ctx,team,kind,p);}
__declspec(dllexport) int DriTestEpsGeneric(U ctx,Rec *team,int kind,int arch){return DriEpsGeneric(ctx,team,kind,arch);}
__declspec(dllexport) int DriTestEpsRequired(U ctx,Rec *info,Rec *p,Rec *team){return DriEpsRequired(ctx,info,p,team);}
__declspec(dllexport) void DriTestEpsSeed(Rec franchise,int year,int week,int type){
 eps_reset();eps_state.dynasty=eps_record_key(franchise);eps_state.league=scalar(franchise,28,0x100);
 Rec schools=array_field(franchise,45);
 if(schools){
  unsigned count=(unsigned)array_count(schools);
  for(unsigned i=0;i<count;i++){
   Rec school=array_at(schools,i);
   if(school){unsigned id=scalar(school,383,0x100);if(id<EPS_TEAMS)eps_state.schools[id]=eps_record_key(school);}
   release(school);
  }
  release(schools);
 }
 eps_state.year=year;eps_state.index=0;eps_state.stage=2;eps_state.week=week;eps_state.type=type;eps_state.valid=1;
 for(int t=0;t<138;t++)for(int p=0;p<EPS_PROFILES;p++){eps_state.output.grade[t][p].grade=4;eps_state.output.grade[t][p].status=1;eps_state.output.grade[t][p].rank=40.5;}
}
__declspec(dllexport) void DriTestEpsSet(int team,int profile,int grade,int status){eps_state.output.grade[team][profile].grade=grade;eps_state.output.grade[team][profile].status=status;}
__declspec(dllexport) int DriTestEpsEnsure(U c,int force,int publish){
 EpsTrace trace={"fixture","current_dynasty",-1,-1};return eps_ensure(c,force,publish,&trace);
}
__declspec(dllexport) void DriTestEpsReset(void){eps_reset();}
__declspec(dllexport) int DriTestEpsCache(Rec franchise,int publish,U *hash){*hash=(U)eps_cache(franchise,&eps_state.output,publish,eps_state.schools);return *hash!=0;}
__declspec(dllexport) void DriTestEpsAddresses(U *out){out[0]=(U)NativeEpsGrade;out[1]=(U)NativeEpsGeneric;out[2]=(U)NativeEpsRequired;out[3]=(U)NativeEpsUpdate;out[4]=(U)HookEpsGrade;out[5]=(U)HookEpsGeneric;out[6]=(U)HookEpsRequired;out[7]=(U)HookEpsUpdate;}
__declspec(dllexport) int DriTestPortalDecision(Rec recruit,Rec team,U context) {return portal_decision(recruit,team,context,"FIXTURE");}
__declspec(dllexport) U DriTestHsQuotaHook(void){ResumeHsQuota=game_base+0x0817c051;AcceptHsQuota=game_base+0x0817c0fd;return (U)HookHsQuota;}
__declspec(dllexport) int DriTestEligibility(Rec r,Rec t,U e){return native_recruit_eligible(e,t,r);}
__declspec(dllexport) int DriTest34R3Admission(Rec r,Rec t,U e){return cpu_recruit_decision(r,t,e,"HS_WEEKLY",1);}
__declspec(dllexport) int DriTestHsDecision(Rec recruit,Rec team,U context) {return hs15_decision(recruit,team,context,"FIXTURE",1);}
__declspec(dllexport) void DriTestHsRecoverTeam(Rec team,U eval) {hs15_recover_team(team,eval);}
__declspec(dllexport) int DriTestHsSnapshot(Rec team,U eval,int *out) {
 Hs15Snapshot s;if(!hs15_collect(&s,team,eval))return 0;
 for(int p=0;p<22;p++) {
  out[p]=s.ideal[p];out[22+p]=s.count[p];out[44+p]=s.returning[p];
  out[66+p]=s.hs[p];out[88+p]=s.four[p];out[110+p]=s.five[p];out[132+p]=s.flex[p];
 }
 out[154]=s.qb85;out[155]=s.total;out[156]=s.total_ideal;return 1;
}
__declspec(dllexport) int DriTestQualityFault(void){return faulted;}
__declspec(dllexport) int DriTestPendingOwner(Rec recruit,Rec player,int *owner){return portal_signed_destination(recruit,player,owner);}
__declspec(dllexport) void DriTestOwnershipReset(void){portal_ownership_clear_locked();hs28_nsd_reset();}
__declspec(dllexport) int DriTestHardOwner(Rec recruit,U eval,int *owner){return portal_hard_owner(recruit,eval,owner);}
__declspec(dllexport) int DriTestActiveOffer(Rec recruit,Rec team){return portal_active_offer(recruit,team);}
__declspec(dllexport) int DriTestAcademy(unsigned tid){return portal_academy(tid);}
__declspec(dllexport) void DriTestRecoverMarket(U eval,int opening_grace){portal_recover_market(eval,opening_grace);}
__declspec(dllexport) void DriTestDeadlineRecovery(Rec rec,U eval){portal_deadline_recover(rec,eval);}
__declspec(dllexport) int DriTestCleanupAll(U eval){return portal_cleanup_all(eval);}
__declspec(dllexport) int DriTestCleanupRivals(Rec recruit,unsigned owner,U eval){return portal_cleanup_rivals(recruit,owner,eval);}
__declspec(dllexport) void DriTestCleanupProtection(Registers *r){DriOnPortalCleanupProtection(r);}
__declspec(dllexport) void DriTestMaintenance(U eval){portal_maintenance(eval);}
__declspec(dllexport) int DriTestProjectedRoom(Rec team,U context,int pos,PortalQuality *out) {
 PortalRoster s;if(!portal_roster(&s,team,context,pos))return 0;*out=s.projected;return 1;
}
__declspec(dllexport) U DriTest31ConditionHook(void){NativeHsCondition=game_base+0x08627970;ResumeHsCondition=game_base+0x081b9945;ResumeHsOwnership=game_base+0x081b995c;SkipHsOwnership=game_base+0x081b9a61;return (U)HookHsCondition;}
__declspec(dllexport) U DriTest31InitialHook(void){ResumeInitialHsStage=game_base+0x0815516c;SkipInitialHsStage=game_base+0x0815519b;return (U)HookInitialHsStage;}
__declspec(dllexport) U DriTest31OfferHook(void){ResumeOfferAction=game_base+0x0819ae5f;return (U)HookOfferAction;}
__declspec(dllexport) U DriTest33BetweenFundingHook(void){ResumeHsBetweenFunding=game_base+0x08172360;return (U)HookHsBetweenFunding;}
__declspec(dllexport) unsigned DriTest30QualitySize(void){return sizeof(PortalQuality);}
__declspec(dllexport) int DriTest30ProjectedRooms(Rec team,U context,PortalQuality *out){return portal_quality_rooms(out,0,0,team,context);}
__declspec(dllexport) int DriTest30CurrentRooms(Rec team,U context,PortalQuality *out){
 PortalQuality projected[POSITIONS];return portal_quality_rooms(projected,out,0,team,context);
}
__declspec(dllexport) int DriTestQualityRoom(Rec team,U context,int pos,PortalQuality *out) {
 PortalRoster r;if(!portal_roster(&r,team,context,pos))return 0;*out=r.quality;return 1;
}
#endif

#include "car35_test_exports.inc"
#ifdef DRI_EMULATION_TEST
#include "test36_fixture.inc"
#endif
