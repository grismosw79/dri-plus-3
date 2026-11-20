"""Compiled eligibility helper through captured rejection/format/grow/reserve code.

Schema/grade services, printf and allocator imports are modeled. The native
string implementation is executed, including heap transitions and DLL cleanup.
The shipped R9 source is rebuilt as a negative control and must fault at +0xf.
"""
from pathlib import Path
import os, sys, subprocess, shutil

setup = Path(__file__).with_name('test28r9_native_pipeline_test.py').read_text().split('status=[]')[0]
exec(compile(setup, __file__, 'exec'))

# Restore captured code over the old fixture's no-op diagnostic formatter.
for address, length in [(0x007ab000, 0x1000), (0x0074a000, 0x1000), (0x007529b0, 0x1000)]:
 map_game(address, length)
 uc.mem_write(GAME+address, raw[address-0x1000:address-0x1000+length])
 for h in list(handlers):
  if GAME+address <= h < GAME+address+length: del handlers[h]
 uc.ctl_remove_cache(GAME+address, GAME+address+length)

live_allocations={}; allocations=[]; frees=[]; formatter_calls=[]; initialized=[]
payload=b''
def printf_import():
 dest, size, fmt, argv=args()
 # Test the native string mechanics at both sides of the 15-byte SSO boundary.
 assert fmt==GAME+0x0bd322f8, ('unexpected rejection format',hex(fmt))
 assert argv and size==(len(payload)+1 if dest else 0)
 if dest: uc.mem_write(dest,payload+b'\0')
 formatter_calls.append({'size':size,'query_only':not bool(dest)})
 return len(payload)
def allocate_import():
 allocator,size,alignment,_=args()
 assert rq(allocator)==0 and alignment==0
 base=alloc(size+32); address=base+16
 uc.mem_write(base,b'\xa5'*16+b'\xcc'*size+b'\x5a'*16)
 live_allocations[address]=(allocator,size,base)
 allocations.append(size)
 return address
def free_import():
 allocator,address,size,_=args()
 assert address in live_allocations, ('unknown/double release',hex(address))
 expected_allocator,expected_size,base=live_allocations.pop(address)
 assert (allocator,size)==(expected_allocator,expected_size)
 assert rq(allocator)==0
 assert bytes(uc.mem_read(base,16))==b'\xa5'*16
 assert bytes(uc.mem_read(address+size,16))==b'\x5a'*16
 assert bytes(uc.mem_read(address,len(payload)+1))==payload+b'\0'
 frees.append(size)
 return 0
def copy_import():
 dest,source,size,_=args();uc.mem_write(dest,bytes(uc.mem_read(source,size)));return dest
def fill_import():
 dest,value,size,_=args();uc.mem_write(dest,bytes([value&255])*size);return dest
for address,fn in [(0x019e8ee0,printf_import),(0x019ea670,allocate_import),
                   (0x019ea6a0,free_import),(0x078cd64c,copy_import),(0x078cd652,fill_import)]:
 game_api(address,fn)
def observe(u,address,size,data):
 if address==GAME+0x007ac0c0:
  reason=reg('RCX')
  if (b'DRI_PORTAL_TEST28R10' in image or (b'DRI_PORTAL_TEST28R11' in image or (b'DRI_PORTAL_TEST28R12' in image or (modern_r14 or b'DRI_PORTAL_TEST28R13' in image)))):
   assert reason and bytes(uc.mem_read(reason,24))==b'\0'*15+b'\x0f'+b'\0'*8
   initialized.append(reason)
uc.hook_add(unicorn.UC_HOOK_CODE,observe)

if '--r9-negative-control' in sys.argv:
 assert b'DRI_PORTAL_TEST28R9' in image and (b'DRI_PORTAL_TEST28R10' not in image and b'DRI_PORTAL_TEST28R11' not in image)
 current_grade=5;faults=[];payload=b'Failing grade should explain the reason.'
 def invalid(u,access,address,size,value,data):
  faults.append({'address':hex(address),'rva':hex(reg('RIP')-GAME),'receiver':hex(reg('RCX'))})
  return False
 uc.hook_add(unicorn.UC_HOOK_MEM_INVALID,invalid)
 try:call(exports['DriTestEligibility'],backup,team,ctx)
 except unicorn.UcError:
  assert faults[-1]=={'address':'0xf','rva':'0x7ac134','receiver':'0x0'},faults
  print('R9_NULL_REASON_REPRODUCED',json.dumps(faults[-1]))
 else:raise AssertionError('Shipped R9 unexpectedly survived native rejection')
 sys.exit(0)

assert (b'DRI_PORTAL_TEST28R10' in image or (b'DRI_PORTAL_TEST28R11' in image or (b'DRI_PORTAL_TEST28R12' in image or (modern_r14 or b'DRI_PORTAL_TEST28R13' in image))))
cases=[]
for length in [0,1,14,15,16,30,31,80,255,1024,80,1,16]:
 payload=(b'Native rejected grade. '*60)[:length]
 current_grade=5;before=(len(allocations),len(frees),len(formatter_calls),len(initialized))
 seen.clear()
 assert call(exports['DriTestEligibility'],backup,team,ctx)==0
 assert seen==['requirement','grade']
 assert call(exports['DriTestQualityFault'])==0
 assert not live_allocations
 a,f,prints,init=(len(allocations)-before[0],len(frees)-before[1],len(formatter_calls)-before[2],len(initialized)-before[3])
 assert a==f==int(length>15) and prints==(2 if length else 1) and init==1
 reason=initialized[-1]
 if length<=15:
  assert bytes(uc.mem_read(reason,length+1))==payload+b'\0'
  assert bytes(uc.mem_read(reason+15,1))==bytes([15-length])
 cases.append({'reason_length':length,'heap_allocations':a,'matching_frees':f,'allowed':False})
 # A subsequent successful check cannot inherit the prior failure or allocate.
 current_grade=10;before_success=(len(allocations),len(frees),len(formatter_calls))
 assert call(exports['DriTestEligibility'],backup,team,ctx)==1
 assert before_success==(len(allocations),len(frees),len(formatter_calls))
 assert call(exports['DriTestQualityFault'])==0

call(exports['DriTestOwnershipReset'])
assert all(v==1 for v in refs.values()),[(records[r]['name'],v)for r,v in refs.items()if v!=1]
control=OUTPUT/'r9-control';control.mkdir(exist_ok=True)
shutil.copy2(os.environ['DRI_TEST28R9_CONTROL_DLL'],control/'retention_fixture.dll')
proc=subprocess.run([sys.executable,__file__,'--r9-negative-control'],
 env=dict(os.environ,DRI_TEST_OUTPUT_DIR=str(control)),capture_output=True,text=True)
(OUTPUT/'r9-null-reason-negative.log').write_text(proc.stdout+proc.stderr)
assert proc.returncode==0 and 'R9_NULL_REASON_REPRODUCED' in proc.stdout,proc.stdout[-2000:]+proc.stderr
result={'scope':__doc__,'cases':cases,'success_after_each_rejection':True,
 'allocations':len(allocations),'frees':len(frees),'no_live_allocations':not live_allocations,
 'R9_null_pointer_negative_control':True,'live_game_verified':False}
(OUTPUT/'test28r10_eligibility_reason_results.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
