static struct {
    U table;
    int tid, before, after, valid;
} t36_eos_pay[T36_MAX_PLAYERS];
static void t36_pay_by_team(Rec team, int after) {
    unsigned tid = scalar(team, 383, 0x100);
    Rec a = array_field(team, 239);
    U n = a ? array_count(a) : 0;
    for (U j = 0; j < n && n <= 192; j++) {
        Rec p = array_at(a, j);
        if (!p)
            continue;
        unsigned id = row(p);
        if (id < T36_MAX_PLAYERS) {
            int pay = (int)scalar(p, 139, 0x100);
            if (!after) {
                t36_eos_pay[id].valid = 1;
                t36_eos_pay[id].table = q((U)p + 0x10);
                t36_eos_pay[id].tid = (int)tid;
                t36_eos_pay[id].before = pay;
                t36_eos_pay[id].after = pay;
            } else if (t36_eos_pay[id].valid && t36_eos_pay[id].table == q((U)p + 0x10)) {
                t36_eos_pay[id].after = pay;
                if (pay != t36_eos_pay[id].before)
                    log_line("TEST36_EOS_PAY,%u,%u,%u,%d,%d,%d", tid, id, scalar(p, 3, 0x108),
                             t36_eos_pay[id].before, pay, (int)scalar(p, 126, 0x100));
            }
        }
        release(p);
    }
    release(a);
}
static U t36_retention_pay(U module, Rec *team) {
    int active = mode == 1 && !faulted && team && *team;
    int before = 0;
    if (active)
        __try {
            before = (int)scalar(*team, 170, 0x100);
            t36_pay_by_team(*team, 0);
        } __except (fail_exception("T36_EOS_before", GetExceptionCode())) {
            active = 0;
        }
    U result = ((U(*)(U, Rec *))(game_base + 0x082b7a50))(module, team);
    if (active)
        __try {
            t36_pay_by_team(*team, 1);
            log_line("TEST36_EOS_TEAM,%u,%d,%d", scalar(*team, 383, 0x100), before,
                     (int)scalar(*team, 170, 0x100));
        } __except (fail_exception("T36_EOS_after", GetExceptionCode())) {
        }
    return result;
}
static unsigned char t36_attempted[T36_MAX_PLAYERS];
static int t36_nfl_chance(U pl, Rec *player, unsigned a, unsigned char b) {
    int result = ((int (*)(U, Rec *, unsigned, unsigned char))(game_base + 0x08433b30))(pl, player, a, b);
    if (mode == 1 && !faulted && player && *player)
        __try {
            unsigned id = row(*player);
            if (id < T36_MAX_PLAYERS)
                t36_attempted[id] = 1;
            log_line("TEST36_NFL_ATTEMPT,%u,%u,%u,%u,%d,%d", id, scalar(*player, 261, 0x100),
                     scalar(*player, 3, 0x108), scalar(*player, 191, 0x100), t36_year(*player), result);
        } __except (fail_exception("T36_NFL_attempt", GetExceptionCode())) {
        }
    return result;
}
static void t36_nfl(U pl) {
    memset(t36_attempted, 0, sizeof(t36_attempted));
    ((void (*)(U))(game_base + 0x0842e770))(pl);
    if (mode != 1 || faulted)
        return;
    __try {
        Rec list = context_ref(pl, 7), c = as_array(list), early = context_ref(pl, 10), a = as_array(early);
        U pop = a ? array_count(a) : 0, cap = a ? ((U(*)(Rec))(game_base + 0x03803800))(a) : 0,
          n = c ? array_count(c) : 0;
        unsigned char admitted[T36_MAX_PLAYERS] = {0};
        for (U j = 0; j < pop && pop <= 2048; j++) {
            Rec r = array_at(a, j), p = r ? reference(r, 4) : 0;
            if (p && row(p) < T36_MAX_PLAYERS)
                admitted[row(p)] = 1;
            release(p);
            release(r);
        }
        for (U j = 0; j < n && n <= 4096; j++) {
            Rec r = array_at(c, j), p = r ? reference(r, 4) : 0;
            if (p && row(p) < T36_MAX_PLAYERS && t36_year(p) < 5) {
                unsigned id = row(p);
                log_line("TEST36_NFL_OUTCOME,%u,%u,%u,%d,%u,%u,%u,%u,%s", id, scalar(p, 261, 0x100),
                         scalar(p, 3, 0x108), t36_year(p), scalar(r, 5, 0x100), t36_attempted[id],
                         (unsigned)pop, (unsigned)cap,
                         admitted[id]        ? "ADMITTED"
                         : t36_attempted[id] ? "EVALUATED_DID_NOT_ENTER"
                         : pop == cap && cap ? "UNROLLED_AT_FULL_CAP"
                                             : "UNROLLED");
            }
            release(p);
            release(r);
        }
        release(a);
        release(early);
        release(c);
        release(list);
    } __except (fail_exception("T36_NFL_outcome", GetExceptionCode())) {
    }
}
/* Current native grade and effective required grade precede the NIL code-16
   choice at 842f326. Preserve the return and record the actual comparison. */
static int t36_leave_grade(U pl, int motivation, Rec *player, Rec *team) {
    int needed = ((int (*)(U, int, Rec *, Rec *))(game_base + 0x084330e0))(pl, motivation, player, team);
    if (mode == 1 && !faulted && player && *player && team && *team)
        __try {
            U grades = q(pl + 0xc8);
            int key = ((int (*)(U, int))(game_base + 0x0840a660))(grades, motivation);
            int actual = ((int (*)(U, Rec *, int, Rec *))(game_base + 0x08407000))(grades, team, key, player);
            log_line("TEST36_LEAVE_GATE,%u,%u,%d,%d,%d,%u,%d,%s", row(*player), scalar(*team, 383, 0x100),
                     motivation, actual, needed, scalar(*player, 139, 0x100),
                     (int)scalar(*player, 126, 0x100), actual <= needed ? "NIL_BRANCH_16" : "GRADED_BRANCH");
        } __except (fail_exception("T36_leave_gate", GetExceptionCode())) {
        }
    return needed;
}

static int t36_effective_nil(U finance, Rec *player, Rec *team) {
    int ask = ((int (*)(U, Rec *, Rec *))(game_base + 0x082c0dd0))(finance, player, team);
    if (mode == 1 && !faulted && player && *player && team && *team)
        __try {
            int paid = (int)scalar(*player, 139, 0x100), base = (int)scalar(*player, 126, 0x100);
            log_line("TEST36_EFFECTIVE_NIL,%u,%u,%u,%d,%d,%d,%d", row(*player), scalar(*team, 383, 0x100),
                     scalar(*player, 3, 0x108), paid, base, ask, ask - paid);
        } __except (fail_exception("T36_effective_NIL", GetExceptionCode())) {
        }
    return ask;
}
