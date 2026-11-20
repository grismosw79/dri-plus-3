import gc
"""Execute the captured fallback prologue plus compiled early-return trampoline."""
exec(compile((__import__('pathlib').Path(__file__).parent/'quality_hook_test.py').read_text(),'quality_hook_test.py','exec'))
checks=[]
for shift in [0,0x70000000]:
 base=PB+shift;start=offsets[12];end=offsets[13]
 ins=list(cs.disasm(IMG[start:end],base+start));callback=next(i.operands[0].imm for i in ins if i.mnemonic=='call')
 u=unicorn.Uc(unicorn.UC_ARCH_X86,unicorn.UC_MODE_64)
 u.mem_map(base,(len(IMG)+4095)&~4095);u.mem_write(base,IMG)
 u.mem_map(0x20000000,0x20000);u.mem_map(0x30000000,0x1000);u.mem_map(GB+0x081abd10,0x2000)
 sp=0x20010008;stop=0x30000800;out=0x30000100;dest=0x30000200
 def wq(a,v):u.mem_write(a,struct.pack('<Q',v))
 def rq(a):return struct.unpack('<Q',u.mem_read(a,8))[0]
 initial={reg:0x11000000+i*0x100 for i,reg in enumerate(regs)}
 initial.update({UC_X86_REG_RSP:sp,UC_X86_REG_RDX:out})
 for reg,value in initial.items():u.reg_write(reg,value)
 for i,reg in enumerate(xmms):u.reg_write(reg,(i+1)*0x112233445566778899aabbccddeeff)
 wq(sp,stop)
 u.mem_write(GB+0x081ac8b0,raw[0x081ac8b0-0x1000:0x081ac8c2-0x1000])
 u.mem_write(GB+0x081ac8c2,b'\xff\x25\0\0\0\0'+struct.pack('<Q',base+start))
 calls=[]
 def callback_service(uc,address,size,data):
  if address==stop:uc.emu_stop();return
  if address!=callback:return
  rsp=uc.reg_read(UC_X86_REG_RSP);frame=uc.reg_read(UC_X86_REG_RCX)
  assert rsp%16==8 and frame+128==sp-0x108
  saved=struct.unpack('<16Q',uc.mem_read(frame,128));assert saved[12]==out
  wq(out,dest);wq(frame+14*8,out);ret=rq(rsp)
  for reg in regs:
   if reg!=UC_X86_REG_RSP:uc.reg_write(reg,0xbadc0de)
  for reg in xmms:uc.reg_write(reg,0xdeadbeef)
  uc.reg_write(UC_X86_REG_RAX,1);uc.reg_write(UC_X86_REG_RSP,rsp+8);uc.reg_write(UC_X86_REG_RIP,ret);calls.append(frame)
 emulator_hook=u.hook_add(unicorn.UC_HOOK_CODE,callback_service);u.emu_start(GB+0x081ac8b0,0,count=2000)
 assert len(calls)==1 and u.reg_read(UC_X86_REG_RIP)==stop
 assert u.reg_read(UC_X86_REG_RSP)==sp+8 and u.reg_read(UC_X86_REG_RAX)==out and rq(out)==dest
 for name in ['RBX','RBP','RSI','RDI','R12','R13','R14','R15']:
  reg=globals()['UC_X86_REG_'+name];assert u.reg_read(reg)==initial[reg],name
 for i,reg in enumerate(xmms):assert u.reg_read(reg)==(i+1)*0x112233445566778899aabbccddeeff
 checks.append({'base':hex(base),'stack_return_registers_xmm_and_output':'PASS'})
 u.hook_del(emulator_hook);del u;gc.collect()
(OUT/'test26_fallback_machine_results.json').write_text(json.dumps({'checks':checks,'live_game_verified':False},indent=2)+'\n')
print('PASS native fallback prologue/early-return ABI at two DLL addresses')
