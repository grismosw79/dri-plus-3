"""Replay captured native closest-anchor helper against every shipped FTC curve.

Requires Python unicorn. The native nearest helper and native field accessor
execute unchanged; a scalar database-read stub supplies authored FTC integers.
This is an offline fixture, not live game or complete schema integration.
Run after all five build_test28_ftc.cjs invocations.
"""
import hashlib
import json
import struct
from pathlib import Path

from unicorn import Uc, UC_ARCH_X86, UC_MODE_64
from unicorn.x86_const import (
    UC_X86_REG_RSP, UC_X86_REG_RCX, UC_X86_REG_EDX, UC_X86_REG_EAX,
)

PACKAGE = Path(__file__).resolve().parents[3]
AUDIT = PACKAGE / "Audit/Test28"
FIXTURE = AUDIT / "NativeRequirementFixture"
BASE = 0x140000000
HELPER = BASE + 0x0860dd70
ACCESSOR = BASE + 0x08055d00
STACK = 0x110000
WRAPPER, RECORD, SCHEMA, OBJECT, VTABLE = range(0x200000, 0x200500, 0x100)
ARRAY, VALUES, SCALAR, STOP = 0x200800, 0x200900, 0x300000, 0x301000

descriptor = json.loads((FIXTURE / "descriptor.json").read_text())
helper = (FIXTURE / "nearest.bin").read_bytes()
accessor = (FIXTURE / "field-accessor.bin").read_bytes()
assert hashlib.sha256(helper).hexdigest() == descriptor["nearestSHA256"]
assert hashlib.sha256(accessor).hexdigest() == descriptor["accessorSHA256"]
keys = [p["name"] for p in descriptor["pairs"]]
assert [p["ordinal"] for p in descriptor["pairs"]] == list(range(13))

u = Uc(UC_ARCH_X86, UC_MODE_64)
for addr, size in [(HELPER & ~4095, 8192), (ACCESSOR & ~4095, 8192),
                   (0x100000, 0x20000), (0x200000, 0x2000), (SCALAR, 0x2000)]:
    u.mem_map(addr, size)
u.mem_write(HELPER, helper)
u.mem_write(ACCESSOR, accessor)
u.mem_write(SCALAR, b"\x41\x8b\x00\xc3")  # mov eax,[r8]; ret

def qword(address, value):
    u.mem_write(address, struct.pack("<Q", value))

qword(WRAPPER + 8, RECORD)
qword(WRAPPER + 16, SCHEMA)
qword(RECORD + 16, OBJECT)
u.mem_write(RECORD + 24, struct.pack("<I", 0))
qword(OBJECT, VTABLE)
qword(VTABLE + 0x100, SCALAR)
qword(SCHEMA + 0xD0, ARRAY)
u.mem_write(SCHEMA + 0xD8, struct.pack("<h", 13))
for i in range(13):
    qword(ARRAY + i * 8, VALUES + i * 4)

def captured_nearest(values, value):
    u.mem_write(VALUES, struct.pack("<13i", *values))
    qword(STACK, STOP)
    u.reg_write(UC_X86_REG_RSP, STACK)
    u.reg_write(UC_X86_REG_RCX, WRAPPER)
    u.reg_write(UC_X86_REG_EDX, value)
    u.emu_start(HELPER, STOP, count=3000)
    return u.reg_read(UC_X86_REG_EAX)

def nearest(values, value):
    best_difference, best_index = 200, 0
    for i, anchor in enumerate(values):
        difference = abs(anchor - value)
        if difference == 0:
            return i
        if difference < best_difference:
            best_difference, best_index = difference, i
    return best_index

routes = []
for route in range(5):
    audit = json.loads((AUDIT / f"ftc-route-{route}.json").read_text())
    ftc = PACKAGE / f"frantkdata/{route}/dynasty-tuning-binary.FTC"
    assert hashlib.sha256(ftc.read_bytes()).hexdigest() == audit["sha256"]
    assert audit["nativeRequirementSemantics"]["gradeKeys"] == keys
    star_cases = roster_cases = 0
    differences = {}
    for curve in audit["nativeCurves"]:
        for value, expected, floor in zip(curve["inputs"], curve["nativeResults"], curve["explicitFloorResults"]):
            actual = captured_nearest(curve["values"], value)
            assert actual == expected == nearest(curve["values"], value), (route, curve["motivation"], curve["phase"], value, actual, expected)
            if curve["phase"] == "roster":
                roster_cases += 1
                if actual != floor:
                    differences.setdefault(curve["motivation"], []).append(value)
            else:
                star_cases += 1
                assert curve["values"][actual] >= 0
                if curve["motivation"] == "ProximityToHome":
                    assert actual == 4
                elif curve["phase"] == "HS":
                    expected_hs = [12, 11, 8, 2, 2] if curve["motivation"] == "ConferencePrestige" else ([11, 10, 8, 6, 4] if route in [1, 2] else [12, 11, 8, 6, 4])
                    assert actual == expected_hs[value]
    assert star_cases == 80 and roster_cases == 800
    routes.append({"route": route, "HSAndPortalNativeStarCases": star_cases,
                   "rosterNativeOVRCases": roster_cases,
                   "nativeVsFloorDifferences": differences})

# Independent edge witnesses: exact-match wins, ties favor better grade, and
# native does not discard inactive negative anchors. The runtime floor policy
# intentionally differs and must never be tested using this native rule.
witnesses = [([-100] * 4 + [0] + [-100] * 8, 4, 4),
             ([-100] * 4 + [80, 75] + [-100] * 7, 77, 5),
             ([-100] * 4 + [80, 74] + [-100] * 7, 77, 4),
             ([-1] + [-100] * 11 + [40], 0, 0)]
for values, value, expected in witnesses:
    assert captured_nearest(values, value) == expected

result = {
    "status": "PASS_OFFLINE_LIVE_PENDING",
    "actualCapturedInstructionCases": sum(r["HSAndPortalNativeStarCases"] + r["rosterNativeOVRCases"] for r in routes) + len(witnesses),
    "HSAndPortalNativeStarCases": 400,
    "rosterNativeOVRCases": 4000,
    "edgeWitnesses": len(witnesses),
    "helperRVA": hex(HELPER - BASE), "accessorRVA": hex(ACCESSOR - BASE),
    "scope": "Unchanged captured native nearest helper and field accessor, with synthetic database scalar reads. Exact college policy depends on the matching Test28 DLL explicit-floor override on roster and portal paths. No live-game verification.",
    "routes": routes,
}
(AUDIT / "ftc-native-requirement-replay.json").write_text(json.dumps(result, indent=2) + "\n")
print(json.dumps({k: v for k, v in result.items() if k != "routes"}))
