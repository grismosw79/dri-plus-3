import gc
"""Exercise alternate native paths into the relocated allocation/quota hooks."""
from pathlib import Path
import os, struct, json
import pefile, capstone, unicorn
from unicorn.x86_const import *
S=Path(__file__).resolve().parents[1]
raw=(Path(os.environ['DRI_CAPTURE_DIR'])/'section_00_00001000.bin').read_bytes()
pe=pefile.PE(str(S.parent/'DRIChallengerRetention.dll'))
img=pe.get_memory_mapped_image();preferred=pe.OPTIONAL_HEADER.ImageBase
prefix=bytes.fromhex('9c5051525355565741504151415241534154415541564157')
offsets=[];at=0
while (at:=img.find(prefix,at))>=0:offsets.append(at);at+=1
assert len(offsets)==(30 if (b'DRI_PORTAL_TEST28R9' in img or (b'DRI_PORTAL_TEST28R10' in img or (b'DRI_PORTAL_TEST28R11' in img or (b'DRI_PORTAL_TEST28R12' in img or b'DRI_PORTAL_TEST28R13' in img)))) else 29 if b'DRI_PORTAL_TEST28R8' in img else 28 if (b'DRI_PORTAL_TEST28R4' in img or b'DRI_PORTAL_TEST28R5' in img or b'DRI_PORTAL_TEST28R6' in img or b'DRI_PORTAL_TEST28R7' in img) else 27 if b'DRI_PORTAL_TEST28' in img else 25)
cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);cs.detail=True
G=0x140000000;H=0x30000000;SP=0x20010000;BP=SP+0x200
names=['RAX','RCX','RDX','RBX','RBP','RSI','RDI','R8','R9','R10','R11','R12','R13','R14','R15','RSP']
regs=[globals()['UC_X86_REG_'+n] for n in names]
xmms=[globals()['UC_X86_REG_XMM'+str(i)] for i in range(16)]
paths=[('allocation_normal',15,0x081734c1),('allocation_no_array',15,0x08172eef),
       ('allocation_no_board',15,0x08172e6f),('quota_15',10,0x0818c304),
       ('quota_max',10,0x0818c327),('HS_quota_15',29,0x0817bfb1),('HS_quota_max',29,0x0817bfda)]

def run(path,idx,entry,shift,modified,count,patched,old=False):
    base=preferred+shift;u=unicorn.Uc(unicorn.UC_ARCH_X86,unicorn.UC_MODE_64)
    mapped=set()
    def mp(a,n):
        for page in range(a&~4095,(a+n+4095)&~4095,4096):
            if page not in mapped:u.mem_map(page,4096);mapped.add(page)
    def wq(a,v):u.mem_write(a,struct.pack('<Q',v))
    def rq(a):return struct.unpack('<Q',u.mem_read(a,8))[0]
    u.mem_map(base,(len(img)+4095)&~4095);mapped.update(range(base,base+((len(img)+4095)&~4095),4096));u.mem_write(base,img);mp(SP-0x10000,0x20000);mp(H,0x10000)
    for page in [0x08172d10,0x0818bd10,0x0817bd10]:
        mp(G+page,0x1000);u.mem_write(G+page,raw[page-0x1000:page])
    optional=G+0x08103c20;release=H+0xc00
    mp(optional,32);u.mem_write(optional,b'\xc3');u.mem_write(release,b'\xc3')
    for i,r in enumerate(regs):u.reg_write(r,0x11000000+i*0x100)
    values={'RSP':SP,'RBP':BP,'RDI':0,'R13':0,'R14':0,'RCX':1,'RDX':5,
            'R10':count,'R15':H+0xd00,'RSI':0 if path=='allocation_no_board' else H+0xa00}
    for name,v in values.items():u.reg_write(globals()['UC_X86_REG_'+name],v)
    for i,r in enumerate(xmms):u.reg_write(r,(i+1)*0x112233445566778899aabbccddeeff)
    u.reg_write(UC_X86_REG_EFLAGS,0x247)
    wq(H+0xa00,H+0xb00);wq(H+0xb08,release);wq(BP-0x68,H+0xd00)
    start=offsets[idx];end=offsets[idx+1] if idx+1<len(offsets) else start+230
    ins=list(cs.disasm(img[start:end],base+start));cb=next(i.operands[0].imm for i in ins if i.mnemonic=='call')
    jumps=[i for i in ins if i.mnemonic=='jmp' and 'rip' in i.op_str]
    quota=idx in (10,29)
    site,size,resume,alternate=(0x0818c32c,18,0x0818c33e,0x0818c4d1) if quota else (0x081734d3,15,0x081734e2,None)
    if idx==29:site,size,resume,alternate=0x0817bfdf,18,0x0817bff1,0x0817c09d
    for i,dest in zip(jumps,[resume,alternate]):wq(i.address+i.size+i.operands[0].mem.disp,G+dest)
    if not quota:
        i=next(i for i in ins if i.mnemonic=='call' and 'rip' in i.op_str)
        wq(i.address+i.size+i.operands[0].mem.disp,optional)
    old_site,old_size=(0x0818c327,14) if quota else (0x081734c1,18)
    if patched:
        at,n=(old_site,old_size) if old else (site,size)
        u.mem_write(G+at,bytes.fromhex('ff2500000000')+struct.pack('<Q',base+start)+b'\x90'*(n-14))
    calls=[];released=[];out=[]
    def ret(value=0):
        sp=u.reg_read(UC_X86_REG_RSP);rip=rq(sp)
        u.reg_write(UC_X86_REG_RAX,value);u.reg_write(UC_X86_REG_RSP,sp+8);u.reg_write(UC_X86_REG_RIP,rip)
    def apply(frame=None):
        if not modified:return
        if quota:
            value=max(3,rq(frame+13*8) if frame else u.reg_read(UC_X86_REG_RCX))
            if frame:wq(frame+13*8,value)
            else:u.reg_write(UC_X86_REG_RCX,value)
        else:wq(BP-0x80,H+0xf00)
    def trace(uc,a,n,d):
        if old and G+old_site<a<G+old_site+old_size:
            out.append(('DETOUR_PAYLOAD',hex(a-G)));uc.emu_stop();return
        if a in [G+resume,G+alternate if alternate else 0]:out.append(('EXIT',hex(a-G)));uc.emu_stop();return
        if a==G+site and not patched:apply()
        if a==cb and patched:
            sp=u.reg_read(UC_X86_REG_RSP);frame=u.reg_read(UC_X86_REG_RCX);rip=rq(sp)
            assert sp%16==8 and frame==SP-128
            calls.append(a);apply(frame)
            for r in regs:
                if r!=UC_X86_REG_RSP:u.reg_write(r,0xbadc0de)
            for r in xmms:u.reg_write(r,0xdeadbeef)
            u.reg_write(UC_X86_REG_EFLAGS,0x202);u.reg_write(UC_X86_REG_RSP,sp+8);u.reg_write(UC_X86_REG_RIP,rip)
        elif a==optional:ret(count&1)
        elif a==release:released.append(u.reg_read(UC_X86_REG_RCX));ret()
    emulator_hook=u.hook_add(unicorn.UC_HOOK_CODE,trace);u.emu_start(G+entry,0,count=2000)
    assert len(out)==1,(path,out)
    if patched and not old:assert len(calls)==1,(path,calls)
    snapshot={'outcome':out,'regs':[u.reg_read(r) for r in regs],
            'xmms':[u.reg_read(r) for r in xmms],'flags':u.reg_read(UC_X86_REG_EFLAGS)&0x8c5,
            'replacement_end':rq(BP-0x80),'released':released}
    u.hook_del(emulator_hook);del u;gc.collect()
    return snapshot

cases=[]
for path,idx,entry in paths:
    for shift in [0,0x70000000]:
        for modified in [False,True]:
            for count in ([0,2,5,15,16] if idx in (10,29) else [0,1]):
                assert run(path,idx,entry,shift,modified,count,False)==run(path,idx,entry,shift,modified,count,True),(path,shift,modified,count)
                cases.append({'path':path,'shift':hex(shift),'modified':modified,'count':count,'passed':True})
neg=[]
for path,idx,entry in [paths[1],paths[3]]:
    result=run(path,idx,entry,0,False,0,True,old=True)
    assert result['outcome'][0][0]=='DETOUR_PAYLOAD',result
    neg.append({'path':path,'outcome':result['outcome']})
(S/'build/branch_join_results.json').write_text(json.dumps({'passed':True,'cases':cases,'original_negative_controls':neg,'live_game_verified':False},indent=2)+'\n')
print('PASS',len(cases),'alternate-entry cases; both original interior-entry failures reproduced')
