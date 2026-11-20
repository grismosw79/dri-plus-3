/* Snapshot keys are player table/row plus team and season. All SDF records
   retained during selection are released before returning to native code. */
#define T36_MAX_PLAYERS 32768
typedef struct {
    U table;
    unsigned year;
    int team, pay, reason, known, encouraged, source_comp, source_known;
} T36Person;
static int t36_year(Rec p);
#include "test36_eos.inc"
static T36Person t36_person[T36_MAX_PLAYERS];
static U t36_last_f;
static unsigned t36_last_y;
static void t36_season(Rec f, unsigned y) {
    U k = q((U)f + 0x10);
    if (k != t36_last_f || y != t36_last_y) {
        memset(t36_person, 0, sizeof(t36_person));
        t36_last_f = k;
        t36_last_y = y;
    }
}
typedef struct {
    Rec p;
    unsigned id;
    int pos, year, ovr, dev, starts, downs, used, alternate, protect, removed;
} T36Player;
static const int t36_pair[22] = {-1, -1, -1, -1, -1, 9,  8,  -1, 6,  5,  11,
                                 10, -1, 15, -1, 13, -1, -1, -1, -1, -1, -1};
static int t36_year(Rec p) { return 1 + (int)scalar(p, 242, 0x108) + (scalar(p, 234, 0x108) == 3); }
static int t36_config(Rec t, int *ideal, int *starter, int *useful) {
    static const int fields[22] = {5, 2, 1, 9, 8, 4, 3, 0, 6, 7, 3, 6, 1, 4, 5, 7, 0, 2, 8, 1, 6, 4};
    Rec cfg = reference(t, 64);
    if (!cfg)
        return 0;
    int ok = 1;
    for (int p = 0; p < 22; p++) {
        Rec g = p < 19 ? reference(cfg, p < 10 ? 5 : 0) : 0,
            e = (p >= 19 || g) ? reference(p < 19 ? g : cfg, fields[p]) : 0;
        if (!e)
            ok = 0;
        else {
            ideal[p] = (int)scalar(e, 2, 0x100);
            starter[p] = (int)scalar(e, 0, 0x100) + (int)scalar(e, 4, 0x100);
            if (ideal[p] < 0 || ideal[p] > 30 || starter[p] < 0 || starter[p] > 11)
                ok = 0;
            if (ideal[p] && !starter[p])
                starter[p] = 1;
            PortalQuality q0 = {.pos = p, .ideal = ideal[p], .starters = starter[p]};
            useful[p] = dri_portal_useful_depth(&q0);
        }
        release(e);
        release(g);
        if (!ok)
            break;
    }
    release(cfg);
    return ok;
}
static void t36_read_usage(T36Player *v, unsigned tid, unsigned year) {
    Rec stats = array_field(v->p, 243);
    U n = stats ? array_count(stats) : 0;
    const char *types[] = {"SeasonKickingStats",           "SeasonDefensiveKPReturnStats",
                           "SeasonOffensiveKPReturnStats", "SeasonOffensiveStats",
                           "SeasonDefensiveStats",         "SeasonOLineStats"};
    if (n > 128)
        v->protect = 1;
    for (U j = 0; j < n && n <= 128; j++) {
        Rec st = array_at(stats, j);
        if (!st)
            continue;
        U sch = schema(st);
        int type = -1;
        const char *name = sch ? (const char *)q(sch + 0x18) : 0;
        if (name)
            for (int k = 0; k < 6; k++)
                if (!strcmp(name, types[k])) {
                    type = k;
                    break;
                }
        if (type >= 0 && scalar(st, 0, 0x100) == year && scalar(st, 2, 0x100) == tid) {
            int downs = (int)scalar(st, 3, 0x100), starts = (int)scalar(st, 6, 0x100);
            if (downs > v->downs)
                v->downs = downs;
            if (starts > v->starts)
                v->starts = starts;
            if (type == 0)
                v->used |= scalar(st, 12, 0x100) + scalar(st, 24, 0x100) + scalar(st, 26, 0x100) >= 10;
            if (type == 1)
                v->used |= scalar(st, 25, 0x100) + scalar(st, 29, 0x100) >= 10;
            if (type == 2)
                v->used |= scalar(st, 30, 0x100) + scalar(st, 34, 0x100) >= 10;
        } else if (type < 0)
            v->protect = 1;
        release(st);
    }
    release(stats);
}
static int t36_better(T36Player *a, T36Player *b, int future) {
    if (a->ovr != b->ovr)
        return a->ovr > b->ovr;
    if (future && a->dev != b->dev)
        return a->dev > b->dev;
    if (!future && a->starts != b->starts)
        return a->starts > b->starts;
    if (a->downs != b->downs)
        return a->downs > b->downs;
    if (a->dev != b->dev)
        return a->dev > b->dev;
    return a->year != b->year ? a->year < b->year : a->id < b->id;
}
static int t36_courtesy(T36Player *a, T36Player *b) {
    if (a->dev != b->dev)
        return a->dev < b->dev;
    if (a->ovr != b->ovr)
        return a->ovr < b->ovr;
    if (a->starts != b->starts)
        return a->starts < b->starts;
    if (a->downs != b->downs)
        return a->downs < b->downs;
    return a->year != b->year ? a->year > b->year : a->id < b->id;
}
static int t36_leave(U eval, Rec requests, Rec player) {
    U population = array_count(requests), cap = ((U(*)(Rec))(game_base + 0x03803800))(requests);
    if (population >= cap)
        return 0;
    Rec record = 0;
    U arr_table = q((U)requests + 0x10);
    ((Rec * (*)(Rec *, U, U))(game_base + 0x0843eca0))(&record, q(arr_table + 0x10), schema(requests));
    if (!record)
        return 0;
    ((void (*)(Rec, U, Rec *))(game_base + 0x0805e530))(record, field(schema(record), 4), &player);
    /* A CPU roster-management exit is affirmative. Native leave type 11 supplies
       the Playing Time UI text. The trace separately identifies encouragement. */
    portal_recovery_scalar(record, 2, 11);
    portal_recovery_scalar(record, 1, 2);
    ((void (*)(Rec, Rec *))(game_base + 0x08045d30))(requests, &record);
    Rec last = array_count(requests) == population + 1 ? array_at(requests, population) : 0;
    int ok = portal_same_record(last, record);
    release(last);
    if (ok)
        ((void (*)(U, Rec *))(game_base + 0x03811c80))(eval + 0x468, &record);
    release(record);
    return ok;
}
static int t36_mark_leaving(Rec a, int *status, int reason) {
    U n = a ? array_count(a) : 0;
    if (!a || n > 12000)
        return 0;
    for (U j = 0; j < n; j++) {
        Rec e = array_at(a, j), p = e ? reference(e, 4) : 0;
        if (!p || row(p) >= T36_MAX_PLAYERS) {
            release(p);
            release(e);
            return 0;
        }
        unsigned id = row(p);
        int state = (int)scalar(e, 1, 0x108), why = (int)scalar(e, 2, 0x108);
        status[id] = state == 1 ? 2 : 1;
        if (reason && why >= 8 && why <= 16) {
            t36_person[id].source_comp = (int)scalar(p, 139, 0x100);
            t36_person[id].source_known = 1;
            t36_person[id].reason = why;
            t36_person[id].team = (int)scalar(p, 261, 0x100);
            t36_person[id].table = q((U)p + 0x10);
        }
        release(p);
        release(e);
    }
    return 1;
}
static void t36_position_release(U eval, Rec team, Rec requests, const int *status, unsigned year,
                                 unsigned stat_year) {
    unsigned tid = scalar(team, 383, 0x100);
    Rec user = reference(team, 407);
    if (user || tid >= 138) {
        release(user);
        return;
    }
    int target[22], starters[22], rotation[22], count[22] = {0}, young[22] = {0}, max_down[3] = {0};
    if (!t36_config(team, target, starters, rotation))
        return;
    T36Player v[260] = {0};
    int nv = 0, valid = 1;
    for (int source = 0; source < 2 && valid; source++) {
        Rec roster = array_field(team, source ? 33 : 239);
        U n = roster ? array_count(roster) : 0;
        if (!roster || n > 192)
            valid = 0;
        for (U i = 0; valid && i < n; i++) {
            Rec p = array_at(roster, i);
            if (!p || row(p) >= T36_MAX_PLAYERS) {
                release(p);
                valid = 0;
                break;
            }
            unsigned id = row(p);
            int dup = 0;
            for (int j = 0; j < nv; j++)
                if (portal_same_record(p, v[j].p)) {
                    dup = 1;
                    break;
                }
            int pos = (int)scalar(p, 3, 0x108), yr = t36_year(p);
            if (dup || (!source && (status[id] == 1 || yr >= 5))) {
                release(p);
                continue;
            }
            if (nv >= 260 || pos < 0 || pos >= 22 || yr < 1 || yr > 5) {
                release(p);
                valid = 0;
                break;
            }
            T36Player *z = v + nv++;
            z->p = p;
            z->id = id;
            z->pos = pos;
            z->year = yr;
            z->ovr = (int)scalar(p, 191, 0x100) + (source ? 0 : 2);
            if (z->ovr > 99)
                z->ovr = 99;
            z->dev = (int)scalar(p, 271, 0x108);
            z->protect = source || status[id] == 2;
            count[pos]++;
            young[pos] += yr <= 3;
            if (!source) {
                T36Person *hist = t36_person + id;
                int paid = (int)scalar(p, 139, 0x100);
                unsigned injury = scalar(p, 140, 0x100);
                z->protect |= scalar(p, 155, 0x108) != 1 || (injury != 0 && injury != 30);
                int known = hist->known && hist->year == year && hist->team == (int)tid &&
                            hist->table == q((U)p + 0x10);
                if (t36_eos_pay[id].valid && t36_eos_pay[id].tid == (int)tid &&
                    t36_eos_pay[id].table == q((U)p + 0x10) && t36_eos_pay[id].after == paid) {
                    known = 1;
                    hist->pay = t36_eos_pay[id].before;
                }
                z->protect |= paid > 0 && (!known || paid > hist->pay);
                t36_read_usage(z, tid, stat_year);
                int side = pos < 10 ? 0 : pos < 19 ? 1 : 2;
                if (z->downs > max_down[side])
                    max_down[side] = z->downs;
            }
        }
        release(roster);
    }
    Rec dc = reference(team, 63);
    if (!dc)
        valid = 0;
    for (int p = 0; p < 35 && valid; p++) {
        Rec a = array_field(dc, p);
        U n = a ? array_count(a) : 0;
        if (n > 192)
            valid = 0;
        int lim = p == 21 && !starters[p] ? 1 : p < 22 ? starters[p] : 1;
        for (U j = 0; j < n && j < (U)lim; j++) {
            Rec x = array_at(a, j);
            if (x)
                for (int k = 0; k < nv; k++)
                    if (v[k].pos != p && portal_same_record(x, v[k].p))
                        v[k].alternate = 1;
            release(x);
        }
        release(a);
    }
    release(dc);
    for (int p = 0; p < 22 && valid && !faulted; p++)
        while (count[p] > target[p]) {
            int paired = t36_pair[p];
            if (paired >= 0 && count[p] + count[paired] <= target[p] + target[paired])
                break;
            int best = -1;
            for (int j = 0; j < nv; j++) {
                T36Player *z = v + j;
                int max = max_down[p < 10 ? 0 : p < 19 ? 1 : 2];
                if (z->pos != p || z->protect || z->removed || z->alternate || z->used || z->starts >= 3 ||
                    (max && z->downs * 5 >= max))
                    continue;
                if (z->year <= 3 && (young[p] <= target[p] ||
                                     (paired >= 0 && young[p] + young[paired] <= target[p] + target[paired])))
                    continue;
                int now = 1, next = 1;
                for (int k = 0; k < nv; k++)
                    if (k != j && v[k].pos == p && !v[k].removed) {
                        now += t36_better(v + k, z, 0);
                        if (v[k].year <= 3)
                            next += t36_better(v + k, z, 1);
                    }
                if (now <= rotation[p] || (z->year <= 3 && next <= rotation[p]))
                    continue;
                if (best < 0 || t36_courtesy(z, v + best))
                    best = j;
            }
            if (best < 0)
                break;
            T36Player *z = v + best;
            if (!t36_leave(eval, requests, z->p)) {
                log_line("TEST36_PLV_HELD,%u,%u,REQUEST_CAPACITY_OR_PUBLICATION", tid, z->id);
                valid = 0;
                break;
            }
            z->removed = 1;
            t36_person[z->id].encouraged = 1;
            t36_person[z->id].reason = 11;
            t36_person[z->id].team = (int)tid;
            t36_person[z->id].table = q((U)z->p + 0x10);
            log_line("TEST36_PLV_RELEASE,%u,%u,%d,%d,%d,%d,%d,%d,%d", tid, z->id, p, count[p], target[p],
                     young[p], z->year, z->starts, z->downs);
            count[p]--;
            young[p] -= z->year <= 3;
        }
    for (int k = 0; k < nv; k++)
        release(v[k].p);
}
#include "test36r3_plv.inc"
static void t36_plv(U eval, unsigned char arg) {
    ((void (*)(U, unsigned char))(game_base + 0x0842ebe0))(eval, arg);
    if (mode != 1 || faulted || !eval)
        return;
    __try {
        Rec f = context_ref(eval, 6), season = f ? reference(f, 44) : 0;
        unsigned y = season ? scalar(season, 2, 0x100) : 999, type = season ? scalar(season, 5, 0x108) : 99,
                 week = season ? scalar(season, 4, 0x100) : 99;
        unsigned base = season ? scalar(season, 0, 0x100) : 0;
        release(season);
        if (!f || y > 3500 || type != 6 || week > 2) {
            release(f);
            return;
        }
        t36_season(f, y);
        t36_arm(f, y);
        Rec requests = context_ref(eval, 37), draft = context_ref(eval, 10), a = as_array(requests),
            b = as_array(draft), teams = array_field(f, 45);
        int *status = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, T36_MAX_PLAYERS * sizeof(int));
        if (status && t36_mark_leaving(b, status, 0) && t36_mark_leaving(a, status, 1)) {
            U n = teams ? array_count(teams) : 0;
            for (U j = 0; j < n && n <= 138 && !faulted; j++) {
                Rec team = array_at(teams, j);
                if (team)
                    t36_position_release(eval, team, a, status, y, y >= base ? y - base : y);
                release(team);
            }
            /* Overages may have appended requests; refresh before the QB
               supplement so neither pass can remove the same player twice. */
            if(!faulted)
                r3_qb_initial(eval,f,a,status,y,y>=base?y-base:y);
            log_line("TEST36_PLV_COMPLETE,%u,%u,%u,%u,REQUEST_ARRAY", y, type, week,
                     (unsigned)array_count(a));
        }
        if (status)
            HeapFree(GetProcessHeap(), 0, status);
        release(teams);
        release(b);
        release(a);
        release(draft);
        release(requests);
        release(f);
    } __except (fail_exception("T36_PLV", GetExceptionCode())) {
    }
}
