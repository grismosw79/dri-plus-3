import gc
"""Execute compiled Windows hook stubs in x86-64 emulation, with callbacks stubbed.
This verifies ABI preservation/control flow, NOT the game's SDF implementation.
"""
from pathlib import Path
import sys,json,struct,os
ROOT=Path(__file__).resolve().parents[1]
PACKAGE=Path(os.environ.get('DRI_PACKAGE_DIR',ROOT/'package' if (ROOT/'package').exists() else ROOT.parent))
OUTPUT=Path(os.environ.get('DRI_TEST_OUTPUT_DIR',ROOT/'build'));OUTPUT.mkdir(parents=True,exist_ok=True)
CAPTURE=Path(os.environ.get('DRI_CAPTURE_DIR',ROOT.parent/'timing50/native_inputs/DRI_ReadOnly_20261013_221008_7d5d37'))
sys.path[:0]=[str(ROOT/'deps'),str(ROOT.parent/'timing50/deps')]
import pefile,capstone,unicorn
from unicorn.x86_const import *

pe=pefile.PE(str(PACKAGE/'DRIChallengerRetention.dll'))
image=pe.get_memory_mapped_image();preferred=pe.OPTIONAL_HEADER.ImageBase
capture=(CAPTURE/'section_00_00001000.bin').read_bytes()
prefix=bytes.fromhex('9c5051525355565741504151415241534154415541564157')
offsets=[];at=0
while (at:=image.find(prefix,at))>=0:offsets.append(at);at+=1
assert len(offsets)==(30 if (b'DRI_PORTAL_TEST28R9' in image or (b'DRI_PORTAL_TEST28R10' in image or (b'DRI_PORTAL_TEST28R11' in image or (b'DRI_PORTAL_TEST28R12' in image or b'DRI_PORTAL_TEST28R13' in image)))) else 29 if (b'DRI_PORTAL_TEST28R8' in image or (b'DRI_PORTAL_TEST28R9' in image or (b'DRI_PORTAL_TEST28R10' in image or (b'DRI_PORTAL_TEST28R11' in image or (b'DRI_PORTAL_TEST28R12' in image or b'DRI_PORTAL_TEST28R13' in image))))) else 28 if (b'DRI_PORTAL_TEST28R4' in image or b'DRI_PORTAL_TEST28R5' in image or b'DRI_PORTAL_TEST28R6' in image or b'DRI_PORTAL_TEST28R7' in image or (b'DRI_PORTAL_TEST28R8' in image or (b'DRI_PORTAL_TEST28R9' in image or (b'DRI_PORTAL_TEST28R10' in image or (b'DRI_PORTAL_TEST28R11' in image or (b'DRI_PORTAL_TEST28R12' in image or b'DRI_PORTAL_TEST28R13' in image)))))) else 27 if b'DRI_PORTAL_TEST28' in image else 25),offsets
cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);cs.detail=True
specs=[('start',0x08172e29,15),('protect',0x08173151,14),('admit',0x08173514,18)]
reg_names=['RAX','RCX','RDX','RBX','RBP','RSI','RDI','R8','R9','R10','R11','R12','R13','R14','R15','RSP']
regs=[globals()['UC_X86_REG_'+n] for n in reg_names]
results=[]
for shift in [0,0x70000000]:
 base=preferred+shift
 for (name,rva,length),offset in zip(specs,offsets):
  original=capture[rva-0x1000:rva-0x1000+length]
  decoded=list(cs.disasm(original,0x140000000+rva))
  assert sum(x.size for x in decoded)==length
  assert all(x.mnemonic not in ['call','jmp'] and 'rip' not in x.op_str for x in decoded)
  instructions=list(cs.disasm(image[offset:offset+220],base+offset))
  call=next(x for x in instructions if x.mnemonic=='call')
  jumps=[x for x in instructions if x.mnemonic=='jmp' and 'rip' in x.op_str]
  jumps=jumps[:2 if name=='admit' else 1]
  assert len(jumps)==(2 if name=='admit' else 1)
  assert original in image[offset:offset+220]
  for blocked in ([False,True] if name=='admit' else [False]):
   uc=unicorn.Uc(unicorn.UC_ARCH_X86,unicorn.UC_MODE_64)
   uc.mem_map(base,(len(image)+4095)&~4095);uc.mem_write(base,image)
   stack=0x20000000;sp=stack+0x8000;heap=0x30000000;stop=0x40000000
   uc.mem_map(stack,0x10000);uc.mem_map(heap,0x10000);uc.mem_map(stop,0x2000)
   initial={reg:0x11220000+j*0x100 for j,reg in enumerate(regs)}
   initial[UC_X86_REG_RSP]=sp;initial[UC_X86_REG_RBP]=sp+0x100
   initial[UC_X86_REG_RBX]=heap+0x1000
   for reg,value in initial.items():uc.reg_write(reg,value)
   uc.reg_write(UC_X86_REG_EFLAGS,0x247)
   initial_xmm={globals()['UC_X86_REG_XMM'+str(j)]:(j+1)*0x112233445566778899aabbccddeeff for j in range(16)}
   for reg,value in initial_xmm.items():uc.reg_write(reg,value)
   uc.mem_write(sp+0x58,struct.pack('<Q',heap+0x2000));uc.mem_write(heap+0x2000+0xc8,struct.pack('<Q',heap+0x3000))
   uc.mem_write(sp+0x44,b'\x01')
   for j,ins in enumerate(jumps):
    pointer=ins.address+ins.size+ins.operands[0].mem.disp
    uc.mem_write(pointer,struct.pack('<Q',stop+j*0x100))
   callback=call.operands[0].imm;seen=[]
   def intercept(u,address,size,user):
    if address in [stop,stop+0x100]:seen.append(('destination',address));u.emu_stop();return
    if address==callback:
     rsp=u.reg_read(UC_X86_REG_RSP);frame=u.reg_read(UC_X86_REG_RCX)
     assert rsp%16==8 and frame==sp-128
     saved=struct.unpack('<16Q',u.mem_read(frame,128))
     assert saved[10]==initial[UC_X86_REG_RBP] and saved[3]==initial[UC_X86_REG_R12]
     ret=struct.unpack('<Q',u.mem_read(rsp,8))[0]
     for reg in regs:
      if reg!=UC_X86_REG_RSP:u.reg_write(reg,0xbadc0de)
     for reg in initial_xmm:u.reg_write(reg,0xdeadbeef)
     u.reg_write(UC_X86_REG_EFLAGS,0x202)
     if name=='protect':u.mem_write(sp+0x44,b'\x00')
     u.reg_write(UC_X86_REG_RAX,int(blocked));u.reg_write(UC_X86_REG_RSP,rsp+8);u.reg_write(UC_X86_REG_RIP,ret)
     seen.append(('callback',frame))
   emulator_hook=uc.hook_add(unicorn.UC_HOOK_CODE,intercept)
   uc.emu_start(base+offset,stop+0x1000,count=500)
   assert len(seen)==2 and seen[1]==('destination',stop+(0x100 if blocked else 0)),seen
   expected=initial.copy();expected_xmm=initial_xmm.copy();expected_flags=0x247
   if name=='start':
    expected[UC_X86_REG_R13]=0;expected[UC_X86_REG_RDX]=0xe0;expected[UC_X86_REG_RCX]=initial[UC_X86_REG_RBX]
    expected_xmm[UC_X86_REG_XMM0]=0;expected_flags=0x246
   elif name=='protect':
    expected[UC_X86_REG_R8]=initial[UC_X86_REG_R14]+0xd0
    expected[UC_X86_REG_RDX]=initial[UC_X86_REG_R12];expected[UC_X86_REG_RCX]=initial[UC_X86_REG_RBP]-0x60
    assert bytes(uc.mem_read(sp+0x44,1))==b'\0'
   elif not blocked:
    expected[UC_X86_REG_RAX]=heap+0x2000;expected[UC_X86_REG_R8]=initial[UC_X86_REG_RBX]
    expected[UC_X86_REG_RDX]=initial[UC_X86_REG_R12];expected[UC_X86_REG_RCX]=heap+0x3000
   for reg,value in expected.items():assert uc.reg_read(reg)==value,(name,reg,hex(uc.reg_read(reg)),hex(value))
   for reg,value in expected_xmm.items():assert uc.reg_read(reg)==value,(name,reg,'XMM mismatch')
   assert uc.reg_read(UC_X86_REG_EFLAGS)&0x8c5==expected_flags&0x8c5
   results.append({'hook':name,'blocked':blocked,'image_base':hex(base),'passed':True})
   uc.hook_del(emulator_hook);del uc;gc.collect()
out={'compiled_hook_tests':results,'checks':['captured instruction boundaries','exact stolen instruction bytes','16-byte call alignment','shadow space','all general registers','XMM0-XMM15','status flags','resume and admission-skip destinations','two DLL image bases'],'not_tested':['Windows loader execution','live game schema calls','in-game stability','recruiting outcomes']}
(OUTPUT/'hook_test_results.json').write_text(json.dumps(out,indent=2)+'\n')
print(json.dumps(out,indent=2))
