import gc
"""Compare all six Test02 stubs against captured instructions in two address layouts."""
from pathlib import Path
import json,os,struct
import pefile,capstone,unicorn
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'build';GB=0x140000000
P=pefile.PE(str(ROOT.parent/'DRIChallengerRetention.dll'));IMG=P.get_memory_mapped_image();PB=P.OPTIONAL_HEADER.ImageBase
raw=(Path(os.environ['DRI_CAPTURE_DIR'])/'section_00_00001000.bin').read_bytes()
prefix=bytes.fromhex('9c5051525355565741504151415241534154415541564157');offsets=[];at=0
while (at:=IMG.find(prefix,at))>=0:offsets.append(at);at+=1
assert len(offsets)==(30 if (b'DRI_PORTAL_TEST28R9' in IMG or (b'DRI_PORTAL_TEST28R10' in IMG or (b'DRI_PORTAL_TEST28R11' in IMG or (b'DRI_PORTAL_TEST28R12' in IMG or b'DRI_PORTAL_TEST28R13' in IMG)))) else 29 if b'DRI_PORTAL_TEST28R8' in IMG else 28 if (b'DRI_PORTAL_TEST28R4' in IMG or b'DRI_PORTAL_TEST28R5' in IMG or b'DRI_PORTAL_TEST28R6' in IMG or b'DRI_PORTAL_TEST28R7' in IMG or b'DRI_PORTAL_TEST28R8' in IMG) else 27 if b'DRI_PORTAL_TEST28' in IMG else 25)
cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);cs.detail=True
specs=[('Prepare',0x08172d0f,20,0x08172d23,None),('Score',0x08185ad7,20,0x08185aeb,None),('Quota',0x0818c32c,18,0x0818c33e,0x0818c4d1),('Select',0x081acfff,15,0x081ad00e,0x081ad0a4),('FallbackStart',0x081ac8c2,14,0x081ac8d0,None),('Fallback',0x081aca14,15,0x081aca23,0x081acc18)]
names=['RAX','RCX','RDX','RBX','RBP','RSI','RDI','R8','R9','R10','R11','R12','R13','R14','R15','RSP'];regs=[globals()['UC_X86_REG_'+n] for n in names]
xmms=[globals()['UC_X86_REG_XMM'+str(i)] for i in range(16)];results=[]
for shift in [0,0x70000000]:
 base=PB+shift
 for idx,(name,rva,size,resume,alternate) in enumerate(specs,8):
  code=raw[rva-0x1000:rva-0x1000+size];assert sum(i.size for i in cs.disasm(code,GB+rva))==size
  start=offsets[idx];end=offsets[idx+1] if idx+1<len(offsets) else start+230
  ins=list(cs.disasm(IMG[start:end],base+start));callback=next(i.operands[0].imm for i in ins if i.mnemonic=='call')
  jumps=[i for i in ins if i.mnemonic=='jmp' and 'rip' in i.op_str]
  for changed in [False,True]:
   for count in ([0,2,4,7] if name=='Quota' else [0]):
    snapshots=[]
    for use_hook in [False,True]:
     u=unicorn.Uc(unicorn.UC_ARCH_X86,unicorn.UC_MODE_64);mapped=set()
     def mp(a,n=1):
      for pg in range(a&~4095,(a+n+4095)&~4095,4096):
       if pg not in mapped:u.mem_map(pg,4096);mapped.add(pg)
     def wq(a,v):u.mem_write(a,struct.pack('<Q',v))
     def rq(a):return struct.unpack('<Q',u.mem_read(a,8))[0]
     sp=0x20008000;bp=sp+0x200;heap=0x30000000
     u.mem_map(base,(len(IMG)+4095)&~4095);mapped.update(range(base,base+((len(IMG)+4095)&~4095),4096));u.mem_write(base,IMG);mp(sp-0x8000,0x10000);mp(heap,0x10000)
     for loc in [rva,resume,alternate]:
      if loc is not None:mp(GB+loc,32)
     u.mem_write(GB+rva,code)
     u.mem_write(GB+resume,b"\xcc")
     if alternate:u.mem_write(GB+alternate,b"\xcc")
     initial={r:0x11000000+i*0x100 for i,r in enumerate(regs)}
     initial.update({UC_X86_REG_RSP:sp,UC_X86_REG_RBP:bp,UC_X86_REG_RAX:1,UC_X86_REG_RCX:1,UC_X86_REG_RDX:0,UC_X86_REG_RSI:2,UC_X86_REG_R10:count,UC_X86_REG_R12:0,UC_X86_REG_R14:522,UC_X86_REG_R15:heap+0x200})
     wq(sp+0x98,0xabcdef);wq(bp-0x39,heap+0x400);wq(bp-0x69,heap+0x600);wq(heap+0x610,heap+0x700)
     if name=='FallbackStart':initial[UC_X86_REG_RCX]=heap+0x800;initial[UC_X86_REG_RAX]=sp+0x108;wq(heap+0x8c8,heap+0x900)
     for r,v in initial.items():u.reg_write(r,v)
     for i,r in enumerate(xmms):u.reg_write(r,(i+1)*0x112233445566778899aabbccddeeff)
     u.reg_write(UC_X86_REG_EFLAGS,0x247)
     def apply_change(frame=None):
      if not changed:return
      key,val={'Score':('R14',767),'Quota':('RCX',3),'Select':('RAX',0)}.get(name,(None,None))
      if key:
       if frame is None:u.reg_write(globals()['UC_X86_REG_'+key],val)
       else:wq(frame+{'R14':1,'RDX':12,'RCX':13,'RAX':14}[key]*8,val)
     for j,i in enumerate(jumps):wq(i.address+i.size+i.operands[0].mem.disp,GB+(resume if j==0 else alternate))
     destinations=[]
     def trace(u,a,n,d):
      if a in [GB+resume,GB+alternate if alternate else 0]:destinations.append(a);u.emu_stop();return
      if use_hook and a==callback:
       rsp=u.reg_read(UC_X86_REG_RSP);frame=u.reg_read(UC_X86_REG_RCX);assert rsp%16==8 and frame==sp-128
       ret=rq(rsp);apply_change(frame)
       for r in regs:
        if r!=UC_X86_REG_RSP:u.reg_write(r,0xbadc0de)
       for r in xmms:u.reg_write(r,0xdeadbeef)
       u.reg_write(UC_X86_REG_EFLAGS,0x202);u.reg_write(UC_X86_REG_RAX,int(changed and name=='Fallback'))
       u.reg_write(UC_X86_REG_RSP,rsp+8);u.reg_write(UC_X86_REG_RIP,ret)
     emulator_hook=u.hook_add(unicorn.UC_HOOK_CODE,trace)
     if not use_hook:apply_change()
     if not use_hook and name=='Fallback' and changed:destinations.append(GB+alternate)
     else:
      try:u.emu_start(base+start if use_hook else GB+rva,0,count=2000)
      except unicorn.UcError:
       print('FAULT',name,changed,count,use_hook,hex(u.reg_read(UC_X86_REG_RIP)),[(i.mnemonic,i.op_str) for i in jumps]);raise
     assert len(destinations)==1,(name,destinations)
     snapshots.append(([u.reg_read(r) for r in regs],[u.reg_read(r) for r in xmms],u.reg_read(UC_X86_REG_EFLAGS)&0x8c5,bytes(u.mem_read(heap+0x228,8)),bytes(u.mem_read(bp+0x6f,8)),bytes(u.mem_read(sp+0x20,16)),destinations))
     u.hook_del(emulator_hook);del u;gc.collect()
    assert snapshots[0]==snapshots[1],(name,changed,count,snapshots)
    results.append({'hook':name,'modified':changed,'count':count,'base':hex(base),'pass':True})
(OUT/'quality_hook_results.json').write_text(json.dumps({'cases':results,'live_game_verified':False,'checks':['exact instruction boundaries','register/XMM/flags preservation','callback stack alignment','both branch destinations','candidate score writes','temporary argument/local memory','two DLL address layouts']},indent=2)+'\n')
print('PASS',len(results),'new hook machine-state cases')
