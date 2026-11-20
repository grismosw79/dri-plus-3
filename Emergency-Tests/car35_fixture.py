"""Execute compiled Test35 callbacks; Win32 and SDF are explicitly modeled.
Not a live-game acceptance test. Captured native control flow is tested separately.
"""
from pathlib import Path
import os,sys,struct,json
if os.environ.get('DRI_CAR35_DEPS'):sys.path.insert(0,os.environ['DRI_CAR35_DEPS'])
import unicorn,pefile
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parent
SOURCE=Path(os.environ.get('DRI_CAR35_SOURCE',str(ROOT.parent/'source/r3')))
P=pefile.PE(str(ROOT/'retention_fixture.dll'));BASE=P.OPTIONAL_HEADER.ImageBase
G=0x140000000;H=0x30000000;ST=0x20000000;API=0x50000000;STOP=0x60000000
u=unicorn.Uc(unicorn.UC_ARCH_X86,unicorn.UC_MODE_64)
image=P.get_memory_mapped_image();u.mem_map(BASE,(len(image)+4095)&~4095);u.mem_write(BASE,image)
u.mem_map(G,302002176);u.mem_map(H,0x4000000);u.mem_map(ST,0x100000);u.mem_map(API,0x10000);u.mem_map(STOP,0x1000)
exports={s.name.decode():BASE+s.address for s in P.DIRECTORY_ENTRY_EXPORT.symbols}
heap=H+0x1000;next_api=API;native_api=G+0x100000;handlers={};tls={};records={};tables={};refs={};events=[];dirty=[]
def reg(n):return u.reg_read(globals()['UC_X86_REG_'+n])
def wq(a,v):u.mem_write(a,struct.pack('<Q',v&((1<<64)-1)))
def rq(a):return struct.unpack('<Q',u.mem_read(a,8))[0]
def wi(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def ri(a):return struct.unpack('<I',u.mem_read(a,4))[0]
def alloc(n):
 global heap
 a=heap;heap=(a+n+15)&~15;assert heap<H+0x4000000;return a
def args():return reg('RCX'),reg('RDX'),reg('R8'),reg('R9')
def api(fn,address=None,native=False):
 global next_api,native_api
 if address is None:
  if native:address=native_api;native_api+=16
  else:address=next_api;next_api+=16
 handlers[address]=fn;u.mem_write(address,b'\xc3');return address
def incref(r):
 if r:refs[r]+=1
def decref(r):
 if r:refs[r]-=1;assert refs[r]>=1,('premature release',records[r]['name'],refs[r])
def ref_get():
 data,out,row,f=args();r=tables[data][row]
 v=records[r]['refs'].get(f-0x100000,0) if f>=0x100000 else records[r]['array'][f]
 incref(v);wq(out,v);return out
def scalar_get():
 data,row,f,_=args();return records[tables[data][row]]['scalar'].get(f-0x100000,0)
def scalar_set():
 data,row,f,v=args();records[tables[data][row]]['scalar'][f-0x100000]=v&0xffffffff;return 0
def cast():
 r,out,tag,_=args();assert tag==0x41525259;assert records[r]['array'] is not None;incref(r);wq(out,r);return out
SCHEMA=alloc(0x100);wq(SCHEMA+0xd0,SCHEMA);u.mem_write(SCHEMA+0xd8,struct.pack('<h',500))
VT=alloc(0x200);DVT=alloc(0x200)
wq(VT,api(lambda:incref(reg('RCX')),native=True));wq(VT+8,api(lambda:decref(reg('RCX')),native=True))
wq(VT+0x38,api(lambda:SCHEMA,native=True));wq(VT+0x28,api(cast,native=True))
for offset in [0xf8,0x100,0x108]:wq(DVT+offset,api(scalar_get,native=True))
wq(DVT+0x128,api(ref_get,native=True));wq(DVT+0x148,api(scalar_set,native=True))
def record(name,scalar=None,ref=None,array=None):
 r=alloc(0x300);data=alloc(0x20);idx=len(records)+1
 wq(r,VT);wq(r+0x10,data);wi(r+0x18,idx);wq(r+0x30,SCHEMA);wq(data,DVT)
 records[r]={'name':name,'scalar':scalar or {},'refs':ref or {},'array':array};tables[data]={idx:r};refs[r]=1;return r
def ref(r):p=alloc(8);wq(p,r);return p
api(lambda:0x100000+reg('RDX'),G+0x8055d60)
api(lambda:len(records[reg('RCX')]['array']),G+0x37f4130)
api(lambda:dirty.append(reg('RCX')),G+0x80595e0)
def win(name):
 a,b,c,d=args()
 if name in ['AcquireSRWLockExclusive','ReleaseSRWLockExclusive']:return 0
 if name in ['GetCurrentProcess','GetProcessHeap']:return 123
 if name=='TlsGetValue':return tls.get(a,0)
 if name=='TlsSetValue':tls[a]=b;return 1
 if name=='HeapAlloc':
  p=alloc(c)
  if b&8:u.mem_write(p,b'\0'*c)
  return p
 if name=='HeapFree':return 1
 if name=='ReadProcessMemory':
  n=d;got=rq(reg('RSP')+0x28)
  try:u.mem_write(c,bytes(u.mem_read(b,n)));wq(got,n);return 1
  except unicorn.UcError:wq(got,0);return 0
 if name in ['memcpy','memmove']:u.mem_write(a,bytes(u.mem_read(b,c)));return a
 if name=='memset':u.mem_write(a,bytes([b&255])*c);return a
 if name=='memcmp':
  x=bytes(u.mem_read(a,c));y=bytes(u.mem_read(b,c));return (x>y)-(x<y)
 raise AssertionError('Unexpected import '+name)
for desc in P.DIRECTORY_ENTRY_IMPORT:
 for item in desc.imports:
  name=item.name.decode();wq(item.address,api(lambda name=name:win(name)))
def on_code(uc,a,n,d):
 if a==STOP:uc.emu_stop();return
 if a in handlers:
  value=handlers[a]() or 0;rsp=reg('RSP');ret=rq(rsp)
  uc.reg_write(UC_X86_REG_RAX,value&((1<<64)-1));uc.reg_write(UC_X86_REG_RSP,rsp+8);uc.reg_write(UC_X86_REG_RIP,ret)
u.hook_add(unicorn.UC_HOOK_CODE,on_code)
def call(addr,*values,rbp=None):
 if isinstance(addr,str):addr=exports[addr]
 rsp=ST+0xf0000-8;wq(rsp,STOP)
 u.reg_write(UC_X86_REG_RSP,rsp)
 if rbp is not None:u.reg_write(UC_X86_REG_RBP,rbp)
 for n,v in zip(['RCX','RDX','R8','R9'],values):u.reg_write(globals()['UC_X86_REG_'+n],v&((1<<64)-1))
 for i,v in enumerate(values[4:]):wq(rsp+0x28+8*i,v)
 try:u.emu_start(addr,STOP+1,count=int(os.environ.get("DRI_CAR35_INSTRUCTION_LIMIT","5000000")))
 except unicorn.UcError:print('FAULT',hex(reg('RIP')),hex(reg('RSP')));raise
 assert reg('RIP')==STOP,('No return',hex(reg('RIP')))
 return reg('RAX')
def config(active=1):call('DriTestCar35Configure',G,active)
def coach(name='coach',**fields):
 s={0:0,1:45,8:4,10:3,11:100,12:0,13:2,14:35,16:1,17:1,19:3,112:6,120:10,128:0,134:2,135:10}
 s.update({int(k):v for k,v in fields.items()});return record(name,s)
checks=[]
def check(name):checks.append(name)
def assert_refs():assert all(n==1 for n in refs.values()),[(records[r]['name'],n)for r,n in refs.items()if n!=1]
config()
