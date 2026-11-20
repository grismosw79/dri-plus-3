import gc
"""Compare every Test11 trampoline with the corresponding captured instructions.

Callbacks and native helper services are deliberately destructive ABI fixtures.
Actual policy callbacks are exercised separately by the runtime tests.
"""
from pathlib import Path
import os,struct,json,re
import pefile,capstone,unicorn
from unicorn.x86_const import *

ROOT=Path(__file__).resolve().parents[1]
PACKAGE=Path(os.environ.get('DRI_PACKAGE_DIR',ROOT.parent))
OUT=Path(os.environ.get('DRI_TEST_OUTPUT_DIR',ROOT/'build'));OUT.mkdir(parents=True,exist_ok=True)
P=pefile.PE(str(PACKAGE/'DRIChallengerRetention.dll'))
IMG=P.get_memory_mapped_image();PB=P.OPTIONAL_HEADER.ImageBase
raw=(Path(os.environ['DRI_CAPTURE_DIR'])/'section_00_00001000.bin').read_bytes()
GB=0x140000000;RELEASE=0x70000000
prefix=bytes.fromhex('9c5051525355565741504151415241534154415541564157')
offsets=[];at=0
while (at:=IMG.find(prefix,at))>=0:offsets.append(at);at+=1
hook_names=re.findall(r'^\.global\s+(Hook\w+)',(ROOT/'hooks.S').read_text(),re.M)
# Function-entry wrappers do not begin with SAVE_STATE; PP stubs remain appended.
test28=b'DRI_PORTAL_TEST28' in IMG
entry_wrappers={'HookEpsGrade','HookEpsGeneric','HookEpsRequired','HookEpsUpdate'}
assert {n for n in hook_names if n.startswith('HookEps')}==entry_wrappers
if test28:
 test28_entries={'HookStaff28Update','HookPt28Grade','HookProximity28Grade','HookPPGroupGrade'}
 assert test28_entries<=set(hook_names)
 entry_wrappers|=test28_entries
if b'DRI_PORTAL_TEST28R13' in IMG:
 assert 'HookHsShortlist' in hook_names
 entry_wrappers.add('HookHsShortlist')
hook_names=[n for n in hook_names if n not in entry_wrappers]
assert len(offsets)==len(hook_names)==(30 if (b'DRI_PORTAL_TEST28R9' in IMG or (b'DRI_PORTAL_TEST28R10' in IMG or (b'DRI_PORTAL_TEST28R11' in IMG or (b'DRI_PORTAL_TEST28R12' in IMG or b'DRI_PORTAL_TEST28R13' in IMG)))) else 29 if b'DRI_PORTAL_TEST28R8' in IMG else 28 if test28 else 25)
cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);cs.detail=True
names='RAX RCX RDX RBX RBP RSI RDI R8 R9 R10 R11 R12 R13 R14 R15 RSP'.split()
regs=[globals()['UC_X86_REG_'+name] for name in names]
xmms=[globals()['UC_X86_REG_XMM'+str(i)] for i in range(16)]
specs=[
 ('HookNsdCollect',0x081a2ddd,16,0x081a2ded,0x081a2f64,None,'4d85e40f847e0100000fb69dc0000000'),
 ('HookNsdResolve',0x081a2f90,17,0x081a2fa1,0x081a3441,None,'4533f64c897424484d85e40f84a0040000'),
 ('HookPortalFunding',0x08171eda,19,0x08171eed,0x0817285e,None,'4533f64c8d3d645f35034d85e40f8471090000'),
 ('HookBulkGuard',0x08162f6f,18,0x08162f81,0x08163108,GB+0x08055d00,'488b4defba09000000488b5de7e87f2defff'),
 ('HookCleanupProtection',0x081a7dae,15,0x081a7dbd,0x081a7df9,None,'84c07447488b4def488d1d8b003203'),
 ('HookPostWeekly',0x081b9f5a,19,0x081b9f6d,0x081b9f6d,None,'4c8b6d60488d55684c897d68498b8da0000000'),
 ('HookPostInstant',0x081569b7,14,0x081569c5,0x081569c5,RELEASE,'4d85ff7409498b07498bcfff5008'),
 ('HookPostNsd',0x081a3441,16,0x081a3451,0x081a3451,None,'488d4de841c6070041c685a101000000'),
]
results=[]
for shift in [0,0x70000000]:
 base=PB+shift
 for name,rva,length,resume,skip,helper,signature in specs:
  original=raw[rva-0x1000:rva-0x1000+length]
  assert original.hex()==signature
  assert sum(i.size for i in cs.disasm(original,GB+rva))==length
  index=hook_names.index(name);offset=offsets[index]
  end=offsets[index+1] if index+1<len(offsets) else offset+600
  instructions=list(cs.disasm(IMG[offset:end],base+offset))
  callback=next(i.operands[0].imm for i in instructions if i.mnemonic=='call')
  jumps=[i for i in instructions if i.mnemonic=='jmp' and 'rip' in i.op_str]
  indirect_calls=[i for i in instructions if i.mnemonic=='call' and 'rip' in i.op_str]
  variants=[(False,False,1)]
  if name=='HookBulkGuard':variants=[(False,False,1),(True,False,1)]
  if name in ('HookNsdResolve','HookNsdCollect'):variants=[(False,False,0),(False,False,1)]
  if name=='HookPostInstant':variants=[(False,False,0),(False,False,1)]
  if name=='HookCleanupProtection':variants=[(False,False,0),(False,False,1),(False,True,1)]
  for blocked,force_allow,nonnull in variants:
   snapshots=[]
   for compiled in [False,True]:
    u=unicorn.Uc(unicorn.UC_ARCH_X86,unicorn.UC_MODE_64);mapped=set()
    def mp(address,size=1):
     for page in range(address&~4095,(address+size+4095)&~4095,4096):
      if page not in mapped:u.mem_map(page,4096);mapped.add(page)
    def wq(address,value):u.mem_write(address,struct.pack('<Q',value))
    def rq(address):return struct.unpack('<Q',u.mem_read(address,8))[0]
    u.mem_map(base,(len(IMG)+4095)&~4095);mapped.update(range(base,base+((len(IMG)+4095)&~4095),4096));u.mem_write(base,IMG)
    sp=0x20008000;bp=sp+0x200;heap=0x30000000
    mp(sp-0x8000,0x10000);mp(heap,0x10000)
    for destination in [rva,resume,skip]:mp(GB+destination,32)
    u.mem_write(GB+rva,original)
    if helper:mp(helper,16);u.mem_write(helper,b'\xc3')
    initial={reg:0x11000000+j*0x100 for j,reg in enumerate(regs)}
    initial.update({UC_X86_REG_RSP:sp,UC_X86_REG_RBP:bp,UC_X86_REG_R15:heap+0x200,UC_X86_REG_R13:heap+0x300})
    if name in ('HookNsdResolve','HookNsdCollect'):initial[UC_X86_REG_R12]=nonnull
    if name=='HookPostInstant' and not nonnull:initial[UC_X86_REG_R15]=0
    if name=='HookCleanupProtection':initial[UC_X86_REG_RAX]=0x1122334455667700|nonnull
    wq(bp-0x11,heap+0xa00);wq(bp-0x19,heap+0xb00)
    wq(bp+0x60,heap+0x1000);wq(heap+0x10a0,heap+0x1100)
    wq(heap+0x200,heap+0x900);wq(heap+0x908,RELEASE)
    u.mem_write(heap+0x4a1,b'\x01')
    for reg,value in initial.items():u.reg_write(reg,value)
    for j,reg in enumerate(xmms):u.reg_write(reg,(j+1)*0x112233445566778899aabbccddeeff)
    u.reg_write(UC_X86_REG_EFLAGS,0x247)
    if not compiled and force_allow:u.reg_write(UC_X86_REG_RAX,initial[UC_X86_REG_RAX]&~255)
    for j,instruction in enumerate(jumps):
     destination=resume if j==0 else skip
     wq(instruction.address+instruction.size+instruction.operands[0].mem.disp,GB+destination)
    for instruction in indirect_calls:
     assert helper is not None
     wq(instruction.address+instruction.size+instruction.operands[0].mem.disp,helper)
    if name in ['HookCleanupProtection','HookPortalFunding']:
     vtable_load=next(i for i in instructions if i.mnemonic=='mov' and i.op_str.startswith('rbx,' if name=='HookCleanupProtection' else 'r15,') and 'rip' in i.op_str)
     wq(vtable_load.address+vtable_load.size+vtable_load.operands[1].mem.disp,GB+0x0b4c7e48)
    destinations=[];helper_calls=[];callbacks=[]
    def intercept(uc,address,size,data):
     if address in [GB+resume,GB+skip]:destinations.append(address);uc.emu_stop();return
     rsp=uc.reg_read(UC_X86_REG_RSP)
     if compiled and address==callback:
      frame=uc.reg_read(UC_X86_REG_RCX)
      assert frame==sp-128 and rsp%16==8
      saved=struct.unpack('<16Q',uc.mem_read(frame,128))
      assert saved[10]==bp and saved[3]==initial[UC_X86_REG_R12]
      if force_allow:wq(frame+14*8,saved[14]&~255)
      ret=rq(rsp)
      for reg in regs:
       if reg!=UC_X86_REG_RSP:uc.reg_write(reg,0xbadc0de)
      for reg in xmms:uc.reg_write(reg,0xdeadbeef)
      uc.reg_write(UC_X86_REG_EFLAGS,0x202)
      uc.reg_write(UC_X86_REG_RAX,int(blocked));uc.reg_write(UC_X86_REG_RSP,rsp+8);uc.reg_write(UC_X86_REG_RIP,ret)
      callbacks.append(frame);return
     if helper is not None and address==helper:
      assert rsp%16==8
      helper_calls.append(tuple(uc.reg_read(reg) for reg in [UC_X86_REG_RCX,UC_X86_REG_RDX,UC_X86_REG_R8]))
      ret=rq(rsp)
      uc.reg_write(UC_X86_REG_RAX,0x6789);uc.reg_write(UC_X86_REG_RCX,0x112233);uc.reg_write(UC_X86_REG_RDX,0x445566)
      uc.reg_write(UC_X86_REG_RSP,rsp+8);uc.reg_write(UC_X86_REG_RIP,ret)
    emulator_hook=u.hook_add(unicorn.UC_HOOK_CODE,intercept)
    if not compiled and blocked:destinations.append(GB+skip)
    else:
     u.emu_start(base+offset if compiled else GB+rva,0,count=1200)
     assert len(destinations)==1,(name,compiled,hex(u.reg_read(UC_X86_REG_RIP)))
    assert len(callbacks)==int(compiled)
    snapshots.append(([u.reg_read(reg) for reg in regs],[u.reg_read(reg) for reg in xmms],u.reg_read(UC_X86_REG_EFLAGS)&0x8c5,helper_calls,destinations[0],bytes(u.mem_read(sp,0x800)),bytes(u.mem_read(heap,0x1800))))
    u.hook_del(emulator_hook);del u;gc.collect()
   assert snapshots[0]==snapshots[1],(name,blocked,force_allow,nonnull,'captured/hook state mismatch',[(i,a,b) for i,(a,b) in enumerate(zip(*snapshots)) if a!=b])
   results.append({'hook':name,'base_shift':hex(shift),'veto':blocked,'cleanup_override':force_allow,'nonnull':nonnull,'pass':True})
result={'cases':results,'checks':['all five exact captured signatures and instruction boundaries','all general registers and XMM0-XMM15','status flags','callback frame and 16-byte stack alignment','native helper arguments','native memory writes','accepted and rejected bulk branches','both cleanup protection branches and override','null and retained instant record release','two DLL addresses'],'fixture_services':['policy callback','native field lookup','native record release'],'live_game_verified':False}
(OUT/'integrity_hook_results.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
