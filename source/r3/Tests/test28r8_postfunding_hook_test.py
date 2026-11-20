import gc
"""R8 compiled postfunding hook vs the captured native convergence fragment.

The observer callback is deliberately adversarial/stubbed here. This proves
hook ABI, timing and original control flow, not complete native spending.
At this boundary non-immediate productive actions are allocated for later processing.
Coverage callback/data semantics have separate runtime fixture tests.
"""
from pathlib import Path
import json, os, struct
import capstone, pefile, unicorn
from unicorn.x86_const import *

ROOT = Path(__file__).resolve().parents[1]
PACKAGE = Path(os.environ.get('DRI_PACKAGE_DIR', ROOT.parent))
OUTPUT = Path(os.environ.get('DRI_TEST_OUTPUT_DIR', ROOT/'build'))
CAPTURE = Path(os.environ['DRI_CAPTURE_DIR'])
OUTPUT.mkdir(parents=True, exist_ok=True)
pe = pefile.PE(str(PACKAGE/'DRIChallengerRetention.dll'))
image = pe.get_memory_mapped_image()
capture = (CAPTURE/'section_00_00001000.bin').read_bytes()
GAME = 0x140000000
SITE, RESUME = 0x0817287c, 0x0817288b
original = capture[SITE-0x1000:RESUME-0x1000]
assert original == bytes.fromhex('8b9e20020000488b8ee0000000ffc3')
cs = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
cs.detail = True
assert [(i.mnemonic, i.op_str) for i in cs.disasm(original, GAME+SITE)] == [
    ('mov', 'ebx, dword ptr [rsi + 0x220]'),
    ('mov', 'rcx, qword ptr [rsi + 0xe0]'), ('inc', 'ebx')]
prefix = bytes.fromhex('9c5051525355565741504151415241534154415541564157')
offsets, at = [], 0
while (at := image.find(prefix, at)) >= 0:
    offsets.append(at)
    at += 1
assert len(offsets) == int(os.environ.get('DRI_TEST_EXPECTED_SAVED_HOOK_COUNT', '30'))
offset = offsets[28]
assert original in image[offset:offset+240]
reg_names = ['RAX','RCX','RDX','RBX','RBP','RSI','RDI','R8','R9','R10','R11','R12','R13','R14','R15','RSP']
regs = [globals()['UC_X86_REG_'+n] for n in reg_names]
xmms = [globals()['UC_X86_REG_XMM'+str(i)] for i in range(16)]
results = []

for shift in [0, 0x70000000]:
    base = pe.OPTIONAL_HEADER.ImageBase+shift
    stub = list(cs.disasm(image[offset:offset+240], base+offset))
    call = next(i for i in stub if i.mnemonic == 'call')
    jump = next(i for i in stub if i.mnemonic == 'jmp' and 'rip' in i.op_str)
    callback = call.operands[0].imm
    resume_pointer = jump.address+jump.size+jump.operands[0].mem.disp
    for empty in [False, True]:
        for counter in [0, 17, 0x7fffffff, 0xffffffff]:
            snapshots = []
            for hooked in [False, True]:
                u = unicorn.Uc(unicorn.UC_ARCH_X86, unicorn.UC_MODE_64)
                u.mem_map(base, (len(image)+4095)&~4095)
                u.mem_write(base, image)
                u.mem_map(GAME+0x08171000, 0x3000)
                u.mem_write(GAME+0x08171d10, capture[0x08171d10-0x1000:0x08173d10-0x1000])
                u.mem_map(GAME+0x0862b000, 0x2000)
                noop = capture[0x0862c2d0-0x1000:0x0862c2d3-0x1000]
                assert noop == b'\xc2\0\0'
                u.mem_write(GAME+0x0862c2d0, noop)
                stack, heap = 0x20000000, 0x30000000
                u.mem_map(stack, 0x10000); u.mem_map(heap, 0x10000)
                sp, bp, evaluator, team_ref, team = stack+0x8000, stack+0x8100, heap+0x1000, heap+0x3000, heap+0x4000
                initial = {r:0x11220000+i*0x100 for i,r in enumerate(regs)}
                initial.update({UC_X86_REG_RSP:sp, UC_X86_REG_RBP:bp, UC_X86_REG_RSI:evaluator, UC_X86_REG_R14:0})
                for r,v in initial.items(): u.reg_write(r,v)
                for i,r in enumerate(xmms): u.reg_write(r,(i+1)*0x112233445566778899aabbccddeeff)
                u.reg_write(UC_X86_REG_EFLAGS,0x247)
                u.mem_write(evaluator+0x220,struct.pack('<I',counter))
                u.mem_write(evaluator+0xe0,struct.pack('<Q',heap+0x5000))
                u.mem_write(evaluator+0x1ac,struct.pack('<III',1500,725,50))
                u.mem_write(bp-0x60,struct.pack('<Q',team_ref))
                u.mem_write(team_ref,struct.pack('<Q',team))
                u.mem_write(team,struct.pack('<II',2,20))  # supplied offer/effort witness
                before = bytes(u.mem_read(heap,0x10000))
                seen = []
                if hooked:
                    u.mem_write(GAME+SITE,b'\xff\x25\0\0\0\0'+struct.pack('<Q',base+offset)+b'\x90')
                    u.mem_write(resume_pointer,struct.pack('<Q',GAME+RESUME))
                def intercept(uc,address,size,user):
                    if address == GAME+RESUME:
                        uc.emu_stop(); return
                    if hooked and address == callback:
                        rsp = uc.reg_read(UC_X86_REG_RSP)
                        frame = uc.reg_read(UC_X86_REG_RCX)
                        assert rsp%16 == 8 and frame == sp-128
                        saved = struct.unpack('<16Q',uc.mem_read(frame,128))
                        assert saved[9] == evaluator and saved[10] == bp
                        assert struct.unpack('<Q',uc.mem_read(bp-0x60,8))[0] == team_ref
                        assert struct.unpack('<Q',uc.mem_read(team_ref,8))[0] == team
                        assert struct.unpack('<II',uc.mem_read(team,8)) == (2,20)
                        # The callback runs before native temporary budgets clear.
                        assert struct.unpack('<III',uc.mem_read(evaluator+0x1ac,12)) == (1500,725,50)
                        seen.append({'offer':2,'hours':20,'evaluator':evaluator,'team':team})
                        ret = struct.unpack('<Q',uc.mem_read(rsp,8))[0]
                        for r in regs:
                            if r != UC_X86_REG_RSP: uc.reg_write(r,0xbadc0de)
                        for r in xmms: uc.reg_write(r,0xdeadbeef)
                        uc.reg_write(UC_X86_REG_EFLAGS,0x202)
                        uc.reg_write(UC_X86_REG_RSP,rsp+8);uc.reg_write(UC_X86_REG_RIP,ret)
                emulator_hook=u.hook_add(unicorn.UC_HOOK_CODE,intercept)
                u.emu_start(GAME+(0x0817285e if empty else 0x08172852),GAME+RESUME+1,count=1000)
                assert u.reg_read(UC_X86_REG_RIP) == GAME+RESUME
                assert len(seen) == int(hooked)
                expected_heap = bytearray(before)
                if empty:
                    expected_heap[0x1228:0x122c] = b'\0'*4
                    expected_heap[0x1230:0x1234] = b'\0'*4
                assert bytes(u.mem_read(heap,0x10000)) == bytes(expected_heap)
                snapshots.append(([u.reg_read(r) for r in regs], [u.reg_read(r) for r in xmms], u.reg_read(UC_X86_REG_EFLAGS)))
                u.hook_del(emulator_hook);del u;gc.collect()
            assert snapshots[0] == snapshots[1], ('hook changed native state',empty,counter,shift)
            results.append({'dllBase':hex(base),'emptyVector':empty,'nativeCounter':counter,'passed':True})

report = {'cases':len(results),'results':results,'hookRVA':hex(SITE),'resumeRVA':hex(RESUME),'stolenHex':original.hex(),
          'checks':['captured native instruction boundaries','allocated and empty-vector convergence','observer sees team/evaluator before reset','all GPR/XMM/status flags match native despite callback clobber','native memory unchanged by hook','two image bases','zero/positive/signed-overflow/unsigned-wrap counters'],
          'scope':'Compiled hook with stubbed observer; native allocation and later processing are distinct. Production observation semantics have separate runtime tests. No live-game verification.'}
(OUTPUT/'test28r8_postfunding_hook_results.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report))
