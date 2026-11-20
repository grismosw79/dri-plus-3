from car35_fixture import *
import re,hashlib
S=SOURCE
C=Path(os.environ.get('DRI_CAPTURE_DIR','unpacked/DRI_ReadOnly_20261031_170052_33d487'))
report=json.loads((C/'report.json').read_text(encoding='utf-8-sig'))
sections=[(s['Rva'],(C/s['File']).read_bytes())for s in report['Sections']if s.get('Selected')]
def captured(r,n):
 for a,b in sections:
  if a<=r and r+n<=a+len(b):return b[r-a:r-a+n]
 raise AssertionError(hex(r))
cs=(S/'car35_sites.inc').read_text();main=(S/'retention.c').read_text();sig=(S/'signatures.h').read_text()
guards=[(int(r,16),int(n))for r,n in re.findall(r'\{(0x[0-9a-f]+),(\d+),0x[0-9a-f]+ULL\}',cs)]
patches=[]
for r,n,bb in re.findall(r'\{(0x[0-9a-f]+),(\d+),\{([^}]+)\}',main.split('static Hook hooks[]')[1].split('#define HOOK_COUNT')[0])+re.findall(r'\{(0x[0-9a-f]+),(\d+),\{([^}]+)\}',cs):
 b=bytes(int(x,16)for x in bb.split(','));assert len(b)==int(n)and b==captured(int(r,16),int(n));patches.append((int(r,16),b))
sig_ranges=[]
for r,n,bb in re.findall(r'\{(0x[0-9a-f]+),(\d+),\{([^}]+)\}',sig):
 b=bytes(int(x,16)for x in bb.split(','));assert b==captured(int(r,16),int(n));sig_ranges.append((int(r,16),int(n)))
def restore():
 u.mem_write(G,(C/'pe_headers.bin').read_bytes())
 for r,n in guards+sig_ranges+[(r,len(b))for r,b in patches]:u.mem_write(G+r,captured(r,n))
def exact():return all(bytes(u.mem_read(G+r,len(b)))==b for r,b in patches)
restore();assert call('DriTestCar35Verify',G)==1 and call('DriTestTU1029VerifyBuild',G)==1
for r,b in patches:
 u.mem_write(G+r,bytes([b[0]^1]));assert call('DriTestTU1029VerifyBuild',G)==0,hex(r);u.mem_write(G+r,b)
check(f'Inspected capture {C.name}: all {len(patches)} sites match; mutating each site refuses the combined runtime')
# Also reject a non-patch dependency change in the longer native guards.
r,n=guards[1];orig=bytes(u.mem_read(G+r+n-1,1));u.mem_write(G+r+n-1,bytes([orig[0]^1]));assert call('DriTestCar35Verify',G)==0;restore()
check(f'{len(guards)} current carousel dependency guards checked, beyond the patch bytes')
# New R10 descriptor tables contain relocated pointers. Verify the complete
# carousel guard set at a second image base, with only those pointer words
# relocated. A changed index must still refuse the compatible image.
member_ranges=[(int(r,16),int(n))for r,n in re.findall(r'\{(0x[0-9a-f]+),(\d+),0x[0-9a-f]+ULL\}',cs.split('static const Car35Guard car35_member_guards[]={')[1].split('};')[0])]
relocated=G+0x20000000;u.mem_map(relocated,302030848)
for r,n in guards+sig_ranges+[(r,len(b))for r,b in patches]:u.mem_write(relocated+r,captured(r,n))
for r,n in member_ranges:
 for o in range(8,n,16):wq(relocated+r+o,rq(relocated+r+o)+relocated-G)
assert call('DriTestCar35Verify',relocated)==1
r,n=member_ranges[0];wi(relocated+r,123);assert call('DriTestCar35Verify',relocated)==0
config();check('All six member-table guards accept a relocated image and reject a changed descriptor index')
old_win=win;allocated=[];prot={};fail_protect=[0];fail_flush=[0];protect_count=[0];flush_count=[0];in_site=[False];suspends=[0];resumes=[0]
end=(G+302030848+65535)&~65535

def installer_win(name):
 a,b,c,d=args()
 if name=='GetSystemInfo':
  u.mem_write(a,b'\0'*48);wi(a+4,4096);wq(a+8,0x10000);wq(a+16,0x7ffffffeffff);wi(a+40,65536);return 0
 if name=='GetTickCount64':return 1
 if name=='VirtualQuery':
  # Large game image and then individual allocated relay pages.
  if a<end:base=G;size=end-G;state=0x1000
  else:
   match=next((x for x in allocated if x<=a<x+4096),None)
   base=a;size=0x10000;state=0x10000
   if match is not None:base=match;size=4096;state=0x1000
  u.mem_write(b,b'\0'*48);wq(b,base);wq(b+24,size);wi(b+32,state);return 48
 if name=='VirtualAlloc':
  assert a>=end;u.mem_map(a,4096);allocated.append(a);prot[a]=4;return a
 if name=='VirtualProtect':
  protect_count[0]+=1
  if protect_count[0]==fail_protect[0]:return 0
  page=a&~4095;wi(d,prot.get(page,0x20));prot[page]=c;return 1
 if name=='FlushInstructionCache':
  flush_count[0]+=1;return int(flush_count[0]!=fail_flush[0])
 if name=='CreateToolhelp32Snapshot':return 10
 if name=='Thread32First':wi(b+8,43);wi(b+12,456);return 1
 if name=='Thread32Next':return 0
 if name=='GetLastError':return 18
 if name=='GetCurrentProcessId':return 456
 if name=='GetCurrentThreadId':return 42
 if name in ['CloseHandle','OpenThread']:return 11
 if name=='SuspendThread':suspends[0]+=1;return 0
 if name=='ResumeThread':resumes[0]+=1;return 0
 if name=='GetThreadContext':wq(b+0xf8,G+patches[-1][0] if in_site[0] else G+0x5000);return 1
 return old_win(name)
# Replace import handlers while retaining the compiled IAT addresses.
for desc in P.DIRECTORY_ENTRY_IMPORT:
 for item in desc.imports:
  nm=item.name.decode();handlers[rq(item.address)]=lambda nm=nm:installer_win(nm)
pages=set((G+r+e)&~4095 for r,b in patches for e in [0,len(b)-1]);page_count=len(pages)
for label,pf,ff,inside in [('success',0,0,False),('thread_in_new_site',0,0,True),('protection_before_writes',4,0,False),('flush_after_writes',0,2,False),('restore_protection_failure',page_count+2,0,False)]:
 restore();fail_protect[0]=pf;fail_flush[0]=ff;protect_count[0]=flush_count[0]=0;in_site[0]=inside
 before_s=suspends[0];before_r=resumes[0]
 result=call('DriTestCar35Install',G)
 assert result==int(label=='success'),(label,result)
 assert suspends[0]-before_s==resumes[0]-before_r
 assert all(prot.get(p,0x20)==0x20 for p in pages),(label,'protection restoration')
 if label!='success':assert exact(),label
 else:
  assert all(bytes(u.mem_read(G+r,len(b)))!=b for r,b in patches)
  for r,b in patches[51:]:
   data=bytes(u.mem_read(G+r,len(b)));assert data[0]==0xe8 and data[5:]==b'\x90'*(len(b)-5)
   relay=G+r+5+struct.unpack_from('<i',data,1)[0];assert bytes(u.mem_read(relay,6))==b'\xff\x25\0\0\0\0'
 check('Combined installer '+label+': all threads resumed, exact protection restoration, '+(str(len(patches))+' hooks installed together'if label=='success'else'no changed game bytes remain'))
restore()
r={'passed':True,'capture':C.name,'game_sha256':report['ExecutableSha256'],'fixture_sha256':hashlib.sha256((ROOT/'retention_fixture.dll').read_bytes()).hexdigest(),'checks':checks,'retained_R4_sites':51,'carousel_sites':len(patches)-51,'carousel_dependency_guards':len(guards),'compiled_installer_executed':True,'win32_services_modeled':True,'live_windows_injection':False}
(ROOT/('install_results_'+report['ExecutableSha256'][:8]+'.json')).write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r,indent=2))
