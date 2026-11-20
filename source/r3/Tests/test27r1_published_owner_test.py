"""Regression from Test27 TP4 + recorded NSD offers, compiled callbacks.

The saved acquisition/stage and logged NSD destination are replayed as controlled
SDF records. This is not a full native season advance. Set DRI_TEST27_EXPECT_OLD=1
with the original Test27 fixture to reproduce its conflicting publication.
"""
from pathlib import Path
fixture = Path(__file__).parent / 'recovery_runtime_test.py'
source = fixture.read_text().split('\nreset_recovery()\n# Test10-shaped')[0]
exec(compile(source, str(fixture), 'exec'))
import re

old = os.environ.get('DRI_TEST27_EXPECT_OLD') == '1'
data = json.loads((ROOT.parent / 'Audit/Test27Fault/published-owner-cases.json').read_text())
names = re.findall(r'^\.global\s+(Hook\w+)', (ROOT / 'hooks.S').read_text(), re.M)
names = [n for n in names if not n.startswith('HookEps')]
bulk = hooks[next(i for i, n in enumerate(names) if 'BulkGuard' in n)]
game_api(0x0816b130, lambda: 1)
checks27 = []
positions = 'QB HB FB WR TE LT LG C RG RT LE RE DT LOLB MLB ROLB CB FS SS K P LS'.split()
stages = {'Top10': 0, 'SoftCommitted': 4}

def acquire(player, tid):
    records[records[allteams[tid]]['ref'][33]]['array'].append(player)

def published_teams(player):
    return [i for i, t in enumerate(allteams) if player in records[records[t]['ref'][33]]['array']]

def choose(rec):
    result = call(exports['DriTestNsdWinner'], rec, context, 1)
    if result:
        decref(result)
    return result

def signing(rec, tid, deadline=False):
    f = STACK + 0x65000
    bp = f + 0x400
    wq(f + 10 * 8, bp)
    wq(f + 128 + 0x30, rec)
    wq(f + 11 * 8, allteams[tid])
    wq(f, allteams[tid])
    wq(bp + 0x57, context)
    wq(bp + 0x40, context)
    return call(deadline_cb if deadline else sign_cb, f)

def bulk_guard(rec, tid):
    f = STACK + 0x65000
    bp = f + 0x400
    wq(f + 10 * 8, bp)
    wq(bp - 0x19, rec)
    wq(f + 3 * 8, allteams[tid])  # R12
    wq(f + 2 * 8, context)       # R13
    return call(bulk, f)

reset_recovery()
records[season]['scalar'].update({4: 6, 5: 6})
observed = []
for case in data['cases']:
    owner, rival = case['prior'][0], case['pending'][0]
    r, p = rr(case['rid'], ovr=case['ovr'], stage=stages[case['stage']],
              schools=tuple((s['TeamId'], s['TeamInfluence']) for s in case['recruit']['top']),
              offers=(rival,), portal=False)
    records[p]['scalar'].update({3: positions.index(case['pos']), 231: 2})
    acquire(p, owner)
    chosen = choose(r)
    assert call(exports['DriTestQualityFault']) == 0
    if old:
        assert chosen == allteams[rival], (case['name'], chosen)
        assert signing(r, rival) == 0
        # Controlled publication models the newly accepted signing appending
        # its Player to the second school's list, as the live fault reports.
        acquire(p, rival)
    else:
        assert chosen == 0, (case['name'], chosen)
        assert signing(r, rival) == 13
        assert signing(r, owner) == 13
        assert bulk_guard(r, rival) == 13
        assert bulk_guard(r, owner) == 13
        assert published_teams(p) == [owner]
        assert records[r]['scalar'][9] == stages[case['stage']]
    observed.append({'recruit': case['rid'], 'name': case['name'], 'owner': owner,
                     'loggedRival': rival, 'publishedTeamsAfterReplay': published_teams(p)})

result = call(exports['DriTestCleanupAll'], context)
assert (result, call(exports['DriTestQualityFault'])) == ((0, 1) if old else (1, 0))
checks27.append('Original fixture reproduces published-owner conflict' if old else
                'Both recorded HS cases retain one published owner; NSD does not re-offer them; cleanup succeeds')

if not old:
    # Existing ownership applies to every class and user/CPU destination.
    for is_portal in (False, True):
        for stage in (0, 3, 4, 5):
            for user in (False, True):
                reset_recovery()
                r, p = rr(58000 + stage, stage=stage, offers=(11,), portal=is_portal)
                acquire(p, 10)
                if user:
                    records[allteams[11]]['ref'][407] = record('rival user')
                before = (values(r), dict(records[r]['scalar']), published_teams(p))
                assert choose(r) == 0
                assert signing(r, 11, is_portal) == 13
                assert signing(r, 10, is_portal) == (0 if is_portal else 13)
                assert bulk_guard(r, 11) == 13
                assert bulk_guard(r, 10) == (0 if is_portal else 13)
                assert before == (values(r), dict(records[r]['scalar']), published_teams(p))
    checks27.append('HS/portal stages 0,3,4,5: rival CPU/user signing blocked; HS re-admission blocked; portal owner retains native finalization; stored fields unchanged')

    # Published soft choices must not be reopened by either recovery route.
    for is_portal in (False, True):
        reset_recovery()
        r, p = rr(58100 + is_portal, stage=4, offers=(13,), portal=is_portal)
        records[p]['scalar'][231] = 2
        for t in records[board_targets(allteams[13])]['array']:
            records[t]['scalar'][8] = 37
        acquire(p, 10)
        before = (values(r), dict(records[r]['scalar']), published_teams(p))
        if is_portal:
            assert run_recovery() == 0
        else:
            call(exports['DriTestHsRecoverTeam'], allteams[13], context)
            call(exports['DriTestDeadlineRecovery'], r, context)
        assert call(exports['DriTestQualityFault']) == 0
        assert before == (values(r), dict(records[r]['scalar']), published_teams(p))
    checks27.append('HS and portal recovery leave published soft acquisitions and their interest/offer data untouched')

    reset_recovery()
    r, p = rr(58110, stage=3, schools=((10,25),(11,20)), offers=(10,11), portal=False)
    assert choose(r) == allteams[10]
    checks27.append('Unacquired recruit still selects highest active valid offer below the normal commitment threshold')

    for wanted in (0, -1):
        reset_recovery()
        r, p = rr(58120, stage=4, offers=(11,), portal=False)
        acquire(p, 10)
        configure(wanted)
        before = (values(r), dict(records[r]['scalar']), published_teams(p))
        assert choose(r) == 0 and signing(r, 11) == 0 and bulk_guard(r, 11) == 0
        assert before == (values(r), dict(records[r]['scalar']), published_teams(p))
    checks27.append('Diagnostic and disabled modes retain native callbacks without ownership writes or vetoes')

    reset_recovery()
    r, p = rr(58130, stage=4, offers=(11,), portal=False)
    acquire(p, 10); acquire(p, 12)
    before = (values(r), dict(records[r]['scalar']), published_teams(p))
    assert choose(r) == 0 and call(exports['DriTestQualityFault']) == 1
    assert before == (values(r), dict(records[r]['scalar']), published_teams(p))
    checks27.append('Genuinely conflicting published ownership still disables behavior before any custom mutation')

call(exports['DriTestOwnershipReset'])
assert all(v == 1 for v in refs.values()), [(records[k]['name'], v) for k,v in refs.items() if v != 1]
report = {'checks': checks27, 'sourceSaveSHA256': data['sourceSaveSHA256'],
          'cases': observed, 'negativeControl': old, 'live_game_verified': False}
(OUTPUT / 'test27r1_published_owner_results.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, indent=2))
