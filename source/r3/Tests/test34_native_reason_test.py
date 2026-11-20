"""Trace captured native rejection strings before their matching allocator free.

Native rejection/format/grow/reserve instructions execute in emulation. Schema,
grade services and imports are modeled; no live-game outcome is claimed.
"""
from pathlib import Path
import json
native_reason_scope = __doc__

# Reuse the captured native string implementation and guarded allocator model,
# stopping before the historical R9 negative-control branch and its assertions.
setup = Path(__file__).with_name('test28r10_eligibility_reason_test.py').read_text()
exec(compile(setup.split("if '--r9-negative-control' in sys.argv:")[0], __file__, 'exec'))
assert b'DRI_PORTAL_TEST34' in image
assert 'DriTestHs34NativeEligibility' in exports
# The older R9 prefix installs a no-op formatter at +0x7ac0c0. The R10
# restoration ends before that formatter and its +0x74b500 growth function.
# Restore both captured pages; keep the real native calls and allocator checks.
for address, length in [(0x007ac000, 0x1000), (0x0074b000, 0x1000)]:
 map_game(address, length)
 uc.mem_write(GAME+address, raw[address-0x1000:address-0x1000+length])
 for handler in list(handlers):
  if GAME+address <= handler < GAME+address+length:
   del handlers[handler]
 uc.ctl_remove_cache(GAME+address, GAME+address+length)

def copied_reason():
 data = bytes(call(exports['DriTestHs34ReasonByte'], i) for i in range(96))
 assert b'\0' in data, 'diagnostic reason lacks a terminator'
 return data.split(b'\0', 1)[0]

def expected_reason(source):
 source = source[:95].split(b'\0', 1)[0]
 return bytes(59 if c == 44 else 32 if c < 32 or c == 127 else 63 if c > 126 else c
              for c in source) if source else b'NATIVE_REJECTED_NO_REASON'

cases = []
for length in [0, 1, 14, 15, 16, 30, 31, 80, 95, 96, 255, 1024, 80, 1, 16]:
 # Exercise CSV-safe sanitation as well as the native 15-byte SSO transition.
 payload = (b'Grade,\t\n\x7f\x80 rejected. ' * 60)[:length]
 assert len(payload) == length
 current_grade = 5
 before = (len(allocations), len(frees), len(formatter_calls), len(initialized))
 seen.clear()
 assert call(exports['DriTestHs34NativeEligibility'], team, backup, ctx) == 0
 assert seen == ['requirement', 'grade']
 a, f, prints, init = (len(allocations)-before[0], len(frees)-before[1],
                       len(formatter_calls)-before[2], len(initialized)-before[3])
 assert a == f == int(length > 15)
 assert prints == (2 if length else 1) and init == 1, (length, prints, init, a, f)
 reason = initialized[-1]
 if length <= 15:
  assert bytes(uc.mem_read(reason, length+1)) == payload+b'\0'
  assert bytes(uc.mem_read(reason+15, 1)) == bytes([15-length])
 assert call(exports['DriTestQualityFault']) == 0
 assert not live_allocations
 assert copied_reason() == expected_reason(payload), (length, copied_reason(), expected_reason(payload))
 cases.append({'reason_length': length, 'copied_length': len(copied_reason()),
               'heap_allocations': a, 'matching_frees': f, 'allowed': False})
 # Each fresh trace must replace the previous rejection and leave no heap work.
 current_grade = 10
 before_success = (len(allocations), len(frees), len(formatter_calls))
 assert call(exports['DriTestHs34NativeEligibility'], team, backup, ctx) == 1
 assert copied_reason() == b'ALLOWED'
 assert before_success == (len(allocations), len(frees), len(formatter_calls))
 assert call(exports['DriTestQualityFault']) == 0

call(exports['DriTestOwnershipReset'])
assert all(v == 1 for v in refs.values()), [(records[r]['name'], v) for r, v in refs.items() if v != 1]
result = {'scope': native_reason_scope, 'cases': cases, 'success_after_each_rejection': True,
          'allocations': len(allocations), 'frees': len(frees),
          'no_live_allocations': not live_allocations, 'live_game_verified': False}
(OUTPUT/'test34_native_reason_results.json').write_text(json.dumps(result, indent=2)+'\n')
print(json.dumps(result, indent=2))
