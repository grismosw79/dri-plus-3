/* The two accepted images publish CurrentNILOffer at ordinal 2, NILExpectation
   at 4, and Team recruit spend/remaining at 225/227. These are game member
   ordinals, not alphabetical offsets in the exported schema. */
typedef struct {
    U table;
    unsigned rid;
    int pos, slot, firm, conditional, bid;
} T36Paid;
typedef struct {
    U franchise, team;
    unsigned year, week;
    int n;
    T36Paid a[MAX_BOARD];
} T36Week;
static T36Week t36_recent[138], t36_previous[138];
static int t36_nil_price(int paid, int current, int ask, int other, int trailing, int available) {
    if (paid < 0 || current < 0 || ask < 0 || available < 0 || paid > 1023 || current > 1023 || ask > 1023)
        return current;
    int price = current > paid ? current : paid;
    if (trailing && ask > price)
        price = ask;
    if (trailing && other == ask && ask > 0 && other >= price)
        price = other + 5;
    if (price > 1023)
        price = 1023;
    if (price - current > available)
        price = current + available / 5 * 5;
    return price;
}
static int t36_funds(U eval, Rec team, int *reserved, int *spent, int *unassigned) {
    U manager = eval ? q(eval + 0xc8) : 0, finance = manager ? q(manager + 0x1a0) : 0;
    if (!finance)
        return 0;
    *reserved = ((int (*)(U, Rec *))(game_base + 0x082ce2f0))(finance, &team);
    *spent = (int)scalar(team, 225, 0x100);
    *unassigned = (int)scalar(team, 227, 0x100);
    return *reserved >= 0 && *reserved <= 20000 && *spent >= 0 && *spent <= 20000 && *unassigned >= 0 &&
           *unassigned <= 20000;
}
/* The finance ledger already includes paid HS and portal promises. The coach
   percentage remains an opening instant spending policy, not a second debit
   against the normal scholarship wallet. */
static int r2_wallet(int reserve,int spent,int left) {
    return left + (reserve > spent ? reserve - spent : 0);
}
static int r2_quote(U eval,Rec team,Rec recruit,Rec target,int *ask,int *paid,int *wallet) {
    int reserve,spent,left;
    if(!t36_funds(eval,team,&reserve,&spent,&left))return 0;
    *wallet=r2_wallet(reserve,spent,left);
    U finance=q(q(eval+0xc8)+0x1a0);
    *ask=target?(int)scalar(target,4,0x100):
       ((int(*)(U,Rec*,Rec*))(game_base+0x082c0c40))(finance,&recruit,&team);
    *paid=target?(int)scalar(target,2,0x100):0;
    return *ask>=0 && *ask<=1023 && *paid>=0 && *paid<=1023;
}
static int t36_transfer_budget(U eval, Rec team) {
    int reserve, spent, unassigned;
    if (!t36_funds(eval, team, &reserve, &spent, &unassigned))
        return 0;
    int available = r2_wallet(reserve,spent,unassigned);
    Rec coach = reference(team, 111);
    if (!coach)
        return 0;
    Rec posture = reference(coach, 132);
    release(coach);
    if (!posture)
        return 0;
    int pct = (int)scalar(posture, 6, 0x100);
    release(posture);
    int limit = (int)scalar(team, 217, 0x100);
    if (pct < 0 || pct > 100 || limit < 0 || limit > 100000)
        return 0;
    limit = (int)((int64_t)limit * pct / 500) * 5;
    Rec board = board_array(team);
    U n = board ? array_count(board) : MAX_BOARD + 1;
    if (n > MAX_BOARD) {
        release(board);
        return 0;
    }
    for (U i = 0; i < n; i++) {
        Rec t = array_at(board, i), r = t ? reference(t, 10) : 0;
        if (!r) {
            release(t);
            release(board);
            return 0;
        }
        if (transfer_recruit(r))
            limit -= (int)scalar(t, 2, 0x100);
        release(r);
        release(t);
    }
    release(board);
    if (limit < 0)
        return 0;
    return limit < available ? limit : available;
}
static int t36_bid_debit(U eval, Rec team, Rec target, int new_price, int respect_category) {
    int old = (int)scalar(target, 2, 0x100), reserve, spent, left;
    int delta = new_price - old;
    if (delta <= 0)
        return delta == 0;
    if (old < 0 || new_price > 1023 || !t36_funds(eval, team, &reserve, &spent, &left))
        return 0;
    int earmarked = reserve > spent ? reserve - spent : 0;
    int draw = delta > earmarked ? delta - earmarked : 0;
    if (left < draw || spent + delta > 20000 || (respect_category && delta > t36_transfer_budget(eval, team)))
        return 0;
    U manager = q(eval + 0xc8), finance = manager ? q(manager + 0x1a0) : 0;
    if (!finance || !((unsigned char (*)(U, Rec *, int))(game_base + 0x082e5980))(finance, &team, delta))
        return 0;
    portal_recovery_scalar(target, 2, (unsigned)new_price);
    if ((int)scalar(target, 2, 0x100) == new_price && (int)scalar(team, 225, 0x100) == spent + delta &&
        (int)scalar(team, 227, 0x100) == left - draw)
        return 1;
    ((void (*)(U, Rec *, int))(game_base + 0x082e6780))(finance, &team, delta);
    portal_recovery_scalar(target, 2, (unsigned)old);
    portal_fault("T36_NIL_ledger_verification");
    return 0;
}
static int t36_rival_owner(U eval, U table, unsigned rid, int tid) {
    Rec f = context_ref(eval, 6), a = f ? array_field(f, 46) : 0;
    release(f);
    U n = a ? array_count(a) : 0;
    int result = 0;
    if (n <= 12000)
        for (U i = 0; i < n; i++) {
            Rec r = array_at(a, i);
            if (!r)
                break;
            if (q((U)r + 0x10) == table && row(r) == rid) {
                int owner = -1, stage = (int)scalar(r, 9, 0x108), ok = 0;
                if (stage == 5)
                    ok = portal_hard_owner(r, eval, &owner);
                if (stage == 6) {
                    Rec p = reference(r, 5);
                    if (p)
                        ok = portal_signed_destination(r, p, &owner);
                    release(p);
                    if (!ok)
                        ok = portal_published_owner(r, eval, &owner);
                }
                result = ok > 0 && owner >= 0 && owner < 138 && owner != tid;
                release(r);
                break;
            }
            release(r);
        }
    release(a);
    return result;
}
static void t36_paid_witness(Rec team, Rec target, Rec recruit, U eval, const PortalCommitPlan *scope,
                             int pos, int slot, int firm, int conditional, const char *site) {
    if (strcmp(site, "POST_FUNDING") || (!firm && !conditional) || slot < 1 || pos < 0 || pos >= 22)
        return;
    unsigned tid = scalar(team, 383, 0x100);
    if (tid >= 138 || portal_academy(tid))
        return;
    T36Week *now = &t36_recent[tid], *old = &t36_previous[tid];
    if (now->week != scope->week || now->year != scope->year || now->team != scope->team_table ||
        now->franchise != scope->franchise_table) {
        if (now->week + 1 == scope->week && now->year == scope->year && now->team == scope->team_table &&
            now->franchise == scope->franchise_table)
            *old = *now;
        else
            memset(old, 0, sizeof(*old));
        memset(now, 0, sizeof(*now));
        now->team = scope->team_table;
        now->franchise = scope->franchise_table;
        now->year = scope->year;
        now->week = scope->week;
    }
    U table = q((U)recruit + 0x10);
    unsigned rid = row(recruit);
    int before = (int)scalar(target, 2, 0x100), bid = before;
    if (firm)
        for (int j = 0; j < old->n; j++) {
            T36Paid *prior = old->a + j;
            if (!prior->conditional || prior->table != table || prior->rid != rid || prior->pos != pos ||
                prior->slot != slot)
                continue;
            for (int k = 0; k < old->n; k++) {
                T36Paid *lost = old->a + k;
                if (!lost->firm || lost->pos != pos || lost->slot != slot ||
                    !t36_rival_owner(eval, lost->table, lost->rid, (int)tid))
                    continue;
                int ask = (int)scalar(target, 4, 0x100), my = (int)scalar(target, 8, 0x100), leader = 0,
                    other = -1;
                Rec top = array_field(recruit, 13);
                U n = top ? array_count(top) : 0;
                for (U i = 0; i < n && n <= 138; i++) {
                    Rec x = array_at(top, i);
                    if (!x)
                        continue;
                    unsigned t = scalar(x, 0, 0x100);
                    int points = (int)scalar(x, 1, 0x100);
                    release(x);
                    if (t != tid && t < 138 && points > leader) {
                        Rec rival = portal_team_by_id(eval, t);
                        unsigned rp = 0;
                        int offered = rival ? portal_offer_total(recruit, rival, &rp) : 0;
                        if (offered > 0) {
                            leader = points;
                            Rec b = board_array(rival);
                            U m = b ? array_count(b) : 0;
                            for (U z = 0; z < m && m <= MAX_BOARD; z++) {
                                Rec rt = array_at(b, z), rr = rt ? reference(rt, 10) : 0;
                                int same = rr && portal_same_record(rr, recruit);
                                release(rr);
                                if (same)
                                    other = (int)scalar(rt, 2, 0x100);
                                release(rt);
                                if (same)
                                    break;
                            }
                            release(b);
                        }
                        release(rival);
                    }
                }
                release(top);
                int reserved=0,spent=0,unassigned=0;
                if(!t36_funds(eval,team,&reserved,&spent,&unassigned))break;
                int left=r2_wallet(reserved,spent,unassigned);
                int preference=t36_transfer_budget(eval,team);
                log_line("R2_NIL_WALLET,%u,%u,%u,%d,%d,%d,%d,%d,%d,%d",scope->week,tid,rid,
                         reserved,spent,unassigned,left,preference,before,ask);
                int desired = t36_nil_price(prior->bid, before, ask, other, leader > my, left);
                if (t36_bid_debit(eval, team, target, desired, 0))
                    bid = desired;
                log_line("TEST36_BACKUP_NIL,%u,%u,%u,%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,CONFIRMED_RIVAL_PRIMARY",
                         scope->week, tid, rid, lost->rid, slot, prior->bid, before, ask, other, leader - my,
                         left, bid, bid - before);
                break;
            }
            break;
        }
    int j = 0;
    while (j < now->n && (now->a[j].table != table || now->a[j].rid != rid))
        j++;
    if (j < MAX_BOARD) {
        if (j == now->n)
            now->n++;
        now->a[j] = (T36Paid){table, rid, pos, slot, firm, conditional, bid};
    }
}
