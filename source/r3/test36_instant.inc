/* R2H1: 08156320 assumes the chosen school is in TopSchoolsList. Its
   0862d2a0 lookup returns NULL for an unlisted HC destination; 081567da
   dereferences that result AFTER writing stage 5.

   Selection, mutual fit, eligibility, capacity, hours and full NIL payment
   precede this adapter. Publish the selected destination in the existing
   ten-slot list immediately before native acceptance. Native code still owns
   commitment and commit-score influence. Non-acceptance restores the list. */
static int t36_instant_failure(EXCEPTION_POINTERS *p, Rec recruit, Rec team) {
    EXCEPTION_RECORD *e = p ? p->ExceptionRecord : 0;
    U address = e ? (U)e->ExceptionAddress : 0;
    log_line("TEST36R2H1_INSTANT_EXCEPTION,%u,%u,0x%08lx,0x%llx,game_rva_0x%llx,%llu,0x%llx",
             row(recruit), scalar(team, 383, 0x100), e ? e->ExceptionCode : 0,
             (unsigned long long)address,
             (unsigned long long)(address >= game_base ? address - game_base : 0),
             (unsigned long long)(e && e->NumberParameters > 0 ? e->ExceptionInformation[0] : 0),
             (unsigned long long)(e && e->NumberParameters > 1 ? e->ExceptionInformation[1] : 0));
    return fail_exception("T36R2H1_instant", e ? e->ExceptionCode : 0);
}
static int t36_accept_opening(U manager, Rec recruit, Rec team, Rec target, int price) {
    Rec top = 0, linked = 0, entries[10] = {0};
    unsigned ids[10], points[10], next_ids[10], next_points[10], count = 0;
    int found = -1, changed = 0, accepted = 0;
    if (mode != 1 || faulted || !manager || !recruit || !team || !target ||
        !portal_same_record(recruit, t36_designated) || !portal_same_record(team, t36_destination))
        return 0;
    unsigned tid = scalar(team, 383, 0x100);
    if (tid >= 138 || price < 0 || price > 1023 || scalar(recruit, 9, 0x108) >= 5 ||
        scalar(target, 12, 0x108) < 2 || scalar(target, 2, 0x100) < (unsigned)price ||
        scalar(target, 2, 0x100) < scalar(target, 4, 0x100))
        return 0;
    __try {
        __try {
            linked = reference(target, 10);
            if (!portal_same_record(linked, recruit))
                __leave;
            top = array_field(recruit, 13);
            U n = top ? array_count(top) : 0;
            if (!n || n > 10)
                __leave;
            count = (unsigned)n;
            for (unsigned i = 0; i < count; i++) {
                entries[i] = array_at(top, i);
                if (!entries[i])
                    __leave;
                ids[i] = scalar(entries[i], 0, 0x100);
                points[i] = scalar(entries[i], 1, 0x100);
                if (ids[i] >= 138 || points[i] > 65535)
                    __leave;
                for (unsigned j = 0; j < i; j++)
                    if (ids[j] == ids[i] || portal_same_record(entries[i], entries[j]))
                        __leave;
                if (ids[i] == tid)
                    found = (int)i;
            }
            unsigned earned = found >= 0 ? points[found] : scalar(target, 8, 0x100);
            if (earned > 65535)
                __leave;
            next_ids[0] = tid;
            next_points[0] = earned;
            /* A listed destination retains all other schools. An unlisted
               accepted destination uses the final existing slot. No array
               expansion, allocation, bonus or rival offer removal. */
            for (unsigned i = 0, out = 1; i < count && out < count; i++)
                if ((int)i != found) {
                    next_ids[out] = ids[i];
                    next_points[out++] = points[i];
                }
            log_line("TEST36R2H1_INSTANT_PREPARE,%u,%u,%d,%u,%u,%u", row(recruit), tid, found,
                     count, ids[0], scalar(target, 2, 0x100));
            changed = 1;
            for (unsigned i = 0; i < count; i++) {
                if (ids[i] != next_ids[i])
                    portal_recovery_scalar(entries[i], 0, next_ids[i]);
                if (points[i] != next_points[i])
                    portal_recovery_scalar(entries[i], 1, next_points[i]);
            }
            for (unsigned i = 0; i < count; i++)
                if (scalar(entries[i], 0, 0x100) != next_ids[i] ||
                    scalar(entries[i], 1, 0x100) != next_points[i])
                    __leave;
            t36_immediate = 1;
            ((void (*)(U, Rec *, Rec *, void *))(game_base + 0x08156380))(manager, &target, &team, 0);
            accepted = !faulted && scalar(recruit, 9, 0x108) == 5;
        } __finally {
            t36_immediate = 0;
            if (changed && scalar(recruit, 9, 0x108) < 5)
                for (unsigned i = 0; i < count; i++) {
                    if (scalar(entries[i], 0, 0x100) != ids[i])
                        portal_recovery_scalar(entries[i], 0, ids[i]);
                    if (scalar(entries[i], 1, 0x100) != points[i])
                        portal_recovery_scalar(entries[i], 1, points[i]);
                }
            for (unsigned i = 0; i < count; i++)
                release(entries[i]);
            release(top);
            release(linked);
        }
    } __except (t36_instant_failure(GetExceptionInformation(), recruit, team)) {
        accepted = 0;
    }
    log_line("TEST36R2H1_INSTANT_RESULT,%u,%u,%d,%s", row(recruit), tid, accepted,
             accepted ? "NATIVE_ACCEPTED" : faulted ? "FAULT_RESTART" : "DEFERRED_LIST_PRESERVED");
    return accepted;
}
