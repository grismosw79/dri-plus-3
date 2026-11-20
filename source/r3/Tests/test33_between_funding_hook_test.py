"""Captured between-pass boundary versus the compiled production detour.

The callback is adversarial and stubbed to isolate entry ABI, exact stolen
replay, candidate release timing and next-pass indexing. Actual compiled
replanning and native spending have separate Test33 runtime checks.
"""
from pathlib import Path
import gc,json,os,struct
import capstone,pefile,unicorn
from unicorn.x86_const import *

S=Path(__file__).resolve().parents[1]
C=Path(os.environ['DRI_CAPTURE_DIR'])
OUT=Path(os.environ['DRI_TEST_OUTPUT_DIR'])
pe=pefile.PE(str(S.parent/'DRIChallengerRetention.dll'))
image=pe.get_memory_mapped_image()
raw=(C/'section_00_00001000.bin').read_bytes()
GAME=0x140000000
SITE,RESUME=0x081722f1,0x08172300
original=bytes.fromhex('488b7de033c0898628020000448be8')
assert raw[SITE-0x1000:RESUME-0x1000]==original
md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);md.detail=True
assert [(i.mnemonic,i.op_str) for i in md.disasm(original,GAME+SITE)]==[
 ('mov','rdi, qword ptr [rbp - 0x20]'),('xor','eax, eax'),
 ('mov','dword ptr [rsi + 0x228], eax'),('mov','r13d, eax')]
prefix=bytes.fromhex('9c5051525355565741504151415241534154415541564157')
offsets=[];at=0
while (at:=image.find(prefix,at))>=0:offsets.append(at);at+=1
assert len(offsets)==42==int(os.environ['DRI_TEST_EXPECTED_SAVED_HOOK_COUNT'])
matches=[off for index,off in enumerate(offsets)
         if original in image[off:min(off+240,offsets[index+1] if index+1<len(offsets) else len(image))]]
assert len(matches)==1,matches
offset=matches[0]
names=['RAX','RCX','RDX','RBX','RBP','RSI','RDI','R8','R9','R10','R11','R12','R13','R14','R15','RSP']
regs=[globals()['UC_X86_REG_'+n] for n in names]
xmms=[globals()['UC_X86_REG_XMM'+str(n)] for n in range(16)]
cases=[]
for shift in [0,0x70000000]:
 base=pe.OPTIONAL_HEADER.ImageBase+shift
 stub=list(md.disasm(image[offset:offset+240],base+offset))
 callback=next(i for i in stub if i.mnemonic=='call').operands[0].imm
 jump=next(i for i in stub if i.mnemonic=='jmp' and 'rip' in i.op_str)
 resume_pointer=jump.address+jump.size+jump.operands[0].mem.disp
 for count,index,present,reorder,start in [(2,0,True,False,0x081722b9),(1,0,True,False,0x081722b9),
                                         (3,2,True,True,0x081722b9),(1,0,False,True,0x081722b9),
                                         (1,0,True,False,0x081722b6),(1,0,True,True,0x08172142),
                                         (1,0,True,False,0x081722ab)]:
  snapshots=[]
  for hooked in [False,True]:
   u=unicorn.Uc(unicorn.UC_ARCH_X86,unicorn.UC_MODE_64)
   u.mem_map(base,(len(image)+4095)&~4095);u.mem_write(base,image)
   u.mem_map(GAME+0x08171000,0x2000)
   u.mem_write(GAME+0x08171000,raw[0x08171000-0x1000:0x08173000-0x1000])
   stack,heap,api=0x20000000,0x30000000,0x50000000
   for address in [stack,heap,api]:u.mem_map(address,0x10000)
   sp,bp,evaluator=stack+0x8000,stack+0x8100,heap+0x1000
   vec,vt,old_target,first,second,holder=heap+0x2000,heap+0x3000,heap+0x4000,heap+0x4100,heap+0x4200,heap+0x5000
   def putq(a,v):u.mem_write(a,struct.pack('<Q',v))
   def getq(a):return struct.unpack('<Q',u.mem_read(a,8))[0]
   def puti(a,v):u.mem_write(a,struct.pack('<I',v))
   putq(vt,api);putq(vt+8,api+0x10)
   for record in [old_target,first,second]:putq(record,vt)
   putq(vec+48,first);putq(vec+72+48,second)
   putq(bp-0x50,vec);putq(bp-0x58,count);putq(bp-0x60,holder)
   putq(bp-0x20,0x123456789abcdef);putq(bp-0x18,0xfeed)
   putq(sp+0x58,old_target if present else 0)
   puti(evaluator+0x228,123);puti(evaluator+0x230,456)
   initial={r:0x11220000+n*0x100 for n,r in enumerate(regs)}
   initial.update({UC_X86_REG_RSP:sp,UC_X86_REG_RBP:bp,UC_X86_REG_RSI:evaluator,
                   UC_X86_REG_R12:holder,UC_X86_REG_R13:index,UC_X86_REG_R14:old_target,UC_X86_REG_R15:holder})
   expected_releases=([old_target] if start in [0x08172142,0x081722ab] else [])+([old_target] if present else [])
   for r,v in initial.items():u.reg_write(r,v)
   xmm_initial=[(n+1)*0x112233445566778899aabbccddeeff for n in range(16)]
   for r,v in zip(xmms,xmm_initial):u.reg_write(r,v)
   restored={8:0x88aabbccddeeff001122334455667788,7:0x77aabbccddeeff001122334455667788,6:0x66aabbccddeeff001122334455667788}
   for n,off in [(8,0x4f0),(7,0x500),(6,0x510)]:u.mem_write(sp+off,restored[n].to_bytes(16,'little'))
   u.reg_write(UC_X86_REG_EFLAGS,0x247)
   events=[];resumed=[];released=[];added=[]
   if hooked:
    u.mem_write(GAME+SITE,b'\xff\x25\0\0\0\0'+struct.pack('<Q',base+offset)+b'\x90')
    putq(resume_pointer,GAME+RESUME)
   def move_candidates():
    if reorder:
     a=bytes(u.mem_read(vec,72));b=bytes(u.mem_read(vec+72,72))
     u.mem_write(vec,b);u.mem_write(vec+72,a)
   def native_return():
    rsp=u.reg_read(UC_X86_REG_RSP);ret=getq(rsp)
    u.reg_write(UC_X86_REG_RSP,rsp+8);u.reg_write(UC_X86_REG_RIP,ret)
   def intercept(uc,address,size,data):
    if address in [GAME+0x08171f20,GAME+0x0817233e]:uc.emu_stop();return
    if address==api+0x10:
     released.append(uc.reg_read(UC_X86_REG_RCX));events.append('release');uc.reg_write(UC_X86_REG_RAX,0);native_return();return
    if address==api:
     added.append(uc.reg_read(UC_X86_REG_RCX));events.append('next_addref');uc.reg_write(UC_X86_REG_RAX,0);native_return();return
    if address==GAME+SITE and not hooked:move_candidates()
    if address==GAME+RESUME:
     resumed.append(([uc.reg_read(r) for r in regs],[uc.reg_read(r) for r in xmms],uc.reg_read(UC_X86_REG_EFLAGS)))
    if address==callback and hooked:
     rsp=uc.reg_read(UC_X86_REG_RSP);frame=uc.reg_read(UC_X86_REG_RCX)
     assert rsp%16==8 and frame==sp-128
     saved=struct.unpack('<16Q',uc.mem_read(frame,128))
     assert saved[9]==evaluator and saved[10]==bp and saved[3]==holder
     assert saved[2]==count and released==expected_releases
     assert not added and getq(bp-0x50)==vec and getq(bp-0x60)==holder
     assert all(uc.reg_read(xmms[n])==v for n,v in restored.items())
     assert struct.unpack('<I',uc.mem_read(evaluator+0x228,4))[0]==123
     assert struct.unpack('<I',uc.mem_read(evaluator+0x230,4))[0]==456
     events.append('callback');move_candidates();ret=getq(rsp)
     for r in regs:
      if r!=UC_X86_REG_RSP:uc.reg_write(r,0xbadc0de)
     for r in xmms:uc.reg_write(r,0xdeadbeef)
     uc.reg_write(UC_X86_REG_EFLAGS,0x202)
     uc.reg_write(UC_X86_REG_RSP,rsp+8);uc.reg_write(UC_X86_REG_RIP,ret)
   token=u.hook_add(unicorn.UC_HOOK_CODE,intercept)
   u.emu_start(GAME+start,GAME+0x0817233f,count=1000)
   complete=index+1==count
   assert u.reg_read(UC_X86_REG_RIP)==GAME+(0x0817233e if complete else 0x08171f20)
   assert events.count('callback')==int(hooked and complete)
   assert released==expected_releases
   if complete:
    assert added==[second if reorder else first]
    assert u.reg_read(UC_X86_REG_R13)==0 and getq(bp-0x18)==0
    assert struct.unpack('<I',u.mem_read(evaluator+0x228,4))[0]==0
    assert struct.unpack('<I',u.mem_read(evaluator+0x230,4))[0]==0
    if hooked:assert events.index('callback')<events.index('next_addref')
   else:assert not added and not resumed
   snapshots.append((resumed,[u.reg_read(r) for r in regs],[u.reg_read(r) for r in xmms],
                     u.reg_read(UC_X86_REG_EFLAGS),bytes(u.mem_read(heap,0x10000))))
   u.hook_del(token);del u;gc.collect()
  assert snapshots[0]==snapshots[1],('detour changed original boundary behavior',shift,count,index,present,reorder)
  cases.append({'dll_base':hex(base),'native_start_rva':hex(start),'native_count':count,'last_index':index,'candidate_present':present,'callback_reorders_fixture':reorder,'passed':True})
result={'passed':True,'cases':cases,'hook_rva':hex(SITE),'resume_rva':hex(RESUME),'stolen_hex':original.hex(),
 'checks':['whole captured instructions at 15-byte site','first-pass continuation never invokes callback',
 'last candidate released before callback','all GPR/XMM/status flags match unhooked control at resume and second-pass dispatch',
 'native XMM6/7/8 restore before callback','fresh physical index0 and AddRef after callback',
 'native counters and frame locals reset exactly','two DLL image bases'],
 'modeled_services':['adversarial callback with optional fixture-only permutation','candidate AddRef/Release'],
 'limitations':['Does not execute the production replanning callback or a complete week; separate runtime tests cover policy and spending'],
 'live_game_verified':False}
(OUT/'test33_between_funding_hook_results.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({'passed':True,'cases':len(cases),'hook_rva':hex(SITE)}))
