"""Reject direct branches into overwritten hook interiors in containing functions.

Function bounds come from the captured Windows exception directory. This is a
control-flow check in addition to instruction-boundary/signature checks; it does
not claim to discover indirect branches or arbitrary external entry points.
"""
from pathlib import Path
import os, re, struct, json, functools
import capstone, pefile

S = Path(__file__).resolve().parents[1]
C = Path(os.environ['DRI_CAPTURE_DIR'])
raw = (C / 'section_00_00001000.bin').read_bytes()
pe = pefile.PE(data=(C / 'pe_headers.bin').read_bytes(), fast_load=True)
directory = pe.OPTIONAL_HEADER.DATA_DIRECTORY[3]
pdata = (C / 'section_04_0FCDC000.bin').read_bytes()[:directory.Size]
functions = list(struct.iter_unpack('<III', pdata))
hooks = [(int(m[1],16), int(m[2]), m[3]) for m in re.finditer(
    r'\{(0x[0-9a-f]+),(\d+),\{[^}]+\},(Hook\w+)\}',
    (S / 'retention.c').read_text())]
assert len(hooks) == int(os.environ.get('DRI_TEST_EXPECTED_HOOK_COUNT','39'))
assert any(name == 'HookHsShortlist' for _,_,name in hooks)
assert any(name == 'HookHsPostFunding' for _,_,name in hooks)
assert {'HookPPBeforeAge','HookPPPublish','HookStaff28Update','HookPt28Grade','HookProximity28Grade','HookPPGroupGrade'} <= {name for _,_,name in hooks}
cs = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
cs.detail = True
rdata=(C/'section_02_0AC9C000.bin').read_bytes()
@functools.lru_cache(None)
def root(entry):
    a,b,unwind=entry
    off=unwind-0x0ac9c000
    assert 0<=off<len(rdata)-4
    flags=rdata[off]>>3
    if flags&4:
        chain=off+4+((rdata[off+2]+1)&~1)*2
        return root(struct.unpack_from('<III',rdata,chain))
    return entry
roots=set()
for rva,size,name in hooks:
    found=[e for e in functions if e[0]<=rva<e[1]]
    assert len(found)==1,(name,found)
    roots.add(root(found[0]))
ranges={(a,b) for a,b,c in functions if root((a,b,c)) in roots}
for rva,size,name in hooks:
    assert all(any(a<=v<b for a,b in ranges) for v in range(rva,rva+size)),name
branches = []
# These captured functions place switch targets and compact index maps after
# their final RET, within the Windows unwind fragment's nominal extent.
# Validate their targets separately instead of decoding table bytes as code.
literal_regions=[(0x08407344,0x0840737c),(0x0840756c,0x084075a4),(0x0842252c,0x084225ef),(0x08407750,0x08407788),(0x0862db4c,0x0862db64),(0x0819b788,0x0819b7b4)]
switch_tables=[(0x08407344,14),(0x0840756c,14),(0x0842252c,10),(0x08422594,12),(0x08407750,14),(0x0862db4c,6),(0x0819b788,11)]
switch_targets=[]
# Universal action eligibility dispatch: cmp action,10; ja default; indexed RVA table.
assert raw[0x0819b386-0x1000:0x0819b389-0x1000]==bytes.fromhex("83f80a")
for address,count in switch_tables:
    targets=struct.unpack_from('<'+'I'*count,raw,address-0x1000)
    for target in targets:
        assert any(a<=target<b for a,b in ranges),(hex(address),hex(target))
        assert not any(a<=target<b for a,b in literal_regions),(hex(address),hex(target))
        assert not any(a<target<a+n for a,n,_ in hooks),(hex(address),hex(target))
    switch_targets.extend(targets)
assert max(raw[0x8422554-0x1000:0x8422593-0x1000])<10
assert max(raw[0x84225c4-0x1000:0x84225ef-0x1000])<12
for a,b in sorted(ranges):
    for lo,hi in literal_regions:
        if lo<=a<hi:a=min(b,hi)
        if a<lo<b:b=lo
    if a==b:continue
    ins = list(cs.disasm(raw[a-0x1000:b-0x1000], a))
    assert sum(i.size for i in ins) == b-a, (hex(a), hex(b))
    for i in ins:
        if (i.group(capstone.CS_GRP_JUMP) or i.group(capstone.CS_GRP_CALL)) and i.operands[0].type == capstone.x86.X86_OP_IMM:
            branches.append((i.address,i.operands[0].imm))
bad = [(hex(src),hex(dst),name) for src,dst in branches for a,n,name in hooks if a < dst < a+n]
assert not bad, bad
old_bad = [(hex(src),hex(dst)) for src,dst in branches if 0x081731e9 < dst < 0x081731e9+15]
assert old_bad == [('0x817325d','0x81731f3')], old_bad
result = {'passed':True, 'hook_count':len(hooks), 'function_roots_checked':len(roots),'unwind_fragments_checked':len(ranges),
    'verified_switch_targets':len(switch_targets),'excluded_inline_data':[[hex(a),hex(b)] for a,b in literal_regions],
    'direct_branches_checked':len(branches), 'interior_targets':bad,
    'original_Test03_defect':old_bad,
    'other_original_interior_targets':[(hex(src),hex(dst)) for src,dst in branches if 0x081734c1<dst<0x081734c1+18 or 0x0818c327<dst<0x0818c327+14],
    'scope':'Captured containing functions; direct branches/calls, not arbitrary indirect/external entries',
    'live_game_verified':False}
output=Path(os.environ.get('DRI_TEST_OUTPUT_DIR',S/'build'));output.mkdir(parents=True,exist_ok=True)
(output/'hook_entry_results.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
