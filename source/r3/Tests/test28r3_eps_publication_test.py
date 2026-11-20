"""Actual compiled grade-query path, native cache accessors and snapshot I/O.

SDF records and Windows file services are controlled. A valid pre-existing
model snapshot isolates publication from numerical fitting. The saved-season
portable replay covers the real model separately. No live game is started.
"""
from pathlib import Path
import sys,subprocess
control='--r2-control' in sys.argv
source=(Path(__file__).parent/'runtime_fixture_test.py').read_text().split("season=record('season'")[0]
source=source.replace('0x800000','0x6000000')
if control:source=source.replace("P=pefile.PE(str(OUTPUT/'retention_fixture.dll'))", "P=pefile.PE(str(ROOT.parent/'Audit/Test28R3/retention_r2_fixture.dll'))")
exec(compile(source,'runtime_fixture_test.py','exec'))
call(exports['DriTestConfigure'],GAME,1,7)
uc.mem_map(GAME+0x0fad5000,0x1000)
for rva,size in [(0x0860eda0,0x3000),(0x08614da0,0x2000),(0x08598da0,0x1000),(0x08596da0,0x1000),(0x07e18000,0x2000),(0x08055000,0x1000)]:
    uc.mem_map(GAME+rva,size);uc.mem_write(GAME+rva,raw[rva-0x1000:rva-0x1000+size])
rdata=(CAPTURE/'section_02_0AC9C000.bin').read_bytes()
for rva in [0x0bded000,0x0bdcf000]:
    uc.mem_map(GAME+rva,0x1000);uc.mem_write(GAME+rva,rdata[rva-0x0ac9c000:rva-0x0ac9c000+0x1000])
grade_fields=struct.unpack('<'+'II'*63,rdata[0xbded114-0x0ac9c000:0xbded114-0x0ac9c000+504])[::2]
rank_fields=struct.unpack('<'+'II'*63,rdata[0xbdcf4a4-0x0ac9c000:0xbdcf4a4-0x0ac9c000+504])[::2]
writes=[]
def set_value():
    t,row,f,value=args();r=tables[t][row];records[r]['scalar'][f-0x100000]=value&0xffffffff;writes.append((r,f-0x100000,value&0xffffffff));return 0
wq(TABLEVT+0x148,api(set_value));wq(TABLEVT+0xf8,api(native_scalar));api(lambda:0,GAME+0x08059580)
stat_schema=alloc(0x200);uc.mem_write(stat_schema+0xd8,struct.pack('<h',26))
wq(VT+0x38,api(lambda:records[reg('RCX')].get('schema',0xabcdef)))
teamlist=[];grades=[];ranks=[];rosters=[]
for tid in range(138):
    gr=record('grade '+str(tid),{f:8 for f in grade_fields});rr=record('rank '+str(tid),{f:90 for f in rank_fields});grades.append(gr);ranks.append(rr)
    track=record('tracking '+str(tid),ref={20:gr,21:rr});roster=record('roster '+str(tid),array=[]);rosters.append(roster)
    teamlist.append(record('team '+str(tid),{383:tid},ref={165:track,239:roster}))
hs=record('home cache',{27:100,33:10,31:50,45:10,47:0});aws=record('away cache',{27:100,33:10,31:50,45:10,47:0})
game=record('completed game',{63:0,47:0,51:0},ref={42:teamlist[0],18:teamlist[1],43:hs,19:aws},row_id=77)
for side in range(2):
    stat=record('offense stats '+str(side),{24:50,19:10,11:100,5:10,16:0,12:0,17:0},ref={3:teamlist[1-side],4:game});records[stat]['schema']=stat_schema
    history=record('player history '+str(side),array=[stat]);player=record('player '+str(side),{3:0},ref={146:history});records[rosters[side]]['array']=[player]
teams=record('teams',array=teamlist);fcs=record('fcs',array=[])
season=record('season',{2:2026,6:0,3:2,4:0,5:6,24:1})
franchise=record('franchise',{28:3118973581},ref={45:teams,11:fcs,44:season})
myrec=record('grade context',ref={2:franchise});my=alloc(0x1400);wq(my+0x10,rq(myrec+0x10));wi(my+0x18,ri(myrec+0x18));wq(my+0x30,0xabcdef)
teamref=alloc(8);wq(teamref,teamlist[0])
mask=(1<<64)-1
def fnv(data):
    h=1469598103934665603
    for b in data:h=((h^b)*1099511628211)&mask
    return h
signature=1469598103934665603^2
for side in range(2):
    h=1469598103934665603
    for v in [77,side,1-side,100,10,50,10,0]:h=((h^v)*1099511628211)&mask
    signature=(signature+h)&mask
payload=bytearray(326832);struct.pack_into('<iii',payload,0,2026,2,138)
for t in range(138):
    for p in range(23):struct.pack_into('<ddii',payload,250656+(t*23+p)*24,72.,40.,4,1)
folder='C:\\DRI\\EPS1-Cache'
snapshot=folder+f'\\b9e7c28d-2026-{signature:016x}.bin';final=folder+'\\b9e7c28d-2026-final.bin'
files={snapshot:struct.pack('<4Q6I',0x4550533154455354,signature,fnv(payload),0,3118973581,2026,0,1,len(payload),1)+payload}
handles={};next_handle=100;file_writes=[]
def wide(p):
    b=bytearray()
    while True:
        v=bytes(uc.mem_read(p,2));p+=2
        if v==b'\0\0':return b.decode('utf-16le')
        b+=v
def swprint():
    import re
    _,out,cap,fmt=args();va=rq(reg('RSP')+48);values=[]
    def replace(m):
        nonlocal va
        v=rq(va);va+=8;spec=m[0]
        if spec=='%ls':return wide(v)
        if spec.endswith('x'):
            width=int(re.search(r'\d+',spec)[0]) if re.search(r'\d+',spec) else 0
            return format(v if 'll' in spec else v&0xffffffff,'0'+str(width)+'x')
        if spec=='%d':return str((v&0xffffffff)-(1<<32) if v&(1<<31) else v&0xffffffff)
        if spec=='%lu':return str(v&0xffffffff)
        raise AssertionError(spec)
    result=re.sub(r'%(?:ls|0?\d*llx|0?\d*x|lu|d)',replace,wide(fmt));assert len(result)<cap
    uc.mem_write(out,result.encode('utf-16le')+b'\0\0');return len(result)
old_win_import=win_import
def win_import(name):
    global next_handle
    if name=='calloc':
        n=reg('RCX')*reg('RDX');p=alloc(n);uc.mem_write(p,bytes(n));return p
    if name=='free':return 0
    if name=='GetModuleFileNameW':
        path='C:\\DRI\\DRIChallengerRetention.dll';uc.mem_write(reg('RDX'),path.encode('utf-16le')+b'\0\0');return len(path)
    if name=='CreateDirectoryW':return 1
    if name=='GetCurrentProcessId':return 1234
    if name=='__stdio_common_vswprintf':return swprint()
    if name=='CreateFileW':
        path=wide(reg('RCX'));disposition=ri(reg('RSP')+40)
        if disposition==3 and path not in files:return mask
        if disposition==2:files[path]=b''
        next_handle+=1;handles[next_handle]=[path,0];return next_handle
    if name=='GetFileSizeEx':wq(reg('RDX'),len(files[handles[reg('RCX')][0]]));return 1
    if name=='ReadFile':
        h,buf,n,got=args();path,offset=handles[h];part=files[path][offset:offset+n];uc.mem_write(buf,bytes(part));wi(got,len(part));handles[h][1]+=len(part);return 1
    if name=='WriteFile':
        h,buf,n,put=args();path,offset=handles[h];assert offset==0;files[path]=bytes(uc.mem_read(buf,n));wi(put,n);file_writes.append(path);return 1
    if name=='CloseHandle':handles.pop(reg('RCX'));return 1
    if name=='FlushFileBuffers':return 1
    if name=='MoveFileExW':files[wide(reg('RDX'))]=files.pop(wide(reg('RCX')));return 1
    if name=='DeleteFileW':files.pop(wide(reg('RCX')),None);return 1
    return old_win_import(name)
def query():
    rsp=STACK+0xf0000-8;wq(rsp,STOP);uc.reg_write(UC_X86_REG_RSP,rsp)
    for name,v in zip(['RCX','RDX','R8','R9'],[my,teamref,0,0]):uc.reg_write(globals()['UC_X86_REG_'+name],v)
    uc.emu_start(exports['DriTestEpsGeneric'],STOP+1,count=150000000)
    assert reg('RIP')==STOP,('query did not return',hex(reg('RIP')))
    assert reg('RAX')&0xffffffff==4
    assert call(exports['DriTestQualityFault'])==0
query()
if control:
    assert not writes and final not in files
    assert struct.unpack_from('<Q',files[snapshot],24)[0]==0
    call(exports['DriTestEpsReset']);assert all(v==1 for v in refs.values())
    print('R2 negative control reproduced: live grade returned, school tables unchanged, no published final snapshot')
    sys.exit(0)
assert len(writes)==138*63*2
assert all(records[r]['scalar'][f]==4 for r in grades for f in grade_fields)
assert all(records[r]['scalar'][f]==39 for r in ranks for f in rank_fields)
assert final in files and files[final]==files[snapshot]
header=struct.unpack('<4Q6I',files[final][:56]);assert header[3]!=0 and header[7]==1
assert files[final][56:]==payload
first=files[final];w=len(writes);fw=len(file_writes);query()
assert len(writes)==w and len(file_writes)==fw,'Stable query should reuse the published state'
records[season]['scalar'][4]=1;query()
assert files[final]==first and files[snapshot]==first,'Later read must preserve publication hash'
call(exports['DriTestEpsReset']);query()
assert files[final]==first and files[snapshot]==first,'Restart must preserve frozen output and publication identity'
call(exports['DriTestEpsReset']);assert all(v==1 for v in refs.values())
# Prior-year disk recovery after a fresh runtime, with no new-season games.
records[season]['scalar'].update({2:2027,6:1,3:0,4:0,5:0});query()
call(exports['DriTestEpsReset']);assert all(v==1 for v in refs.values())
assert not handles
subprocess.run([sys.executable,str(Path(__file__).resolve()),'--r2-control'],check=True)
report={'passed':True,'nativeCacheCellsPublished':138*63*2,'onDemandFinalPublication':True,'snapshotModelBytesUnchanged':True,'repeatQueryDoesNotRewrite':True,'laterWeekKeepsPublicationHash':True,'restartReusesFinalSnapshot':True,'nextSeasonDiskCarryVerified':True,'referenceOwnershipBalanced':True,'r2NegativeControlReproduced':True,'live_game_verified':False}
(OUTPUT/'test28r3_eps_publication_results.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
