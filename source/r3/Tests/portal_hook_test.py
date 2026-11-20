import gc
"""Compare new compiled stubs with captured instructions, including both branches."""
from pathlib import Path
import os,struct,json
import pefile,capstone,unicorn
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1];PACKAGE=ROOT.parent;OUT=ROOT/'build'
P=pefile.PE(str(PACKAGE/'DRIChallengerRetention.dll'));IMG=P.get_memory_mapped_image();PB=P.OPTIONAL_HEADER.ImageBase
CAP=Path(os.environ['DRI_CAPTURE_DIR']);raw=(CAP/'section_00_00001000.bin').read_bytes();GB=0x140000000
prefix=bytes.fromhex('9c5051525355565741504151415241534154415541564157');offsets=[];i=0
while (i:=IMG.find(prefix,i))>=0:offsets.append(i);i+=1
assert len(offsets)==(30 if (b'DRI_PORTAL_TEST28R9' in IMG or (b'DRI_PORTAL_TEST28R10' in IMG or (b'DRI_PORTAL_TEST28R11' in IMG or (b'DRI_PORTAL_TEST28R12' in IMG or b'DRI_PORTAL_TEST28R13' in IMG)))) else 29 if b'DRI_PORTAL_TEST28R8' in IMG else 28 if (b'DRI_PORTAL_TEST28R4' in IMG or b'DRI_PORTAL_TEST28R5' in IMG or b'DRI_PORTAL_TEST28R6' in IMG or b'DRI_PORTAL_TEST28R7' in IMG or b'DRI_PORTAL_TEST28R8' in IMG) else 27 if b'DRI_PORTAL_TEST28' in IMG else 25)
cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);cs.detail=True
names=['RAX','RCX','RDX','RBX','RBP','RSI','RDI','R8','R9','R10','R11','R12','R13','R14','R15','RSP']
regs=[globals()['UC_X86_REG_'+x] for x in names];xmms=[globals()['UC_X86_REG_XMM'+str(i)] for i in range(16)]
specs=[('stars',0x861684c,15,0x861685b,0x861685b,0x8616190),('weekly',0x81b9d30,18,0x81b9d42,0x81b9e39,None),('instant',0x815673e,18,0x8156750,0x81569f8,0x8055d00),('signing',0x81b4626,14,0x81b4634,0x81b494e,0x8057470),('deadline',0x81b5916,18,0x81b5928,0x81b5ea5,0x70000000),('hs_owner',0x81b98ed,15,0x81b98fc,0x81b9a01,None),('bulk_owner',0x81630c1,18,0x81630d3,0x81630d3,0x8055d00)]
results=[]
for shift in [0,0x70000000]:
 base=PB+shift
 for (name,rva,size,resume,skip,helper),idx in zip(specs,offsets[3:8]+[offsets[16],offsets[17]]):
  original=raw[rva-0x1000:rva-0x1000+size];assert sum(x.size for x in cs.disasm(original,GB+rva))==size
  end=next((x for x in offsets if x>idx),idx+300);instructions=list(cs.disasm(IMG[idx:end],base+idx))
  callback=next(x.operands[0].imm for x in instructions if x.mnemonic=='call')
  indirect_jumps=[x for x in instructions if x.mnemonic=='jmp' and 'rip' in x.op_str]
  indirect_calls=[x for x in instructions if x.mnemonic=='call' and 'rip' in x.op_str]
  for blocked in ([False] if name=='bulk_owner' else [False,True]):
   for year in ([0,2] if name=='stars' and not blocked else [0,1] if name=='hs_owner' else [0]):
    snapshots=[]
    for actual_hook in [False,True]:
     u=unicorn.Uc(unicorn.UC_ARCH_X86,unicorn.UC_MODE_64)
     mapped=set()
     def mp(a,n=1):
      for pg in range(a&~4095,(a+n+4095)&~4095,4096):
       if pg not in mapped:u.mem_map(pg,4096);mapped.add(pg)
     def wq(a,v):u.mem_write(a,struct.pack('<Q',v))
     def rq(a):return struct.unpack('<Q',u.mem_read(a,8))[0]
     u.mem_map(base,(len(IMG)+4095)&~4095);mapped.update(range(base,base+((len(IMG)+4095)&~4095),4096));u.mem_write(base,IMG)
     sp=0x20008000;bp=sp+0x200;heap=0x30000000;mp(sp-0x8000,0x10000);mp(heap,0x10000)
     for r in [rva,resume,skip]:mp(GB+r,32)
     u.mem_write(GB+rva,original)
     if helper:
      hp=GB+helper if helper<0x10000000 else helper
      mp(hp,16);u.mem_write(hp,b'\xc3')
     init={r:0x11000000+j*0x100 for j,r in enumerate(regs)}
     init.update({UC_X86_REG_RSP:sp,UC_X86_REG_RBP:bp,UC_X86_REG_RAX:4 if name=='stars' else 1,UC_X86_REG_RDI:year,UC_X86_REG_R14:heap+0x100,UC_X86_REG_R15:heap+0x200,UC_X86_REG_R13:heap+0x300})
     if name=='hs_owner':init[UC_X86_REG_RBX]=(init[UC_X86_REG_RBX]&~255)|year
     wq(sp+0x30,heap+0x400);wq(sp+0x38,heap+0x500);wq(sp+0x40,heap+0x600)
     wq(bp-0x11,heap+0xa00);wq(bp-0x19,heap+0xb00);wq(bp-0x39,heap+0x700);wq(bp-0x41,heap+0x800);wq(heap+0x110,12)
     wq(heap+0x200,heap+0x900);wq(heap+0x900,helper or 0)
     for reg,val in init.items():u.reg_write(reg,val)
     for k,reg in enumerate(xmms):u.reg_write(reg,(k+1)*0x112233445566778899aabbccddeeff)
     u.reg_write(UC_X86_REG_EFLAGS,0x247)
     # Relocated absolute pointer slots are supplied by DriInitialize in production.
     for j,x in enumerate(indirect_jumps):
      dest=GB+(resume if j==0 or name=='stars' else skip)
      wq(x.address+x.size+x.operands[0].mem.disp,dest)
     for x in indirect_calls:wq(x.address+x.size+x.operands[0].mem.disp,GB+helper)
     calls=[];dest=[]
     def hook(uc,a,sz,data):
      if a in [GB+resume,GB+skip]:dest.append(a);uc.emu_stop();return
      rsp=uc.reg_read(UC_X86_REG_RSP)
      if actual_hook and a==callback:
       frame=uc.reg_read(UC_X86_REG_RCX);assert frame==sp-128 and rsp%16==8
       ret=rq(rsp)
       if name=='weekly' and blocked:wq(frame+14*8,0)
       for reg in regs:
        if reg!=UC_X86_REG_RSP:uc.reg_write(reg,0xbadc0de)
       for reg in xmms:uc.reg_write(reg,0xdeadbeef)
       uc.reg_write(UC_X86_REG_EFLAGS,0x202);uc.reg_write(UC_X86_REG_RAX,int(blocked))
       uc.reg_write(UC_X86_REG_RSP,rsp+8);uc.reg_write(UC_X86_REG_RIP,ret);return
      hp=GB+helper if helper is not None and helper<0x10000000 else helper
      if helper is not None and a==hp:
       assert rsp%16==8
       calls.append(tuple(uc.reg_read(reg) for reg in [UC_X86_REG_RCX,UC_X86_REG_RDX,UC_X86_REG_R8]))
       ret=rq(rsp);uc.reg_write(UC_X86_REG_RAX,0x6789);uc.reg_write(UC_X86_REG_RCX,0x112233);uc.reg_write(UC_X86_REG_RDX,0x445566)
       uc.reg_write(UC_X86_REG_RSP,rsp+8);uc.reg_write(UC_X86_REG_RIP,ret)
     emulator_hook=u.hook_add(unicorn.UC_HOOK_CODE,hook)
     if not actual_hook and blocked:
      if name=='weekly':u.reg_write(UC_X86_REG_RAX,0)
      else:
       # Active skip leaves captured caller state; it intentionally omits stolen instructions.
       snapshots.append((list(init.values()),[u.reg_read(x) for x in xmms],0x247,[],GB+skip,0))
       u.hook_del(emulator_hook);del u;gc.collect();continue
     try:u.emu_start(base+idx if actual_hook else GB+rva,0,count=1000)
     except unicorn.UcError:
      print("FAIL",name,blocked,actual_hook,hex(u.reg_read(UC_X86_REG_RIP)),"helper",helper,"calls",calls,"globals",[(hex(x.address),x.op_str) for x in indirect_calls]);raise
     assert dest
     snapshots.append(([u.reg_read(r) for r in regs],[u.reg_read(r) for r in xmms],u.reg_read(UC_X86_REG_EFLAGS)&0x8c5,calls,dest[0],rq(sp+0x70)))
     u.hook_del(emulator_hook);del u;gc.collect()
    a,b=snapshots
    assert a[:2]==b[:2],(name,blocked,year,'registers',a[:2],b[:2])
    assert (a[2]&0x8c5)==(b[2]&0x8c5),(name,'flags')
    assert a[3:]==b[3:],(name,blocked,'flow',a[3:],b[3:])
    results.append({'hook':name,'active_skip':blocked,'grade':year,'initial_bl':year if name=='hs_owner' else None,'base_shift':hex(shift),'pass':True})
out={'cases':results,'method':'compiled hook versus captured instructions, two DLL addresses, register/vector/flags/call-alignment and native cleanup destinations','live_game_verified':False}
(OUT/'portal_hook_results.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out,indent=2))
