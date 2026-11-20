import gc
"""Run the actual patched prefix-search loop, including its native back-edge.

Use the production DLL stub; only the C callback and SDF access are mocked.
The original Test03 detour is a negative control that must enter its payload.
"""
from pathlib import Path
import os, struct, json
import pefile, capstone, unicorn
from unicorn.x86_const import *

S=Path(__file__).resolve().parents[1]
C=Path(os.environ['DRI_CAPTURE_DIR'])
raw=(C/'section_00_00001000.bin').read_bytes()
pe=pefile.PE(str(S.parent/'DRIChallengerRetention.dll'))
img=pe.get_memory_mapped_image(); preferred=pe.OPTIONAL_HEADER.ImageBase
prefix=bytes.fromhex('9c5051525355565741504151415241534154415541564157')
offsets=[]; at=0
while (at:=img.find(prefix,at))>=0: offsets.append(at); at+=1
assert len(offsets)==(30 if (b'DRI_PORTAL_TEST28R9' in img or (b'DRI_PORTAL_TEST28R10' in img or (b'DRI_PORTAL_TEST28R11' in img or (b'DRI_PORTAL_TEST28R12' in img or b'DRI_PORTAL_TEST28R13' in img)))) else 29 if b'DRI_PORTAL_TEST28R8' in img else 28 if (b'DRI_PORTAL_TEST28R4' in img or b'DRI_PORTAL_TEST28R5' in img or b'DRI_PORTAL_TEST28R6' in img or b'DRI_PORTAL_TEST28R7' in img) else 27 if b'DRI_PORTAL_TEST28' in img else 25)
start=offsets[14]
cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);cs.detail=True
GAME=0x140000000; STACK=0x20000000; HEAP=0x30000000
names=['RAX','RCX','RDX','RBX','RBP','RSI','RDI','R8','R9','R10','R11','R12','R13','R14','R15','RSP']
regs=[globals()['UC_X86_REG_'+n] for n in names]
xmms=[globals()['UC_X86_REG_XMM'+str(i)] for i in range(16)]

def run(count,match,shift,patched=True,old=False,changed=True):
    base=preferred+shift
    u=unicorn.Uc(unicorn.UC_ARCH_X86,unicorn.UC_MODE_64)
    u.mem_map(base,(len(img)+4095)&~4095);u.mem_write(base,img)
    u.mem_map(GAME+0x08172d10,0x1000)
    u.mem_write(GAME+0x08172d10,raw[0x08171d10:0x08172d10])
    u.mem_map(GAME+0x08051000,0x2000);u.mem_write(GAME+0x08055d00,b"\xc3")
    u.mem_map(STACK,0x20000);u.mem_map(HEAP,0x20000)
    def wq(a,v):u.mem_write(a,struct.pack('<Q',v))
    def rq(a):return struct.unpack('<Q',u.mem_read(a,8))[0]
    sp=STACK+0x10000;bp=sp+0x200;vec=HEAP+0x100;arr=HEAP+0x1000
    recs=[HEAP+0x4000+i*64 for i in range(max(count,1))]
    target=HEAP+0x8000 if match is None else recs[match]
    vt=HEAP+0x200;tab=HEAP+0x300;target_record=HEAP+0x400
    get=HEAP+0xe000;release=HEAP+0xe100
    u.mem_write(get,b"\xc3");u.mem_write(release,b"\xc3")
    wq(vt+0x128,get);wq(vt+8,release);wq(tab,vt)
    wq(target_record+0x10,tab);wq(bp-0x38,target_record);wq(bp-0x30,HEAP+0x900)
    wq(bp-0x68,vec);wq(vec,arr);wq(sp+0x68,count)
    for i,r in enumerate(recs):wq(arr+i*48,r);wq(r,vt)
    wq(target,vt)
    for j,r in enumerate(regs):u.reg_write(r,0x11000000+j*0x100)
    for r,v in [(UC_X86_REG_RSP,sp),(UC_X86_REG_RBP,bp),(UC_X86_REG_RSI,count),(UC_X86_REG_R13,0)]:u.reg_write(r,v)
    for i,r in enumerate(xmms):u.reg_write(r,(i+1)*0x112233445566778899aabbccddeeff)
    u.reg_write(UC_X86_REG_EFLAGS,0x247)
    ins=list(cs.disasm(img[start:offsets[15]],base+start))
    cb=next(i.operands[0].imm for i in ins if i.mnemonic=='call')
    jumps=[i for i in ins if i.mnemonic=='jmp' and 'rip' in i.op_str]
    for i,dst in zip(jumps,[0x08173201,0x0817325f]):wq(i.address+i.size+i.operands[0].mem.disp,GAME+dst)
    if patched:
        site=0x081731e9 if old else 0x081731f3
        # Old detour uses an equivalent tiny setup stub for the negative
        # control, then executes the unmodified production loop body.
        dest=HEAP+0xf000 if old else base+start
        if old:
            setup=raw[0x081731e9-0x1000:0x081731f3-0x1000]
            # cmp r15,rax; jae absolute-skip thunk; otherwise absolute resume.
            stub=setup+bytes.fromhex('4c3bf8730e')+bytes.fromhex('ff2500000000')+struct.pack('<Q',GAME+0x081731f8)+bytes.fromhex('ff2500000000')+struct.pack('<Q',GAME+0x0817325f)
            u.mem_write(dest,stub)
        u.mem_write(GAME+site,bytes.fromhex('ff2500000000')+struct.pack('<Q',dest)+(b'\x90' if old else b''))
    visited=[];calls=[];refs=0;outcome=[]
    def ret(value=0):
        rsp=u.reg_read(UC_X86_REG_RSP);rip=rq(rsp)
        u.reg_write(UC_X86_REG_RAX,value);u.reg_write(UC_X86_REG_RSP,rsp+8);u.reg_write(UC_X86_REG_RIP,rip)
    def apply():
        if changed:u.mem_write(sp+0x40,b'\1');u.mem_write(sp+0x44,b'\1')
    def trace(uc,a,n,d):
        nonlocal refs
        if old and GAME+0x081731e9<a<GAME+0x081731f8:
            outcome.append(('DETOUR_PAYLOAD',hex(a-GAME)));uc.emu_stop();return
        if a in [GAME+0x0817325f,GAME+0x08173296]:
            outcome.append(('MATCH' if a==GAME+0x08173296 else 'NO_MATCH',u.reg_read(UC_X86_REG_R15)));uc.emu_stop();return
        if a==GAME+0x081731f3:
            visited.append(u.reg_read(UC_X86_REG_R15))
            if not patched:apply()
        if a==cb and patched and not old:
            assert u.reg_read(UC_X86_REG_RSP)%16==8
            frame=u.reg_read(UC_X86_REG_RCX);assert frame==sp-128
            calls.append(rq(frame));apply()
            rsp=u.reg_read(UC_X86_REG_RSP);rip=rq(rsp)
            for r in regs:
                if r!=UC_X86_REG_RSP:u.reg_write(r,0xbadc0de)
            for r in xmms:u.reg_write(r,0xdeadbeef)
            u.reg_write(UC_X86_REG_EFLAGS,0x202)
            u.reg_write(UC_X86_REG_RSP,rsp+8);u.reg_write(UC_X86_REG_RIP,rip)
        elif a==GAME+0x08055d00:ret(HEAP+0xa00)
        elif a==get:wq(u.reg_read(UC_X86_REG_RDX),target);refs+=1;ret()
        elif a==release:refs-=1;assert refs==0;ret()
    emulator_hook=u.hook_add(unicorn.UC_HOOK_CODE,trace)
    try:u.emu_start(GAME+0x081731e9,0,count=30000)
    except unicorn.UcError:
        print("FAULT",count,match,shift,patched,old,hex(u.reg_read(UC_X86_REG_RIP)),visited);raise
    assert len(outcome)==1,outcome
    assert refs==0
    if not old:
        assert outcome[0][0]==('MATCH' if match is not None else 'NO_MATCH'),outcome
        assert len(visited)==(match+1 if match is not None else max(1,count))
        if patched:assert calls==visited
    snapshot={'outcome':outcome,'visits':visited,'registers':[u.reg_read(r) for r in regs],
            'xmm':[u.reg_read(r) for r in xmms],'flags':u.reg_read(UC_X86_REG_EFLAGS)&0x8c5,
            'stack':bytes(u.mem_read(sp+0x40,8)).hex(),'bound':rq(bp+0x40)}
    u.hook_del(emulator_hook);del u;gc.collect()
    return snapshot

results=[]
for shift in [0,0x70000000]:
    for changed in [False,True]:
        for count in [0,1,3,50]:
            for match in ([None] if not count else sorted(set([0,count//2,count-1]))+[None]):
                expected=run(count,match,shift,patched=False,changed=changed)
                actual=run(count,match,shift,changed=changed)
                assert expected==actual,(count,match,shift,changed,expected,actual)
                results.append({'count':count,'match_index':match,'modified':changed,'shift':hex(shift),'passed':True})
negative=run(3,None,0,old=True)
assert negative['outcome']==[('DETOUR_PAYLOAD','0x81731f3')],negative
result={'passed':True,'cases':results,'original_Test03_negative_control':negative['outcome'],
        'scope':'Captured native prefix loop plus production detour/stub; callback and SDF mocked',
        'live_game_verified':False}
(S/'build/retention_loop_results.json').write_text(json.dumps(result,indent=2)+'\n')
print('PASS',len(results),'prefix-loop cases; original Test03 enters overwritten payload')
