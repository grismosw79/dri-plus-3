"""Execute the compiled Test28 PP callbacks against controlled SDF storage.

Requires retention_fixture.dll and the original captured code file, using the
same DRI_TEST_OUTPUT_DIR / DRI_CAPTURE_DIR variables as runtime_fixture_test.py.
No game or save is edited. These are runtime ABI fixtures, not a live lifecycle.
"""
from pathlib import Path
import json

_common = Path(__file__).with_name('runtime_fixture_test.py').read_text()
exec(compile(_common.split("prefix=bytes.fromhex(")[0], __file__, 'exec'))
_old_import = win_import
def win_import(name):
    if name == 'calloc':
        count, size, _, _ = args()
        address = alloc(count * size)
        uc.mem_write(address, bytes(count * size))
        return address
    if name == 'free':
        return 0
    return _old_import(name)

def map_game(rva):
    page = GAME + (rva & ~0xfff)
    if not any(lo <= page <= hi for lo, hi, _ in uc.mem_regions()):
        uc.mem_map(page, 0x1000)

map_game(0x08059580)
dirty = []
api(lambda: dirty.append(reg('RCX')) or 0, GAME + 0x08059580)
mutations = []
reject = [None]
def set_scalar():
    table, row, field, value = args()
    rec = tables[table][row]
    ordinal = field - 0x100000
    mutations.append((rec, ordinal, value))
    if reject[0] == rec:
        reject[0] = None
        return 0
    records[rec]['scalar'][ordinal] = value & 0xffffffff
    return 0
wq(TABLEVT + 0x148, api(set_scalar))
# The captured native aging helper also queries the array-table interface for
# its active length. Model that SDF interface without replacing the age loop.
wq(TABLEVT, api(lambda: 0))
wq(TABLEVT + 8, api(lambda: 0))
def query_array_table():
    table, out, tag, _ = args()
    assert tag == 0x49415354
    wq(out, table)
    return out
wq(TABLEVT + 0x28, api(query_array_table))
wq(TABLEVT + 0x1a8, api(lambda: len(records[tables[reg('RCX')][reg('RDX')]]['array'])))
map_game(0x083fdda0)
uc.mem_write(GAME + 0x083fdda0, raw[0x083fdda0 - 0x1000:0x083fdda0])
map_game(0x08052720)
removed = []
def erase_history():
    array, index, _, _ = args()
    removed.append(records[array]['array'].pop(index))
    return index
api(erase_history, GAME + 0x08052720)
native_age_holder = alloc(8)
def native_age(array):
    wq(native_age_holder, array)
    call(GAME + 0x083fe420, 0, native_age_holder)

def configure(mode=1):
    call(exports['DriTestConfigure'], GAME, mode, 7)

team = record('PP team', {383: 2})
season = record('PP season', {0: 2026, 2: 2026, 6: 0})
franchise = record('PP franchise', ref={44: season})
ctx_record = record('PP context', ref={2: franchise})
context = alloc(0x1800)
wq(context + 0x10, rq(ctx_record + 0x10))
wi(context + 0x18, ri(ctx_record + 0x18))
wq(context + 0x30, 0xabcdef)
annual = GAME + 0x08430a8d
initialization = GAME + 0x0812ccfb

def history(ages):
    picks = [record('PP pick ' + str(i), {0: age, 1: i % 7, 4: i % 10}, ref={3: team})
             for i, age in enumerate(ages)]
    array = record('PP draft array', array=picks)
    return array, picks

def ages(picks):
    return [records[p]['scalar'][0] for p in picks]

def normalize(array, caller=annual):
    return call(exports['DriTestPPHistory'], context, array, caller)

results = []
configure()
array, picks = history([0, 2, 2, 3, 4, 0])
assert normalize(array) == 1
assert ages(picks) == [0, 1, 1, 2, 3, 0]
assert len(mutations) == len(dirty) == 4
assert all(ordinal == 0 and rec not in [picks[0], picks[-1]] for rec, ordinal, _ in mutations)
assert normalize(array) == 0 and len(mutations) == 4
native_age(array)  # Execute the actual captured aging routine exactly once.
assert ages(picks) == [1, 2, 2, 3, 4, 1]
assert normalize(array) == 0 and len(mutations) == 10 and not removed
unfixed_array, unfixed = history([0, 2, 3, 4])
expired = unfixed[-1]
native_age(unfixed_array)
assert ages(records[unfixed_array]['array']) == [1, 3, 4] and removed == [expired]
results.append('Repair plus captured native age yields 1/2/3/4 once with no removal; native unrepaired history loses age4; repeats are no-ops')

array, picks = history([0, 2, 3, 4])
assert normalize(array, initialization) == 0 and ages(picks) == [0, 2, 3, 4]
records[season]['scalar'][6] = 1
assert normalize(array) == 0 and ages(picks) == [0, 2, 3, 4]
records[season]['scalar'][6] = 0
records[season]['scalar'][2] = 2027
assert normalize(array) == 0 and ages(picks) == [0, 2, 3, 4]
records[season]['scalar'][2] = 2026
for original in [[2, 3, 4], [0, 1, 3, 4], [0, 2, 3], [0, 2, 3, 4, 5], []]:
    a, p = history(original)
    assert normalize(a) == 0 and ages(p) == original
results.append('Initialization, later calendar/dynasty, missing new class, incomplete and already-aged windows do not mutate')

array, picks = history([0, 2, 3, 4])
records[array]['array'].append(picks[2])
before = len(mutations)
assert normalize(array) == 0xffffffff and len(mutations) == before
configure()
array, picks = history([0, 2, 3, 4])
records[picks[2]]['scalar'][4] = 10
assert normalize(array) == 0xffffffff and len(mutations) == before
results.append('Duplicate record identities and invalid position groups fail before any write')

configure()
array, picks = history([0, 2, 3, 4])
reject[0] = picks[2]
before = len(mutations)
assert normalize(array) == 0xffffffff
assert ages(picks) == [0, 2, 3, 4]
assert len(mutations) - before == 4  # Two attempts; both reverted through native writer.
results.append('Readback failure rolls back every attempted historical write and disables custom behavior')

# Exercise the actual hook callback register/stack contract as well as the
# exported underlying normalizer, including preservation in diagnostic mode.
configure()
array, picks = history([0, 2, 3, 4])
frame = alloc(128)
bp = alloc(0x300) + 0x100
wq(frame + 16, context)  # Saved R13.
wq(frame + 80, bp)       # Saved RBP.
wq(bp + 0x6f, array)
wq(bp + 0x5f, annual)
configure(0)
call(exports['DriTestPPBeforeAge'], frame)
assert ages(picks) == [0, 2, 3, 4]
configure()
call(exports['DriTestPPBeforeAge'], frame)
assert ages(picks) == [0, 1, 2, 3]
results.append('Pre-aging callback reads exact R13/frame holders and honors diagnostic mode')

cuts = [5, 10, 15, 25, 35, 45, 55, 65, 75, 90, 105, 120, 250]
for i, cutoff in enumerate(cuts):
    wi(context + 0x10ec + 4*i, cutoff)
uc.mem_write(context + 0x10e8, b'\0')
vector = alloc(138 * 0xb0)
track = [record('PP tracking ' + str(i)) for i in range(138)]
teams = [record('PP school ' + str(i), {383: i}) for i in range(138)]
for i in range(138):
    wq(vector + 0xb0*i, track[i])
    wq(vector + 0xb0*i + 8, teams[i])
wq(bp - 0x59, vector)
wq(bp - 0x51, vector + 138*0xb0)
wq(frame + 88, 138)  # Saved RBX.

def publish(evidence, index, group=0, native_grade=12):
    for i, (hist, projected, composite) in enumerate(evidence):
        entry = vector + 0xb0*i
        wi(entry + 0x34 + 4*group, hist)
        wi(entry + 0x5c + 4*group, projected)
        wi(entry + 0x84 + 4*group, composite)
    wq(frame, index)       # Saved R15.
    wq(frame + 24, group)  # Saved R12.
    wq(frame + 72, native_grade)
    wq(bp - 0x71, track[index])
    call(exports['DriTestPPPublish'], frame)
    return rq(frame + 72)

zero = [(0, 0, 0)] * 138
configure()
assert all(publish(zero, i, i % 10) == 7 for i in range(138))
leaders = [(10, 100, 715)] * 7 + zero[7:]
assert all(publish(leaders, i) == 0 for i in range(7))
middle = [(20, 200, 1430)] * 21 + [(10, 100, 715)] * 11 + zero[32:]
assert all(publish(middle, i) == 4 for i in range(21, 32))
low = [(20, 200, 1430)] * 137 + [(1, 0, 70)]
assert publish(low, 137) == 7
results.append('Compiled publication callback covers all 138 schools and ten groups; zero C, midpoint ties, positive C floor')

# Use the original machine-code setter/getter and its captured jump table.
# This proves the changed RSI value is the exact native field value consumed
# by all ten native Pro Potential groups; this is not a hand-written getter.
for page in [0x0860eda0, 0x08610da0]:
    map_game(page)
    uc.mem_write(GAME + page, raw[page - 0x1000:page])
wrapper = alloc(24)
wq(wrapper + 8, track[0])
wq(wrapper + 16, 0xabcdef)
native_fields = [30, 31, 33, 32, 28, 25, 27, 24, 26, 29]
for group, ordinal in enumerate(native_fields):
    expected = publish(zero if group % 2 else leaders, 0, group)
    call(GAME + 0x08611060, wrapper, group, expected)
    assert records[track[0]]['scalar'][ordinal] == expected
    assert call(GAME + 0x0860f720, wrapper, group) == expected
results.append('Captured native setter and getter publish/read corrected grade through all ten actual field ordinals')

saved = json.loads(Path(__file__).with_name('pp28_saved_evidence.json').read_text())
for group, fixture in enumerate(saved['groups'].values()):
    for school, expected in enumerate(fixture['expected']):
        assert publish(fixture['evidence'], school, group) == expected, (group, school, expected)
results.append('All 1,380 reconstructed candidate evidence cases match the accepted midpoint/C-floor grade model')

# Enter both actual DLL hook wrappers at the mid-function ABI (aligned RSP),
# run the real native age/setter once, and stop at their exact resume target.
configure()
addresses = alloc(32)
call(exports['DriTestPPAddresses'], addresses)
hook_age, hook_publish, native_age_address, native_setter_address = [rq(addresses + 8*i) for i in range(4)]
assert (native_age_address, native_setter_address) == (GAME + 0x083fe420, GAME + 0x08611060)
cs = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
cs.detail = True
def hook_callback(address):
    return next(i.operands[0].imm for i in cs.disasm(bytes(uc.mem_read(address, 100)), address) if i.mnemonic == 'call')

def run_mid_hook(address, resume, native, values, changed=None):
    map_game(resume - GAME)
    uc.mem_write(resume, b'\x90')
    stack = STACK + 0xd0000
    values = dict(values, RSP=stack)
    for name, value in values.items():
        uc.reg_write(globals()['UC_X86_REG_' + name], value)
    xmm = [(0xabc00000000000000000000000000000 + i) for i in range(16)]
    for i, value in enumerate(xmm):
        uc.reg_write(globals()['UC_X86_REG_XMM' + str(i)], value)
    uc.reg_write(UC_X86_REG_EFLAGS, 0x247)
    callback = hook_callback(address)
    visits = {callback: 0, native: 0, resume: 0}
    def watch(u, at, size, data):
        if at in visits:
            visits[at] += 1
        if at == resume:
            u.emu_stop()
    handle = uc.hook_add(unicorn.UC_HOOK_CODE, watch)
    try:
        uc.emu_start(address, STOP + 1, count=2000000)
    finally:
        uc.hook_del(handle)
    assert reg('RIP') == resume and visits == {callback: 1, native: 1, resume: 1}, visits
    for name in ['RBX', 'RBP', 'RSI', 'RDI', 'R12', 'R13', 'R14', 'R15', 'RSP']:
        assert reg(name) == (changed or {}).get(name, values[name]), (name, hex(reg(name)))
    assert [uc.reg_read(globals()['UC_X86_REG_XMM' + str(i)]) for i in range(16)] == xmm

array, picks = history([0, 2, 3, 4])
wq(bp + 0x6f, array)
wq(bp + 0x5f, annual)
regs = dict(RBX=0x123456, RBP=bp, RSI=0x223344, RDI=0x345678,
            R12=0x456789, R13=context, R14=0x56789a, R15=0x6789ab)
before = len(mutations)
run_mid_hook(hook_age, GAME + 0x08422835, native_age_address, regs)
assert ages(picks) == [1, 2, 3, 4] and len(mutations) - before == 7  # 3 corrections + 4 native increments.
assert reg('R9') == 0 and reg('EFLAGS') & 0x45 == 0x44
assert publish(zero, 0, group=4) == 7
wq(bp - 0x79, 0)  # Native wrapper vtable is not read by setter.
wq(bp - 0x69, 0xabcdef)
regs.update(RBX=138, RSI=12, R12=4, R15=0)
before = len(mutations)
run_mid_hook(hook_publish, GAME + 0x08422f4c, native_setter_address, regs, {'RSI': 7})
assert len(mutations) - before == 1 and records[track[0]]['scalar'][28] == 7
results.append('Both compiled hook wrappers call their callback/native target once, resume exactly, preserve stack/nonvolatile GPRs/all XMM, and publish/age correctly')

configure(0)
assert publish(zero, 0, native_grade=12) == 12
configure()
assert publish(zero, 0, native_grade=12) == 7
uc.mem_write(context + 0x10e8, b'\1')
assert publish(leaders, 0, native_grade=12) == 12
uc.mem_write(context + 0x10e8, b'\0')
assert publish(leaders, 0, native_grade=12) == 12  # Fault remains disabled.
results.append('Publication diagnostic mode preserves native grade; unexpected cutoff mode fails closed')
assert all(value == 1 for value in refs.values()), [(records[r]['name'], n) for r, n in refs.items() if n != 1]
results.append('All acquired SDF references return to their original ownership count')

report = {'test': 'pp28_compiled_runtime', 'passed': results, 'live_game': False}
(OUTPUT / 'pp28_runtime_results.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, indent=2))
