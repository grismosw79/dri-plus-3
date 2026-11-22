/* Cache stack (docs/SPEED.md).

   The weekly HS maintenance, the post-week HS block and the offseason portal callbacks re-read the same game
   state many times inside one synchronous block of DRI code: the same league scan, the same school's roster
   snapshot or room, the same (school, recruit) eligibility. The caches in this stack answer such a repeat from
   the first read, and only inside a block in which nothing can have changed the answer.

   cache_stack_mode (cache_stack.h): the shipped build is always CACHE_FAST (2) and answers from the cache. Only a
   verification build (DRI_SPEED_TEST_MODES) has the other two values. CACHE_OFF (0) runs the original code; every
   cache entry point falls through to it before touching any cache state, and nothing is allocated. CACHE_SHADOW (1):
   the original computation runs and decides, the cached answer is computed beside it, and every difference is
   written as a CACHE_STACK_MISMATCH row. In SHADOW and FAST one CACHE_STACK_SUMMARY row per cache is written after
   each post-week and post-NSD hook (queries, hits, compares, mismatches).

   Window. A cache answers only while a window is open. A window is one synchronous block of DRI code on the
   game thread: the pre-plan HS maintenance, the post-week HS block, the read-only plan loop of the portal
   market recovery, the portal market forecast, or the decision part of one native-pass callback. Windows do
   not nest, never span two hook calls, and the post-week window is paused around t36_open (opening
   publication can publish destinations).

   Three counters decide what an entry stays valid for:
   - cache.gen (generation) advances at every window open, close and pause/resume, at every DRI game write
     (record setters, board admits, offers, actions, funding, publication: cache_note_write), after every
     native board removal (cache_note_removal), at every PT28 index invalidation (which every ownership-ledger
     change performs: cache_note_pt28), and at the entry of every native-to-DRI hook other than the read-only
     grade, score and shortlist hooks that run inside the natives DRI itself calls. So within one generation the
     game thread has run only DRI code that wrote nothing and read-only natives.
   - cache.epoch advances at every window open and at every sub-phase boundary of the two HS windows
     (cache_phase_boundary). The league-scan caches and the snapshot and room memos never cross one.
   - cache.class_epoch advances at every test29_class_invalidate.
   The offer-capacity cache has its own counter (cache.offer_epoch, see cache_hook_entry). */
static Rec context_ref(U context,int n); /* portal.inc */
static int cache_on(void){return cache_stack_mode==CACHE_SHADOW || cache_stack_mode==CACHE_FAST;}
static int cache_shadow(void){return cache_stack_mode==CACHE_SHADOW;}
static int cache_fast(void){return cache_stack_mode==CACHE_FAST;}

/* ---- counters and rows ----------------------------------------------------------------------------------- */
enum {CACHE_OWNER_INDEX,CACHE_SECURED_LIST,CACHE_COMMITS_LIST,CACHE_SEASON_SCOPE,CACHE_PT28_ROSTER,
      CACHE_OFFER_CAPACITY,CACHE_ELIGIBILITY,CACHE_RECOVERY_COST,CACHE_SNAPSHOT,CACHE_ROOM,CACHE_PT28_READS,
      CACHE_KINDS};
static const char *const cache_kind_names[CACHE_KINDS]={"OWNER_INDEX","SECURED_LIST","COMMITS_LIST","SEASON_SCOPE",
 "PT28_ROSTER","OFFER_CAPACITY","ELIGIBILITY","RECOVERY_COST","SNAPSHOT","ROOM","PT28_READS"};
/* queries: lookups; hits: answered (FAST) or would have been answered (SHADOW) from the cache; compares and
   mismatch: SHADOW checks; stale: FAST found a cached entry that no longer qualified (must stay 0); unclean:
   live calls not stored because they had a side effect. */
typedef struct {uint64_t queries,hits,compares,mismatch,stale,unclean;} CacheCount;
static CacheCount cache_counts[CACHE_KINDS];
static unsigned cache_mismatch_rows;
/* Counts a SHADOW difference; returns 1 while CACHE_STACK_MISMATCH rows may still be written (first 2000). */
static int cache_mismatch(int kind){cache_counts[kind].mismatch++;return cache_mismatch_rows++<2000;}
static void cache_summary(const char *site,U eval) {
 if(!cache_on() || log_file==INVALID_HANDLE_VALUE)return;
 unsigned year=0,type=0,week=0;
 Rec f=eval?context_ref(eval,6):0,season=f?reference(f,44):0;
 if(season){year=scalar(season,2,0x100);type=scalar(season,5,0x108);week=scalar(season,4,0x100);}
 release(season);release(f);
 for(int k=0;k<CACHE_KINDS;k++) {
  const CacheCount *c=&cache_counts[k];if(!c->queries && !c->compares && !c->mismatch && !c->stale)continue;
  log_line("CACHE_STACK_SUMMARY,%s,%u,%u,%u,%s,%s,queries=%llu,hits=%llu,compares=%llu,mismatch=%llu,stale=%llu,unclean=%llu",
   site,year,type,week,cache_shadow()?"SHADOW":"FAST",cache_kind_names[k],(unsigned long long)c->queries,
   (unsigned long long)c->hits,(unsigned long long)c->compares,(unsigned long long)c->mismatch,
   (unsigned long long)c->stale,(unsigned long long)c->unclean);
 }
 memset(cache_counts,0,sizeof(cache_counts));
}

/* ---- window state and the validity counters --------------------------------------------------------------- */
static struct {
 int active,paused;const char *site;DWORD tid; /* the open window, and the thread that opened it */
 unsigned gen,epoch,class_epoch;
 unsigned excused;   /* DRI writes that no HS snapshot reads (the snapshot memo uses gen-excused) */
 int pref_scope;     /* inside the HS preference re-ordering writes of test29r1_hs_actionable_order */
 unsigned stage_gen; /* every DRI recruit stage write (the secured-list cache) */
 unsigned offer_epoch;int team_pending; /* the offer-capacity cache */
} cache;
static void cache_bump(void){cache.gen++;}
static void cache_note_write(void){if(cache_on())cache_bump();}
static void cache_note_removal(void){if(cache_on())cache_bump();}
static void cache_note_pt28(void){if(cache_on())cache_bump();}
static void cache_note_class(void){if(cache_on())cache.class_epoch++;}
static void cache_note_stage_write(void){if(cache_on())cache.stage_gen++;}
/* The first statement of every native-to-DRI hook body (kinds in cache_stack.h). A hook that is not CAP_QUIET
   ends the offer-capacity run; a CAP_TEAM hook leaves that decision pending for offer_cap_team, and a pending
   decision that was never resolved ends the run at the next hook entry. A hook that is not GEN_QUIET advances
   the memo generation: a game callback can change anything. */
static void cache_hook_entry(int kind) {
 if(!cache_on())return;
 if(!(kind&CACHE_HOOK_CAP_QUIET)) {
  if(cache.team_pending)cache.offer_epoch++;
  cache.team_pending=(kind&CACHE_HOOK_CAP_TEAM)!=0;
  if(!cache.team_pending)cache.offer_epoch++;
 }
 if(!(kind&CACHE_HOOK_GEN_QUIET))cache_bump();
}
/* The league-scan caches may answer. */
static int cache_scan_ready(void){return cache.active && !cache.paused && !faulted && cache_on();}
/* The memos may answer (they also require the opening thread where noted). */
static int cache_memo_ready(void){return cache.active && !cache.paused && cache_on() && mode==1 && !faulted;}
static void cache_window_open(const char *site) {
 cache.active=1;cache.paused=0;cache.site=site;cache.tid=GetCurrentThreadId();
 cache_bump();cache.epoch++;
}
static void cache_window_close(void){if(!cache.active)return;cache.active=0;cache.paused=0;cache_bump();}
/* Around t36_open inside the post-week window: nothing answers while paused, and the generation moves on both
   edges. The sub-phase boundary right after t36_open also starts a new epoch. */
static void cache_pause(int on){if(!cache_on() || !cache.active)return;cache.paused=on;cache_bump();}
/* Every sub-phase boundary of the two HS windows. */
static void cache_phase_boundary(void){if(cache_on())cache.epoch++;}

/* ---- season memo ----------------------------------------------------------------------------------------- */
/* Inside one hook call the game thread does not advance its calendar and DRI never writes the franchise season
   record, so the year/week/type read at window entry are the values every read in the window would return.
   The memo applies only to the same eval and franchise identity (table, row). */
typedef struct {int active;U eval,franchise_table;uint32_t franchise_row,year,week,type;} CacheSeason;
static CacheSeason cache_season;
/* HS window from values the caller has just read from this eval's franchise and season. 1 = opened. */
static int cache_season_window(U eval,U franchise_table,uint32_t franchise_row,uint32_t year,uint32_t week,uint32_t type,
 const char *site) {
 if(!cache_on() || cache_season.active || cache.active || !eval)return 0;
 CacheSeason m={1,eval,franchise_table,franchise_row,year,week,type};cache_season=m;
 cache_window_open(site);return 1;
}
/* The same, reading the season here. */
static int cache_season_window_read(U eval,const char *site) {
 if(!cache_on() || cache_season.active || cache.active || !eval)return 0;
 Rec f=context_ref(eval,6),season=f?reference(f,44):0;int began=0;
 if(season) {
  uint32_t year=scalar(season,2,0x100),week=scalar(season,4,0x100),type=scalar(season,5,0x108);
  began=cache_season_window(eval,q((U)f+0x10),row(f),year,week,type,site);
 }
 release(season);release(f);return began;
}
static void cache_season_window_end(int began){if(began){memset(&cache_season,0,sizeof(cache_season));cache_window_close();}}
/* The decision part of one native-pass callback (mode 1 only): a window with the season memo set. */
static int cache_callback_window(const char *site,U eval) {
 if(!cache_on() || cache.active || cache_season.active || !eval || mode!=1 || faulted)return 0;
 Rec f=context_ref(eval,6),season=f?reference(f,44):0;
 if(!season){release(season);release(f);return 0;}
 uint32_t year=scalar(season,2,0x100),week=scalar(season,4,0x100),type=scalar(season,5,0x108);
 CacheSeason m={1,eval,q((U)f+0x10),row(f),year,week,type};
 release(season);release(f);
 cache_season=m;cache_window_open(site);return 1;
}
static void cache_callback_window_end(int w){if(!w)return;cache_window_close();memset(&cache_season,0,sizeof(cache_season));}
/* A read-only plan loop or forecast (no season memo). */
static int cache_plan_window(const char *site,U eval){if(!cache_on() || cache.active || !site || !eval)return 0;cache_window_open(site);return 1;}
static void cache_plan_window_end(int w){if(w)cache_window_close();}
