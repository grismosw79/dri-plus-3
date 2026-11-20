/* Opening weekend uses the actual recruiting evaluator and request list. The
   normal game owns the scholarship, financial transaction and hard commit. */
typedef struct {
    U manager, franchise;
    unsigned row, year;
} T36OpeningContext;

static T36OpeningContext t36_hours_wait, t36_eval_scope;
static unsigned t36_eval_wait_mask;
static __declspec(thread) unsigned t36_eval_unavailable;
/* Only the scalar witness survives a callback. Native evaluators are acquired
   from current dynasty records for each use, never held across a reload. */
static __declspec(thread) const char *t36_refresh_phase;
static int t36_refresh_exception(const char *where,EXCEPTION_POINTERS *p){
    EXCEPTION_RECORD *e=p?p->ExceptionRecord:0;U at=e?(U)e->ExceptionAddress:0;
    log_line("T36_REFRESH_EXCEPTION,%s,%s,code=0x%08lx,address=0x%llx,game_rva=0x%llx,access=%llu,target=0x%llx,eval_scope=%d,scope_year=%u",
        where,t36_refresh_phase?t36_refresh_phase:"unspecified",e?e->ExceptionCode:0,
        (unsigned long long)at,(unsigned long long)(at>=game_base&&at-game_base<EXPECTED_IMAGE_SIZE?at-game_base:0),
        (unsigned long long)(e&&e->NumberParameters>0?e->ExceptionInformation[0]:0),
        (unsigned long long)(e&&e->NumberParameters>1?e->ExceptionInformation[1]:0),
        t36_eval_scope.manager!=0,t36_eval_scope.year);
    return fail_exception(where,e?e->ExceptionCode:0);
}

static int t36_same_opening(T36OpeningContext a, T36OpeningContext b) {
    return a.manager && a.manager == b.manager && a.franchise && a.franchise == b.franchise &&
           a.row == b.row && a.year == b.year;
}

static void t36_clear_eval(void) {
    t36_eval_scope = (T36OpeningContext){0};
    t36_eval_wait_mask=0;
}

static void t36_keep_opening_scope(T36OpeningContext scope) {
    if(!t36_same_opening(t36_eval_scope,scope))t36_eval_wait_mask=0;
    t36_eval_scope = scope;
}

static void t36_retry_hours(U manager);
/* Current-title member ordinals and RcEv query tag are verified against the
   captured game and guarded in car35_sites.inc. Saved references independently
   confirm this route. Every reference here belongs to the current callback. */
static Rec t36_current_eval(Rec f, U manager) {
    Rec fe=0, goals=0, story=0, record=0, tmp=0, eval=0, owner=0, result=0;
    int completed=0;
    t36_eval_unavailable=1;
    if (!f || !manager)
        return 0;
    __try {
        t36_refresh_phase="current_evaluator_root";
        t36_eval_unavailable=2;
        fe=reference(f,9);
        if(fe){t36_eval_unavailable=3;goals=reference(fe,9);}
        if(goals){t36_eval_unavailable=4;story=reference(goals,34);}
        if(story){t36_eval_unavailable=5;record=reference(story,34);}
        if (record) {
            t36_eval_unavailable=6;
            t36_refresh_phase="current_evaluator_interface";
            Rec *out=((Rec *(*)(Rec,Rec*,unsigned))vfunc(record,0x28))(record,&tmp,0x52634576);
            if (out) { eval=*out; *out=0; }
        }
        t36_refresh_phase="current_evaluator_layout";
        if (eval) {
            t36_eval_unavailable=7;
            if(q((U)eval+0xc8)==manager){
                t36_eval_unavailable=8;
                if(q((U)eval+0xf8)){
                    t36_eval_unavailable=9;
                    t36_refresh_phase="current_evaluator_owner";
                    owner=context_ref((U)eval,6);
                    if(portal_same_record(f,owner)){result=eval;eval=0;t36_eval_unavailable=0;}
                }
            }
        }
        completed=1;
    } __finally {
        if(completed)t36_refresh_phase="release_current_evaluator_locals";
        release(owner);release(eval);release(tmp);release(record);
        release(story);release(goals);release(fe);
    }
    return result;
}

static void t36_open_current(Rec f,U manager,U leaving,const char *where) {
    if(mode!=1 || faulted || !f || !manager)return;
    Rec eval=0;
    int completed=0;
    __try {
        eval=t36_current_eval(f,manager);
        if (eval) {
            if(t36_eval_wait_mask){
                log_line("TEST36_EVAL_READY,%s,%u",where,t36_eval_scope.year);
                t36_eval_wait_mask=0;
            }
            t36_refresh_phase="current_evaluator_opening";
            t36_open((U)eval,leaving,where);
        } else if(t36_eval_unavailable<32 && !(t36_eval_wait_mask&(1u<<t36_eval_unavailable))){
            t36_eval_wait_mask|=1u<<t36_eval_unavailable;
            log_line("TEST36_EVAL_WAIT,%s,%u,%u",where,t36_eval_scope.year,t36_eval_unavailable);
        }
        completed=1;
    } __finally {
        if(completed)t36_refresh_phase="release_current_evaluator";
        release(eval);
    }
}

/* Resolve the existing Players Leaving interface route from this callback's franchise.
   Never keep its native wrapper, or release a previous dynasty's wrapper,
   across callbacks. The caller owns the returned current reference. */
static Rec t36_current_leaving(Rec f) {
    Rec fe = 0, plv = 0, tmp = 0, iface = 0, owner = 0, result = 0;
    if (!f)
        return 0;
    __try {
        t36_refresh_phase = "current_leaving_root";
        fe = reference(f, 9);
        plv = fe ? reference(fe, 13) : 0;
        if (plv) {
            t36_refresh_phase = "current_leaving_interface";
            Rec *out = ((Rec * (*)(Rec, Rec *, unsigned))vfunc(plv, 0x28))(plv, &tmp, 0x46455052);
            if (out) {
                iface = *out;
                *out = 0;
            }
        }
        if (iface) {
            t36_refresh_phase = "current_leaving_owner";
            owner = context_ref((U)iface, 6);
            if (portal_same_record(f, owner)) {
                result = iface;
                iface = 0;
            }
        }
    } __finally {
        t36_refresh_phase = "release_current_leaving_locals";
        release(owner);
        release(iface);
        release(tmp);
        release(plv);
        release(fe);
    }
    return result;
}
static struct {
    U table;
    unsigned rid, year;
    int valid, employed;
} t36_old_hc[138];
static void t36_capture_hcs(Rec f, unsigned year) {
    Rec a = array_field(f, 45);
    U n = a ? array_count(a) : 0;
    for (U j = 0; j < n && n <= 138; j++) {
        Rec t = array_at(a, j), c = t ? reference(t, 111) : 0;
        if (t && c) {
            unsigned id = scalar(t, 383, 0x100);
            if (id < 138) {
                unsigned st = scalar(c, 12, 0x108);
                t36_old_hc[id].table = q((U)c + 0x10);
                t36_old_hc[id].rid = row(c);
                t36_old_hc[id].year = year;
                t36_old_hc[id].valid = 1;
                t36_old_hc[id].employed = scalar(c,16,0x108)==0 && scalar(c,8,0x100)==id && (st==0 || st==1 || st==4 || (st==6 && scalar(c,13,0x100)>0));
            }
        }
        release(c);
        release(t);
    }
    release(a);
}
static U t36_captured_f;
static unsigned t36_captured_row, t36_captured_year, t36_captured_week;
static void t36_capture_prior(Rec f, Rec season, int before_round_clear) {
    if (!f || !season)
        return;
    unsigned week = scalar(season, 4, 0x100), year = scalar(season, 2, 0x100);
    unsigned type = scalar(season, 5, 0x108);
    /* The calendar can already read EOS at the pre-signing round clear.
       Preserve those outgoing assignments once, before any offer is signed. */
    int eos_origins = before_round_clear && type == 6 && week == 0;
    if (!eos_origins && (type == 6 || week < 16 || week > 20))
        return;
    unsigned origin_week = eos_origins ? 20 : week;
    U ft = q((U)f + 0x10);
    if (ft == t36_captured_f && row(f) == t36_captured_row && year == t36_captured_year &&
        origin_week <= t36_captured_week)
        return;
    Rec teams = array_field(f, 45);
    U n = teams ? array_count(teams) : 0;
    release(teams);
    if (n != 138)
        return;
    memset(t36_old_hc, 0, sizeof(t36_old_hc));
    t36_capture_hcs(f, year);
    t36_captured_f = ft;
    t36_captured_row = row(f);
    t36_captured_year = year;
    t36_captured_week = origin_week;
    log_line("TEST36_HC_ORIGINS,%u,%u,%u,%s", year, week, (unsigned)n,
             eos_origins ? "PRE_SIGNING_ROUND_CLEAR" : "LATE_SEASON");
}
static void t36_pre_contract(void *module, int before_round_clear) {
    __try {
        Rec f = car35_retain_reference(module, 20), season = f ? reference(f, 44) : 0;
        t36_capture_prior(f, season, before_round_clear);
        release(season);
        release(f);
    } __except (fail_exception("T36_HC_origins", GetExceptionCode())) {
    }
}
/* Check the calendar owned by the current manager before inspecting its
   evaluator. Weekly preparation also runs when returning from gameplay. */
static void t36_remember_for_manager(U manager, U eval) {
    if (!manager)
        return;
    t36_refresh_phase="current_manager_franchise";
    Rec f = context_ref(manager, 5);
    t36_refresh_phase="current_calendar";
    Rec season = f ? reference(f, 44) : 0;
    unsigned type = season ? scalar(season, 5, 0x108) : 99;
    unsigned week = season ? scalar(season, 4, 0x100) : 99;
    int opening = type == 6 && (week == 1 || week == 2);
    if (!opening) {
        if (t36_eval_scope.manager)
            t36_clear_eval();
        t36_hours_wait = (T36OpeningContext){0};
        if (type != 6 && week >= 16 && week <= 20)
            t36_capture_prior(f, season, 0);
    } else {
        T36OpeningContext scope = {manager, q((U)f + 0x10), row(f), scalar(season, 2, 0x100)};
        if (t36_eval_scope.manager && !t36_same_opening(t36_eval_scope, scope))
            t36_clear_eval();
        if (t36_hours_wait.manager && !t36_same_opening(t36_hours_wait, scope))
            t36_hours_wait = (T36OpeningContext){0};
        int accepted = 0;
        t36_refresh_phase="current_evaluator_layout";
        if (eval && q(eval + 0xc8) == manager && q(eval + 0xf8)) {
            t36_refresh_phase="current_evaluator_owner";
            Rec owner = context_ref(eval, 6);
            if (portal_same_record(f, owner)) {
                t36_keep_opening_scope(scope);
                accepted = 1;
            }
            t36_refresh_phase="release_current_owner";
            release(owner);
        }
        if (!accepted && t36_eval_scope.manager)
            t36_clear_eval();
    }
    t36_refresh_phase="release_current_calendar";release(season);
    t36_refresh_phase="release_current_franchise";release(f);
}
__attribute__((used)) static U t36_prep_impl(U manager, U eval) {
    if (mode == 1 && !faulted) {
        __try {
            t36_remember_for_manager(manager, eval);
        } __except (t36_refresh_exception("T36_prepare_scope", GetExceptionInformation())) {
        }
    }
    U result = ((U(*)(U))(game_base + 0x08161570))(manager);
    if (mode == 1 && !faulted && manager && t36_hours_wait.manager == manager && eval && t36_eval_scope.manager == manager) {
        __try {
            t36_refresh_phase="opening_hours_retry";
            t36_retry_hours(manager);
        } __except (t36_refresh_exception("T36_opening_retry", GetExceptionInformation())) {
        }
    }
    return result;
}
__attribute__((naked, used)) static void t36_prep_adapter(void) {
    __asm__ volatile("movq %rbx,%rdx\n\tjmp t36_prep_impl");
}
static int t36_instant_roll(U rng, int low, int high) {
    int value = ((int (*)(U, int, int))(game_base + 0x03803b10))(rng, low, high);
    return mode == 1 && !faulted && t36_immediate ? (-2147483647 - 1) : value;
}
typedef struct {
    int team, source, pos, follow, rating, rank, role, score, preference, gain, shortage, source_role, grade,
        required;
    Rec recruit;
    unsigned rid;
} T36Candidate;
typedef struct {
    int tid, pos, rating, role, stars;
} T36Promise;
typedef struct {
    Rec team;
    PortalQuality room[22];
    int ready, prestige, tier, old, followers;
    int8_t pair[22];
} T36School;
typedef struct {
 uint32_t magic,league,year,coach,from,to,term,from_p,to_p,from_t,to_t;
 uint64_t check;
} R2Hire;
static R2Hire r2_hires[138];
static unsigned r2_hire_league;
static uint64_t r2_hire_check(const R2Hire *h) {
 const unsigned char *bytes=(const unsigned char *)h;uint64_t v=UINT64_C(1469598103934665603);
 for(size_t i=0;i<offsetof(R2Hire,check);i++)v=(v^bytes[i])*UINT64_C(1099511628211);
 return v;
}
static int r2_hire_file(R2Hire *h,int save) {
 wchar_t path[1024],temp[1100];DWORD n=GetModuleFileNameW(self_module,path,800);
 if(!n || n>=800)return 0;
 while(n && path[n-1]!=L'\\' && path[n-1]!=L'/')n--;
 if(swprintf(path+n,1024-n,L"HC-Hire-Cache")<0)return 0;
 if(save && !CreateDirectoryW(path,0) && GetLastError()!=ERROR_ALREADY_EXISTS)return 0;
 n+=(DWORD)wcslen(L"HC-Hire-Cache");
 if(swprintf(path+n,1024-n,L"\\%08x-%u-%u-%u-%u-%u.bin",h->league,h->year,h->coach,h->from,h->to,h->term)<0)return 0;
 DWORD bytes=0;HANDLE file;
 if(!save) {
  file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
  if(file==INVALID_HANDLE_VALUE)return 0;
  LARGE_INTEGER size;R2Hire d={0};
  int ok=GetFileSizeEx(file,&size) && size.QuadPart==sizeof(d) && ReadFile(file,&d,sizeof(d),&bytes,0) &&
     bytes==sizeof(d) && d.magic==0x32434844 && d.check==r2_hire_check(&d) &&
     d.league==h->league && d.year==h->year && d.coach==h->coach && d.from==h->from && d.to==h->to && d.term==h->term;
  CloseHandle(file);if(ok)*h=d;return ok;
 }
 if(swprintf(temp,1100,L"%ls.%lu.tmp",path,(unsigned long)GetCurrentProcessId())<0)return 0;
 file=CreateFileW(temp,GENERIC_WRITE,0,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
 if(file==INVALID_HANDLE_VALUE)return 0;
 int ok=WriteFile(file,h,sizeof(*h),&bytes,0) && bytes==sizeof(*h) && FlushFileBuffers(file);
 CloseHandle(file);if(ok)ok=MoveFileExW(temp,path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
 if(!ok)DeleteFileW(temp);return ok;
}
static void r2_hc_hire(void *module,Rec coach,Rec dest,int role,int length) {
 if(role!=0 || !module || !coach || !dest || length<1 || length>10)return;
 Rec f=car35_retain_reference(module,20),s=f?reference(f,44):0;
 if(!s){release(f);return;}
 unsigned from=scalar(coach,8,0x100),to=scalar(dest,383,0x100),year=scalar(s,2,0x100),league=scalar(f,28,0x100);
 if(league && from<138 && to<138 && from!=to && t36_captured_f==q((U)f+0x10) &&
    t36_captured_row==row(f) && t36_old_hc[from].year==year && t36_old_hc[from].valid &&
    t36_old_hc[from].employed && t36_old_hc[from].table==q((U)coach+0x10) && t36_old_hc[from].rid==row(coach) &&
    scalar(coach,16,0x108)==0) {
  Rec teams=array_field(f,45),origin=teams && from<array_count(teams)?array_at(teams,from):0;
  Rec incumbent=origin?reference(origin,111):0;
  if(origin && portal_same_record(incumbent,coach) && scalar(origin,383,0x100)==from) {
   int a=car35_hc_destination_conference(module,origin),b=car35_hc_destination_conference(module,dest);
   unsigned status=scalar(coach,12,0x108);
   if(a>=0 && b>=0 && (status==0 || status==1 || status==4 || status==6)) {
    R2Hire h={.magic=0x32434844,.league=league,.year=year,.coach=row(coach),.from=from,.to=to,.term=(unsigned)length,
       .from_p=scalar(origin,384,0x100),.to_p=scalar(dest,384,0x100),.from_t=(a==0 || a==1 || a==2 || a==9),
       .to_t=(b==0 || b==1 || b==2 || b==9)};
    h.check=r2_hire_check(&h);r2_hires[to]=h;
    int saved=r2_hire_file(&h,1);
    log_line("R2_HC_ACCEPTED,%u,%u,%u,%u,%u,%u,%u,%u,%u,%d",year,h.coach,from,to,status,h.from_t,h.to_t,h.from_p,h.to_p,saved);
   }
  }
  release(incumbent);release(origin);release(teams);
 }
 release(s);release(f);
}
static int r2_hire_matches(const R2Hire *h,unsigned league,unsigned year,unsigned pid,unsigned from,unsigned to,unsigned term) {
 return h->magic==0x32434844 && h->check==r2_hire_check(h) && h->league==league && h->year==year &&
   h->coach==pid && h->from==from && h->to==to && h->term==term;
}
static int t36_promotion(T36School *s, T36School *all, unsigned year) {
    Rec coach = reference(s->team, 111);
    int previous = coach ? (int)scalar(coach, 7, 0x100) : -1, ok = 0;
    unsigned to=scalar(s->team,383,0x100);
    if(coach && to<138 && previous>=0 && previous<138 && (unsigned)previous!=to &&
       all[previous].team && scalar(coach,16,0x108)==0 && scalar(coach,17,0x108)==0 &&
       scalar(coach,8,0x100)==to && r2_hire_league) {
        unsigned term=scalar(coach,10,0x100);
        R2Hire *event=&r2_hires[to];
        if(!r2_hire_matches(event,r2_hire_league,year,row(coach),(unsigned)previous,to,term)){
            R2Hire disk={.league=r2_hire_league,.year=year,.coach=row(coach),.from=(unsigned)previous,.to=to,.term=term};
            if(r2_hire_file(&disk,0))*event=disk;
        }
        if(r2_hire_matches(event,r2_hire_league,year,row(coach),(unsigned)previous,to,term)) {
            ok=event->to_t>event->from_t || (event->to_t==event->from_t && event->to_p>event->from_p);
            log_line("R2_HC_MATCH,%u,%u,%u,%u,%d",year,row(coach),previous,to,ok);
        }
    }
    release(coach);
    return ok ? previous : -1;
}
static void t36_conferences(Rec f, T36School *schools) {
    Rec l = reference(f, 27), a = l ? array_field(l, 3) : 0;
    U n = a ? array_count(a) : 0;
    for (U j = 0; j < n && n <= 32; j++) {
        Rec c = array_at(a, j), teams = c ? array_field(c, 40) : 0;
        U m = teams ? array_count(teams) : 0;
        int conf = c ? (int)scalar(c, 22, 0x108) : -1;
        for (U k = 0; k < m && m <= 256; k++) {
            Rec t = array_at(teams, k);
            if (t) {
                unsigned tid = scalar(t, 383, 0x100);
                if (tid < 138)
                    schools[tid].tier = conf == 0 || conf == 1 || conf == 2 || conf == 9;
            }
            release(t);
        }
        release(teams);
        release(c);
    }
    release(a);
    release(l);
}
static int t36_grade(U leaving, Rec player, Rec team, int motivation) {
    U service = q(leaving + 0xc8);
    if (!service || motivation < 0 || motivation > 13)
        return -1;
    int key = ((int (*)(U, int))(game_base + 0x0840a660))(service, motivation);
    return ((int (*)(U, Rec *, int, Rec *))(game_base + 0x08407000))(service, &team, key, &player);
}
static Rec t36_target(Rec team, Rec r) {
    Rec a = board_array(team), out = 0;
    U n = a ? array_count(a) : 0;
    for (U j = 0; j < n && n <= 50; j++) {
        Rec t = array_at(a, j), p = t ? reference(t, 10) : 0;
        int same = portal_same_record(r, p);
        release(p);
        if (same) {
            out = t;
            break;
        }
        release(t);
    }
    release(a);
    return out;
}
static int t36_rank(const PortalQuality *q, int rating) {
    int k = 0;
    while (k < q->count && q->rating[k] >= rating)
        k++;
    return k + 1;
}
static int t36_role(const PortalQuality *q, int rank) {
    return rank <= q->starters ? 3 : rank <= dri_portal_useful_depth(q) ? 2 : 1;
}
static int t36_mutual(T36Candidate *v, T36School *s, T36School *origin, Rec p, int natural_reason, U leaving,
                      U eval, T36Promise *prom, int np) {
    PortalQuality *q = &s->room[v->pos], *old = &origin->room[v->pos];
    int rating = v->rating;
    if (q->ideal <= 0 || q->count >= 192)
        return 0;
    if (v->pos == 0 && dri_qb_block(q,rating,(int)scalar(p,191,0x100)))
        return 0;
    v->rank = t36_rank(q, rating);
    v->role = t36_role(q, v->rank);
    v->source_role = t36_role(old, t36_rank(old, rating));
    v->shortage = q->ideal - q->count;
    v->gain = v->rank <= q->count ? rating - q->rating[v->rank - 1] : 100;
    if (v->follow) {
        if (v->role < 2 || v->role < v->source_role)
            return 0;
        if (v->pos == 0 && v->rank > 1) {
            if (v->rank != 2 || t36_year(p) > 3)
                return 0;
            Rec roster = array_field(s->team, 239);
            U n = roster ? array_count(roster) : 0;
            int veteran = 0;
            for (U j = 0; j < n && n <= 192; j++) {
                Rec x = array_at(roster, j);
                if (x && scalar(x, 3, 0x108) == 0 && t36_year(x) >= 4 && t36_year(x) < 5 &&
                    portal_projected_ovr((int)scalar(x, 191, 0x100), portal_projection_step(eval)) ==
                        q->rating[0]) {
                    veteran = 1;
                }
                release(x);
            }
            release(roster);
            if (!veteran)
                return 0;
        }
        if (v->shortage <= 0) {
            int i = v->rank - 1;
            if (i >= q->count || q->incoming[i])
                return 0;
            if (v->pos == 0 || v->pos == 19 || v->pos == 20) {
                if (v->rank != 1 || v->gain < 5)
                    return 0;
            } else {
                int floor = dri_quality_floor[dri_group(s->prestige)][v->pos][i];
                if (v->gain < (4 * q->rating[i] < floor ? 3 : v->role == 3 ? 5 : 4))
                    return 0;
            }
            for (int j = 0; j < np; j++)
                if (prom[j].tid == v->team && prom[j].pos == v->pos)
                    return 0;
        } else if (v->role == 2 && v->rank <= q->count &&
                   4 * rating < dri_quality_floor[dri_group(s->prestige)][v->pos][v->rank - 1])
            return 0;
    } else {
        if (v->role != 3 || q->count > q->ideal || s->pair[v->pos])
            return 0;
        v->gain = q->count < q->starters ? 100 : rating - q->rating[q->starters - 1];
        if (v->gain < 5)
            return 0;
    }
    for (int j = 0; j < np; j++)
        if (prom[j].tid == v->team && prom[j].pos == v->pos && rating >= prom[j].rating) {
            int k = 0;
            while (k < q->count && q->rating[k] > prom[j].rating)
                k++;
            if (t36_role(q, k + 2) < prom[j].role)
                return 0;
        }
    int mot = (int)scalar(p, 233, 0x108);
    if (mot < 0 || mot > 13)
        return 0;
    int dst = t36_grade(leaving, p, s->team, mot),
        need = ((int (*)(U, int, Rec *, Rec *))(game_base + 0x084330e0))(leaving, mot, &p, &s->team);
    /* This explicit opening-week check uses the roster Player in the native
       departure evaluator. Its ordinary call deliberately has no transfer context. */
    Rc4Member *member=rc4_member(v->recruit);
    if(member && natural_reason==16 && member->player==row(p) && member->pid==scalar(p,228,0x100) &&
       member->mot==(unsigned)mot && rc4.disk.proof && (rc4.disk.nil[row(v->recruit)>>3]&(1u<<(row(v->recruit)&7))) && need>=0 && need<12)need++;
    v->grade = dst;
    v->required = need;
    if (dst < 0 || dst > 13 || need < 0 || need > 13 || dst > need + v->follow)
        return 0;
    if (natural_reason != 16) {
        static const int by_reason[8] = {2, 4, 5, 9, 10, 12, 8, 7};
        if (natural_reason < 8 || natural_reason > 15)
            return 0;
        int why = by_reason[natural_reason - 8], here = t36_grade(leaving, p, s->team, why),
            there = t36_grade(leaving, p, origin->team, why);
        if (here < 0 || there < 0 || (v->follow ? here > there : here >= there))
            return 0;
    }
    t36_relation = v->follow ? v->recruit : 0;
    t36_relation_team = v->follow ? s->team : 0;
    int legal = native_recruit_eligible(eval, s->team, v->recruit);
    t36_relation = 0;
    t36_relation_team = 0;
    return legal && !faulted;
}
static int t36_source_comp(Rec p) {
    unsigned id = row(p);
    if (id < T36_MAX_PLAYERS && t36_person[id].table == q((U)p + 0x10) && t36_person[id].source_known)
        return t36_person[id].source_comp;
    return (int)scalar(p, 139, 0x100);
}
static int t36_opening_price(int native_ask, int offered, int from, int reason) {
    int floor = reason == 16 ? (from / 5 + 1) * 5 : from;
    int price = native_ask > offered ? native_ask : offered;
    return price > floor ? price : floor;
}
#include "test36_instant.inc"
static int t36_purchase(T36Candidate *v, T36School *s, U eval, int reason) {
    Rec r = v->recruit, p = reference(r, 5);
    if (!p)
        return 0;
    t36_relation = v->follow ? r : 0;
    t36_relation_team = v->follow ? s->team : 0;
    t36_designated = r;
    t36_destination = s->team;
    int ok = 0;
    Rec target = 0;
    U manager = q(eval + 0xc8);
    if (r3_qb_admission(r,s->team,eval,"OPENING_PURCHASE"))
        goto done;
    if (!manager || scalar(r, 9, 0x108) >= 5 || !native_recruit_eligible(eval, s->team, r))
        goto done;
    int cap = test32_hs_offer_capacity(r, s->team, eval);
    if (cap != 1)
        goto done;
    target = t36_target(s->team, r);
    if (!target) {
        Rec a = board_array(s->team);
        U n = a ? array_count(a) : 51;
        release(a);
        if (n >= 50)
            goto done;
        U cache[67];
        int score = 0;
        if (!market29_portal_school_cache(s->team, eval, cache) ||
            !market29_portal_score_cached(r, s->team, eval, cache, &score))
            goto done;
        /* The player's move to his employed HC creates an independent destination
           even if the old top-school filter gives no opening interest there. */
        if (score <= 0 && !v->follow)
            goto done;
        PortalNativeCandidate c = {.rec = (U)r};
        c.values[8] = score > 0 ? score : 1;
        if (((int (*)(U, Rec *, void *))(game_base + 0x081496c0))(manager, &s->team, &c))
            goto done;
        target = t36_target(s->team, r);
        if (!target)
            goto done;
    }
    int ask = (int)scalar(target, 4, 0x100), paid = (int)scalar(target, 2, 0x100), prior = t36_source_comp(p);
    if (ask < 0 || ask > 1023)
        goto done;
    int price = t36_opening_price(ask, paid, prior, reason);
    if (price > 1023 || t36_transfer_budget(eval, s->team) < price - paid)
        goto done;
    Rec board = reference(s->team, 224);
    int assigned = board ? (int)scalar(board, 1, 0x100) : 0, total = board ? (int)scalar(board, 3, 0x100) : 0;
    int existing = scalar(target, 12, 0x108) >= 2, cost = hs28_recovery_cost(eval, existing);
    release(board);
    if (cost <= 0 || total - assigned < cost)
        goto done;
    test31_offer_register(eval);
    /* Match the native CPU caller at 814df6f. No per-simulation recruit-class
       counter is supplied (the same optional argument accepted by the UI). */
    int rc = existing ? !t36_bid_debit(eval, s->team, target, price, 1)
                      : ((int (*)(U, Rec *, Rec *, int, void *, int, int))(game_base + 0x0819ee10))(
                            manager, &s->team, &target, price, 0, i32(eval + 0x1a8), i32(eval + 0x1a4));
    if (rc || (int)scalar(target, 2, 0x100) < price || scalar(target, 12, 0x108) < 2)
        goto done;
    if (scalar(r, 9, 0x108) < 5) {
        t36_accept_opening(manager, r, s->team, target, price);
    }
    if (!faulted && scalar(r, 9, 0x108) == 5) {
        int owner = -1;
        ok = portal_hard_owner(r, eval, &owner) > 0 && owner == v->team;
        if (ok) {
            test29_class_invalidate(s->team);
            portal_cleanup_rivals(r, (unsigned)owner, eval);
        }
    }
    log_line("TEST36_OPENING_RESULT,%s,%u,%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", v->follow ? "HC" : "FIVE_STAR",
             row(p), row(r), v->source, v->team, v->pos, v->rank, v->preference, v->score, ask, price,
             v->grade, ok);
done:
    release(target);
    release(p);
    t36_relation = 0;
    t36_relation_team = 0;
    t36_designated = 0;
    t36_destination = 0;
    t36_immediate = 0;
    return ok;
}
static int t36_in_leading_group(int place, int score, int leader) {
    return place >= 1 && place <= 3 && score >= 55 && leader - score <= 5;
}
static int t36_decisive(int best, int runner) { return runner < 0 || best - runner >= 2; }
static int t36_before_opening(const T36Candidate *a, const T36Candidate *b) {
    if (a->follow) {
        if (a->role != b->role)
            return a->role > b->role;
        int an = a->shortage < 0   ? 0
                 : a->shortage > 3 ? 3
                                   : a->shortage,
            bn = b->shortage < 0   ? 0
                 : b->shortage > 3 ? 3
                                   : b->shortage;
        if (an != bn)
            return an > bn;
        if (a->role - a->source_role != b->role - b->source_role)
            return a->role - a->source_role > b->role - b->source_role;
    } else {
        if (a->score != b->score)
            return a->score > b->score;
        if (a->preference != b->preference)
            return a->preference < b->preference;
        if (a->gain != b->gain)
            return a->gain > b->gain;
    }
    return a->rating != b->rating ? a->rating > b->rating
           : a->rid != b->rid     ? a->rid < b->rid
                                  : a->team < b->team;
}
static int t36_check_quote(T36Candidate *v, T36School *s, U eval, int reason) {
    Rec target = t36_target(s->team, v->recruit);
    int ask, paid = 0, offered = 0;
    if (target) {
        ask = (int)scalar(target, 4, 0x100);
        paid = (int)scalar(target, 2, 0x100);
        offered = scalar(target, 12, 0x108) >= 2;
    } else {
        Rec board = board_array(s->team);
        U n = board ? array_count(board) : 51;
        release(board);
        if (n >= 50)
            return 0;
        U manager = q(eval + 0xc8), finance = manager ? q(manager + 0x1a0) : 0;
        if (!finance)
            return 0;
        /* Exact read-only expectation used to initialize a native RecruitTarget.
           Comparing the top three must never put three new records on boards. */
        ask = ((int (*)(U, Rec *, Rec *))(game_base + 0x082c0c40))(finance, &v->recruit, &s->team);
    }
    Rec p = reference(v->recruit, 5);
    if (!p) {
        release(target);
        return 0;
    }
    int was = t36_source_comp(p);
    release(target);
    release(p);
    if (ask < 0 || ask > 1023)
        return 0;
    ask = t36_opening_price(ask, paid, was, reason);
    Rec b = reference(s->team, 224);
    int hours = b ? (int)scalar(b, 3, 0x100) - (int)scalar(b, 1, 0x100) : -1;
    release(b);
    int cost = hs28_recovery_cost(eval, offered);
    return ask <= 1023 && cost > 0 && hours >= cost && ask - paid <= t36_transfer_budget(eval, s->team);
}
static struct {
    U franchise;
    unsigned row, year;
    int active, complete;
} t36_open_scope;
static struct {
    U franchise;
    unsigned row, year;
    int armed;
} t36_transition;
static void t36_arm(Rec f, unsigned year) {
    t36_transition.franchise = q((U)f + 0x10);
    t36_transition.row = row(f);
    t36_transition.year = year;
    t36_transition.armed = 1;
}
static void t36_open(U eval, U leaving, const char *where) {
    if (mode != 1 || faulted || !eval || t36_open_scope.active)
        return;
    Rec f = context_ref(eval, 6), season = f ? reference(f, 44) : 0;
    unsigned week = season ? scalar(season, 4, 0x100) : 99, type = season ? scalar(season, 5, 0x108) : 99,
             y = season ? scalar(season, 2, 0x100) : 0;
    release(season);
    int seed_event = !strcmp(where, "INITIAL_INTEREST") || !strcmp(where, "ADD_TRANSFERS");
    int publication = seed_event || !strcmp(where, "HOURS_PUBLISHED");
    if (!f || type != 6 || (week != 2 && !(week == 1 && publication))) {
        release(f);
        return;
    }
    U ft = q((U)f + 0x10);
    if (t36_open_scope.complete && t36_open_scope.franchise == ft && t36_open_scope.row == row(f) &&
        t36_open_scope.year == y) {
        release(f);
        return;
    }
    Rec teams = array_field(f, 45), list = array_field(f, 46), iface = 0;
    if (!leaving)
        leaving = (U)(iface = t36_current_leaving(f));
    U entries = list ? array_count(list) : 0, n = 0, nt = teams ? array_count(teams) : 0;
    T36School *s = 0;
    T36Candidate *options = 0;
    T36Promise *prom = 0;
    Rec *held = 0;
    unsigned *why = 0;
    int *skip = 0, np = 0, done = 0, full = 0;
    if (!entries || entries > 12000 || nt != 138 || !leaving)
        goto end;
    /* Some callbacks precede seed publication. Revisit the next safe opening
       callback instead of consuming the year before preferences exist. */
    unsigned seeded = 0, owned = 0;
    for (U i = 0; i < entries; i++) {
        Rec rec = array_at(list, i);
        if (!rec)
            goto end;
        if (transfer_recruit(rec)) {
            Rec top = array_field(rec, 13);
            n++;
            seeded += top && array_count(top) > 0;
            owned += scalar(rec, 9, 0x108) >= 5;
            release(top);
        }
        release(rec);
    }
    if (n && seed_event && !owned)
        t36_arm(f, y);
    if (!n || seeded != n || !t36_transition.armed || t36_transition.franchise != ft ||
        t36_transition.row != row(f) || t36_transition.year != y)
        goto end;
    int have_hours = 0;
    for (U j = 0; j < nt && !have_hours; j++) {
        Rec team = array_at(teams, j), board = team ? reference(team, 224) : 0;
        if (board)
            have_hours = scalar(board, 3, 0x100) > scalar(board, 1, 0x100);
        release(board);
        release(team);
    }
    if (!have_hours) {
        if (publication)
            log_line("TEST36_TP1_WAIT,%s,%u,%u,%u,NATIVE_HOURS", where, y, week, (unsigned)n);
        goto end;
    }
    t36_open_scope.active = 1;
    t36_active_eval = eval;
    test31_offer_register(eval);
    /* The same native week can contain a pre-market pass with zero hours.
       Rebuild the opening nominations using the newly published budgets. */
    market29_portal.ready = 0;
    market29_portal_begin(eval);
    if (faulted || !market29_portal.ready)
        goto end;
    s = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 138 * sizeof(*s));
    options = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (n * 3 + 1) * sizeof(*options));
    prom = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (n + 1) * sizeof(*prom));
    held = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, n * sizeof(*held));
    why = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, n * sizeof(*why));
    skip = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, n * sizeof(*skip));
    if (!s || !options || !prom || !held || !why || !skip)
        goto end;
    for (int i = 0; i < 138; i++) {
        s[i].old = -1;
        s[i].tier = -1;
        Rec t = array_at(teams, (U)i);
        if (!t)
            goto end;
        unsigned id = scalar(t, 383, 0x100);
        if (id >= 138 || s[id].team) {
            release(t);
            goto end;
        }
        s[id].team = t;
        s[id].prestige = (int)scalar(t, 384, 0x100);
    }
    r2_hire_league=scalar(f,28,0x100);
    t36_conferences(f, s);
    for (int t = 0; t < 138; t++) {
        if (!s[t].team)
            goto end;
        s[t].old = t36_promotion(s + t, s, y);
    }
    U i = 0;
    for (U j = 0; j < entries; j++) {
        Rec rec = array_at(list, j);
        if (!rec)
            goto end;
        if (!transfer_recruit(rec)) {
            release(rec);
            continue;
        }
        if (i >= n) {
            release(rec);
            goto end;
        }
        held[i] = rec;
        Rec p = reference(held[i], 5);
        if (p && row(p) < T36_MAX_PLAYERS && t36_person[row(p)].table == q((U)p + 0x10))
            why[i] = (unsigned)t36_person[row(p)].reason;
        release(p);
        i++;
    }
    if (i != n)
        goto end;
    Rec raw = context_ref(leaving, 37), requests = as_array(raw);
    U nr = requests ? array_count(requests) : 0;
    /* Reconstruct the departure reason after a checkpoint reload. */
    if (nr && nr <= 6000) {
        for (U j = 0; j < nr; j++) {
            Rec rr = array_at(requests, j), p = rr ? reference(rr, 4) : 0;
            if (p && row(p) < T36_MAX_PLAYERS) {
                t36_person[row(p)].reason = (int)scalar(rr, 2, 0x108);
                t36_person[row(p)].table = q((U)p + 0x10);
            }
            release(p);
            release(rr);
        }
        for (U i = 0; i < n; i++) {
            Rec p = reference(held[i], 5);
            if (p && row(p) < T36_MAX_PLAYERS && t36_person[row(p)].table == q((U)p + 0x10))
                why[i] = (unsigned)t36_person[row(p)].reason;
            release(p);
        }
    }
    release(requests);
    release(raw);
    for (int phase = 0; phase < 2 && !faulted; phase++) {
        int following = !phase, accepted = 0;
        memset(skip, 0, n * sizeof(*skip));
        for (;;) {
            int no = 0;
            for (U i = 0; i < n; i++) {
                Rec r = held[i];
                if (skip[i] || why[i] < 8 || why[i] > 16 || scalar(r, 9, 0x108) >= 5)
                    continue;
                Rec p = reference(r, 5);
                if (!p)
                    continue;
                int src = (int)scalar(p, 230, 0x100), pos = (int)scalar(p, 3, 0x108),
                    star = (int)scalar(p, 231, 0x108);
                if (src < 0 || src >= 138 || pos < 0 || pos >= 22 || (!following && star != 4)) {
                    release(p);
                    continue;
                }
                int targets[138], scores[138], ntg = 0, leader = 0;
                if (following) {
                    for (int t = 0; t < 138; t++)
                        if (s[t].old == src && s[t].followers < 8) {
                            targets[ntg] = t;
                            scores[ntg++] = 0;
                        }
                } else {
                    Rec top = array_field(r, 13);
                    U size = top ? array_count(top) : 0;
                    for (U j = 0; j < size && j < 3; j++) {
                        Rec q0 = array_at(top, j);
                        if (!q0)
                            break;
                        int t = (int)scalar(q0, 0, 0x100), score = (int)scalar(q0, 1, 0x100);
                        release(q0);
                        if (!j) {
                            leader = score;
                            if (t == src)
                                break;
                        }
                        if (t < 0 || t >= 138 || !t36_in_leading_group((int)j + 1, score, leader))
                            continue;
                        targets[ntg] = t;
                        scores[ntg++] = score;
                    }
                    release(top);
                }
                int first = no;
                for (int j = 0; j < ntg; j++) {
                    int t = targets[j];
                    if (t == src || portal_academy((unsigned)t))
                        continue;
                    Rec user = reference(s[t].team, 407);
                    if (user) {
                        release(user);
                        continue;
                    }
                    if (!s[t].ready) {
                        s[t].ready = portal_quality_rooms(s[t].room, 0, &s[t].prestige, s[t].team, eval);
                        if (!s[t].ready)
                            continue;
                    }
                    if (!s[src].ready) {
                        s[src].ready =
                            portal_quality_rooms(s[src].room, 0, &s[src].prestige, s[src].team, eval);
                        if (!s[src].ready)
                            continue;
                    }
                    Market29PortalRecruit *m = market29_portal_find(q((U)r + 0x10), row(r));
                    if (!following && (!m || !market29_has(m->pressure.primary, (unsigned)t) ||
                                       market29_portal_admission_block(r, s[t].team, eval)))
                        continue;
                    T36Candidate v = {.team = t,
                                      .source = src,
                                      .pos = pos,
                                      .follow = following,
                                      .rating = portal_projected_ovr((int)scalar(p, 191, 0x100),
                                                                     portal_projection_step(eval)),
                                      .score = scores[j],
                                      .preference = j + 1,
                                      .rid = (unsigned)i,
                                      .recruit = r};
                    if (t36_mutual(&v, s + t, s + src, p, (int)why[i], leaving, eval, prom, np) &&
                        t36_check_quote(&v, s + t, eval, (int)why[i]))
                        options[no++] = v;
                }
                if (!following && no > first + 1) {
                    int best = first, runner = -1;
                    for (int j = first + 1; j < no; j++) {
                        if (options[j].score > options[best].score) {
                            runner = best;
                            best = j;
                        } else if (runner < 0 || options[j].score > options[runner].score)
                            runner = j;
                    }
                    if (!t36_decisive(options[best].score, runner >= 0 ? options[runner].score : -1))
                        no = first;
                    else {
                        options[first] = options[best];
                        no = first + 1;
                    }
                }
                release(p);
            }
            if (!no || faulted)
                break;
            int k = 0;
            for (int j = 1; j < no; j++)
                if (t36_before_opening(options + j, options + k))
                    k = j;
            T36Candidate *v = options + k;
            int t = v->team;
            skip[v->rid] = 1;
            if (t36_purchase(v, s + t, eval, (int)why[v->rid])) {
                prom[np++] = (T36Promise){t, v->pos, v->rating, v->role, !following};
                s[t].ready = 0;
                if (following)
                    s[t].followers++;
                else
                    s[t].pair[v->pos] = 1;
                accepted++;
            }
        }
        log_line("TEST36_TP1_PHASE,%s,%s,%d,%d", where, following ? "HC" : "FIVE_STAR", accepted, np);
    }
    done = !faulted;
    full = (int)n;
end:
    if (held) {
        for (U i = 0; i < n; i++)
            release(held[i]);
        HeapFree(GetProcessHeap(), 0, held);
    }
    if (s) {
        for (int j = 0; j < 138; j++)
            release(s[j].team);
        HeapFree(GetProcessHeap(), 0, s);
    }
    if (prom)
        HeapFree(GetProcessHeap(), 0, prom);
    if (why)
        HeapFree(GetProcessHeap(), 0, why);
    if (skip)
        HeapFree(GetProcessHeap(), 0, skip);
    if (options)
        HeapFree(GetProcessHeap(), 0, options);
    if (done) {
        t36_open_scope.complete = 1;
        t36_open_scope.franchise = ft;
        t36_open_scope.row = row(f);
        t36_open_scope.year = y;
        market29_portal.ready = 0;
        log_line("TEST36_TP1_COMPLETE,%s,%u,%u,%d,%d", where, y, week, full, np);
    }
    t36_open_scope.active = 0;
    t36_active_eval = 0;
    release(iface);
    release(teams);
    release(list);
    release(f);
}
static void t36_initial_interest(U eval) {
    ((void (*)(U))(game_base + 0x081a2670))(eval);
    if (mode != 1 || faulted)
        return;
    __try {
        if (eval)
            t36_remember_for_manager(q(eval + 0xc8), eval);
        t36_open(eval, 0, "INITIAL_INTEREST");
    } __except (fail_exception("T36_TP1_seed", GetExceptionCode())) {
    }
}
static void t36_transfers(U leaving) {
    unsigned char reasons[RC4_ROWS]={0};int captured=0;
    if (mode == 1 && !faulted)
        __try {
            r3_qb_pre_pool(leaving);
            Rec records = context_ref(leaving, 37), a = as_array(records), f = context_ref(leaving, 6),
                season = f ? reference(f, 44) : 0;
            U n = a ? array_count(a) : 0;
            if (season && n && n <= 12000) {
                unsigned y = scalar(season, 2, 0x100);
                t36_season(f, y);
                t36_arm(f, y);
                captured=1;rc4_publishing.f=(U)f;rc4_publishing.year=y;
                for (U i = 0; i < n; i++) {
                    Rec x = array_at(a, i), p = x ? reference(x, 4) : 0;
                    int why = x ? (int)scalar(x, 2, 0x108) : 0;
                    if (p && row(p)<RC4_ROWS && why>=8 && why<=16 && scalar(x,1,0x108)!=1){
                        unsigned id=row(p);reasons[id]=!reasons[id] || reasons[id]==why?(unsigned char)why:255;
                    }
                    if (p && row(p) < T36_MAX_PLAYERS && why >= 8 && why <= 16) {
                        t36_person[row(p)].source_comp = (int)scalar(p, 139, 0x100);
                        t36_person[row(p)].source_known = 1;
                        t36_person[row(p)].reason = why;
                        t36_person[row(p)].table = q((U)p + 0x10);
                    }
                    release(p);
                    release(x);
                }
            }
            release(season);
            release(f);
            release(a);
            release(records);
        } __except (fail_exception("T36_capture_departures", GetExceptionCode())) {
        }
    rc4_publishing.reason=captured?reasons:0;
    __try {((void (*)(U))(game_base + 0x08429160))(leaving);}
    __finally {rc4_publishing.reason=0;rc4_publishing.f=0;}
    if(mode!=1 || faulted)return;
    __try {
        Rec f=context_ref(leaving,6),s=f?reference(f,44):0,list=f?array_field(f,46):0;
        rc4_bind(f,s,list,captured?reasons:0);release(list);release(s);release(f);
    } __except(fail_exception("RC4_actual_portal",GetExceptionCode())){}
    if(!t36_eval_scope.manager)return;
    __try {
        Rec f = context_ref(leaving, 6);
        Rec season = f ? reference(f, 44) : 0;
        unsigned type = season ? scalar(season, 5, 0x108) : 99;
        unsigned week = season ? scalar(season, 4, 0x100) : 99;
        int same = type == 6 && (week == 1 || week == 2) && t36_eval_scope.franchise == q((U)f + 0x10) &&
                   t36_eval_scope.row == row(f) && t36_eval_scope.year == scalar(season, 2, 0x100);
        __try {
            if (same)
                t36_open_current(f,t36_eval_scope.manager,leaving,"ADD_TRANSFERS");
        } __finally { release(season);release(f); }
    } __except (t36_refresh_exception("T36_TP1_pool", GetExceptionInformation())) {
    }
}

/* The native callback at 08170f36 receives DYRM, the recruiting manager.
   Field 5 is Dynasty. Field 6 is FirstRecruitingStageDetails and must never
   be read as a franchise. The prep dispatcher passes RcEv in RBX and its
   own manager ([RBX+0xc8]) in RCX at 0816ff29. */
static void t36_retry_hours(U manager) {
    if (!manager || t36_hours_wait.manager != manager || !t36_transition.armed ||
        t36_transition.franchise != t36_hours_wait.franchise || t36_transition.row != t36_hours_wait.row ||
        t36_transition.year != t36_hours_wait.year || !t36_eval_scope.manager ||
        !t36_same_opening(t36_eval_scope, t36_hours_wait))
        return;

    Rec f=0,season=0;
    __try {
        f=context_ref(manager,5);
        season=f?reference(f,44):0;
        unsigned type=season?scalar(season,5,0x108):99;
        unsigned week=season?scalar(season,4,0x100):99;
        T36OpeningContext now={manager,f?q((U)f+0x10):0,f?row(f):0,season?scalar(season,2,0x100):0};
        if(type==6 && (week==1 || week==2) && t36_same_opening(now,t36_hours_wait))
            t36_open_current(f,manager,0,"HOURS_PUBLISHED");
    } __finally { release(season);release(f); }
    if (t36_open_scope.complete && t36_open_scope.franchise == t36_hours_wait.franchise &&
        t36_open_scope.row == t36_hours_wait.row && t36_open_scope.year == t36_hours_wait.year)
        t36_hours_wait = (T36OpeningContext){0};
}

static void t36_hours_published(U manager, int event) {
    ((void (*)(U, int))(game_base + 0x081ab310))(manager, event);
    if (mode != 1 || faulted || !manager)
        return;
    __try {
        Rec f = context_ref(manager, 5), season = f ? reference(f, 44) : 0;
        unsigned type = season ? scalar(season, 5, 0x108) : 99;
        unsigned week = season ? scalar(season, 4, 0x100) : 99;
        unsigned year = season ? scalar(season, 2, 0x100) : 0;
        t36_hours_wait = (T36OpeningContext){0};
        if (season && type != 6)
            t36_capture_prior(f, season, 0);
        if (season && type == 6 && (week == 1 || week == 2)) {
            t36_hours_wait.manager = manager;
            t36_hours_wait.franchise = q((U)f + 0x10);
            t36_hours_wait.row = row(f);
            t36_hours_wait.year = year;
        }
        if (t36_eval_scope.manager && !t36_same_opening(t36_eval_scope, t36_hours_wait))
            t36_clear_eval();
        release(season);
        release(f);
        log_line("TEST36R1_HOURS,%u,%u,%u,%d,%s", year, type, week, event,
                 t36_hours_wait.manager ? "OPENING_WINDOW" : "NATIVE_ONLY");
        if (t36_hours_wait.manager)
            t36_retry_hours(manager);
    } __except (t36_refresh_exception("T36R1_hours_manager", GetExceptionInformation())) {
    }
}
