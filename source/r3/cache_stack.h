/* Cache stack declarations used before cache_stack.inc (see that file and docs/SPEED.md). */
#ifndef RM18R22_CACHE_STACK_H
#define RM18R22_CACHE_STACK_H
#define CACHE_OFF 0
#define CACHE_SHADOW 1
#define CACHE_FAST 2
/* The shipped build always answers from the caches: cache_stack_mode is the constant CACHE_FAST, and the
   original-code (CACHE_OFF) and comparison (CACHE_SHADOW) branches compile away. DRI_SPEED_TEST_MODES, which
   scripts/build.py never sets, builds a verification binary in which it is a variable: start value
   DRI_TEST_CACHE_STACK (default CACHE_FAST); the property test also sets it at run time. */
#ifdef DRI_SPEED_TEST_MODES
#ifndef DRI_TEST_CACHE_STACK
#define DRI_TEST_CACHE_STACK CACHE_FAST
#endif
static int cache_stack_mode=DRI_TEST_CACHE_STACK;
#else
static const int cache_stack_mode=CACHE_FAST;
#endif
/* Hook classes for cache_hook_entry (the first statement of every native-to-DRI hook body).
   CAP_QUIET: does not end a school's run of offer-capacity checks. CAP_TEAM: entered inside one school's own
   action loop; ends the run unless its Team is the cached applicant (offer_cap_team). GEN_QUIET: one of the
   read-only grade/score/shortlist hooks that the natives DRI itself calls enter; does not advance the memo
   generation. */
#define CACHE_HOOK_CAP_QUIET 1
#define CACHE_HOOK_CAP_TEAM 2
#define CACHE_HOOK_GEN_QUIET 4
#define CACHE_HOOK_OTHER 0
#define CACHE_HOOK_GRADE (CACHE_HOOK_CAP_QUIET|CACHE_HOOK_GEN_QUIET)  /* EPS grade/required/generic, PT28, proximity */
#define CACHE_HOOK_OFFER CACHE_HOOK_CAP_QUIET                         /* the offer action itself */
#define CACHE_HOOK_TEAM_GRADE (CACHE_HOOK_CAP_TEAM|CACHE_HOOK_GEN_QUIET) /* lockout check, PP group grade */
#define CACHE_HOOK_TEAM CACHE_HOOK_CAP_TEAM                           /* visit eligibility, executed-action witnesses */
#define CACHE_HOOK_SCORE CACHE_HOOK_GEN_QUIET                         /* portal score */
static void cache_hook_entry(int kind);
static void cache_note_write(void);
static void cache_note_removal(void);
static void cache_note_pt28(void);
static void cache_note_class(void);
static void offer_cap_team(const Rec *slot);
static void offer_cap_team_at(U where);
static uint64_t cache_log_calls; /* every log_line call, counted before any filter */
#endif
