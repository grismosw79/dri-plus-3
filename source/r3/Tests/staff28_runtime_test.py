"""Actual compiled callbacks with mock SDF and captured native integer spline.
No game process or save is modified. Native cosine and Windows services are fixtures.
"""
from pathlib import Path
import math,hashlib
source=(Path(__file__).parent/'runtime_fixture_test.py').read_text()
exec(compile(source.split("season=record('season'")[0],'runtime_fixture_test.py','exec'))
# Replay the captured packed-table bool getter before testing compiled callers.
# It masks AL, not EAX; both false and true can retain unrelated upper bits.
abi_dir=ROOT.parents[2]/'Audit/Test28/BooleanABI'
abi=json.loads((abi_dir/'fixture.json').read_text())
capture_report=json.loads((CAPTURE/'report.json').read_text(encoding='utf-8-sig'))
assert abi['executable_sha256']==capture_report['ExecutableSha256']
def bool_fixture(key):
    entry=abi[key];data=(abi_dir/entry['file']).read_bytes()
    assert len(data)==entry['size'] and hashlib.sha256(data).hexdigest()==entry['sha256']
    for section in capture_report['Sections']:
        if not section.get('Selected') or section['Rva']>entry['rva']:continue
        captured=(CAPTURE/section['File']).read_bytes();offset=entry['rva']-section['Rva']
        if offset+len(data)<=len(captured):
            assert data==captured[offset:offset+len(data)];return data
    raise AssertionError('Boolean ABI fixture missing from supplied capture')
bool_code=bool_fixture('getter');binding=bool_fixture('vtable_binding')
assert struct.unpack('<Q',binding)[0]==GAME+abi['getter']['rva']
bool_page=(GAME+abi['getter']['rva'])&~4095
uc.mem_map(bool_page,4096);uc.mem_write(GAME+abi['getter']['rva'],bool_code)
bool_table=alloc(0x60);bool_schema=alloc(0x80);bool_fields=alloc(0x58);bool_storage=alloc(0x50)
wq(bool_table+8,bool_schema);wq(bool_schema+0x40,bool_fields);wi(bool_fields+0x30,1)
wq(bool_table+0x38,bool_storage);wi(bool_storage+0x20,0);wi(bool_storage+0xc,4);wi(bool_storage+0x18,16)
bool_results=[]
for value in (0,1):
    wi(bool_storage+0x40,0xA5A50000|value)
    actual=call(GAME+abi['getter']['rva'],bool_table,0,0)
    assert actual==0xA5A50000|value
    bool_results.append({'logical_bool':value,'native_eax':hex(actual),'low_byte':actual&255})
fixtures=json.loads((Path(__file__).parent/'staff28_saved_fixtures.json').read_text())
call(exports['DriTestConfigure'],GAME,1,7)
# Native packed bool getters return AL only; upper EAX can retain storage bits.
def native_bool_dirty():
    value=native_scalar();assert value in (0,1)
    return 0xA5A50000|value
wq(TABLEVT+0xf8,api(native_bool_dirty))
season=record('staff season',{24:1})
franchise=record('staff franchise',{28:123456},ref={44:season})
context_record=record('staff context',ref={2:franchise});context=alloc(0x1400)
wq(context+0x10,rq(context_record+0x10));wi(context+0x18,ri(context_record+0x18));wq(context+0x30,0xabcdef)
teams=[];coaches=[]
for row in fixtures['snapshots']['tp4']:
    cs=[record(row['school']+' '+role,{113:score}) for role,score in zip(['HC','OC','DC'],row['scores'])]
    coaches.append(cs);teams.append(record(row['school'],{383:row['tid']},ref=dict(zip([111,183,58],cs))))
array=record('138 schools',array=teams);records[franchise]['ref'][45]=array;wq(context+0x80,array)
player=record('staff test player',{3:0,214:0});answer=alloc(4)
def grade(tid,pos):
    records[player]['scalar'][3]=pos;wi(answer,99)
    assert call(exports['DriTestStaff28'],context,teams[tid],player,answer)==1
    return ri(answer)
checks=['Captured native packed-table getter returns dirty upper EAX for both false and true; fixture bytes and vtable binding match supplied capture']
for row in fixtures['snapshots']['tp4']:
    assert grade(row['tid'],0)==row['grades'][0]
    assert grade(row['tid'],10)==row['grades'][1]
assert call(exports['DriTestStaff28Generation'])==1
checks.append('Player-specific grades match all 138 approved school/side outputs; stable cache reused')
teamref=alloc(8);playerref=alloc(8);wq(playerref,player)
for row in fixtures['snapshots']['tp4']:
    wq(teamref,teams[row['tid']])
    for pos,side in [(0,0),(9,0),(10,1),(18,1)]:
        records[player]['scalar'][3]=pos
        assert call(exports['DriTestEpsGrade'],context,teamref,11,playerref)==row['grades'][side]
checks.append('Integrated generic player-grade router uses offensive/defensive Coach grades for all 138 schools')
epsaddresses=alloc(64);call(exports['DriTestEpsAddresses'],epsaddresses)
epsnative=list(struct.unpack('<8Q',uc.mem_read(epsaddresses,64)));native_calls=[]
api(lambda:(native_calls.append(args()) or 7),epsnative[0])
api(lambda:(native_calls.append(args()) or 8),epsnative[1])
records[season]['scalar'][24]=0
assert call(exports['DriTestStaff28'],context,teams[0],player,answer)==0
assert call(exports['DriTestEpsGrade'],context,teamref,11,playerref)==7
records[season]['scalar'][24]=1;records[franchise]['ref'][44]=0
assert call(exports['DriTestStaff28'],context,teams[0],player,answer)==0
records[franchise]['ref'][44]=season;records[context_record]['ref'][2]=0
assert call(exports['DriTestStaff28'],context,teams[0],player,answer)==0
records[context_record]['ref'][2]=franchise
assert call(exports['DriTestQualityFault'])==0
checks.append('Dirty upper bits in native bool returns preserve true readiness and false/no-franchise/no-season native fallback without disabling the DLL')
for pos in [19,20,21]:
    records[player]['scalar'][3]=pos
    assert call(exports['DriTestStaff28'],context,teams[0],player,answer)==0
    assert call(exports['DriTestEpsGrade'],context,teamref,11,playerref)==7
assert call(exports['DriTestEpsGeneric'],context,teamref,11,0)==8
checks.append('K/P/LS preserve the native combined grade')
# Changing the target school immediately refreshes. Legitimate zero differs from a vacancy.
tid=60;records[coaches[tid][1]]['scalar'][113]=0
grade(tid,0);assert call(exports['DriTestStaff28Generation'])==2
assert call(exports['DriTestStaff28Presence'],tid,1)==1
records[teams[tid]]['ref'][183]=0
grade(tid,0);assert call(exports['DriTestStaff28Generation'])==3
assert call(exports['DriTestStaff28Presence'],tid,1)==0
records[teams[tid]]['ref'][183]=coaches[tid][1]
checks.append('Own-school score changes and vacancy/real-zero transitions refresh without HC renormalization')
# Native staff recalculation invalidates other-school ranks in the same week.
addresses=alloc(40);call(exports['DriTestStaff28Addresses'],addresses)
native_update,update,hook,native_proximity,hook_proximity=struct.unpack('<5Q',uc.mem_read(addresses,40));seen=[]
api(lambda:(seen.append(reg('RCX')) or 0),native_update)
records[coaches[90][0]]['scalar'][113]=100000
call(update,context);assert seen==[context]
grade(90,0);assert call(exports['DriTestStaff28Generation'])==4
assert grade(90,0)==0
checks.append('Native staff-refresh callback invalidates all side ranks for same-week changes at another school')
# A second dynasty at the same evaluator address must not borrow the first snapshot.
franchise2=record('staff second franchise',{28:654321},ref={44:season,45:array});records[context_record]['ref'][2]=franchise2
grade(90,0);assert call(exports['DriTestStaff28Generation'])==5
assert refs[franchise]==1 and refs[franchise2]==1
checks.append('Same evaluator reused for a new dynasty rebuilds without retaining either franchise')
for wanted in [0,-1]:
    call(exports['DriTestConfigure'],GAME,wanted,7)
    assert call(exports['DriTestStaff28'],context,teams[0],player,answer)==0
call(exports['DriTestConfigure'],GAME,1,7)

# Proximity uses SDF float32 coordinate fields, typed SPLN conversion, and the
# original integer-spline machine code, including its truncation/interpolation.
def map_native(rva,size):
    for page in range(rva&~4095,(rva+size+4095)&~4095,4096):
        if not any(lo<=GAME+page<=hi for lo,hi,_ in uc.mem_regions()):uc.mem_map(GAME+page,4096)
        uc.mem_write(GAME+page,raw[page-0x1000:page-0x1000+4096])
map_native(0x07e0eca0,0x400);map_native(0x078cd778,1)
api(lambda:0,GAME+0x07e0eca0) # already materialized spline data
def set_float(value):uc.reg_write(UC_X86_REG_XMM0,struct.unpack('<I',struct.pack('<f',value))[0]);return 0
def get_float():
    t,row,f,_=args();return set_float(records[tables[t][row]]['scalar'][f-0x100000])
wq(TABLEVT+0x110,api(get_float))
api(lambda:set_float(math.cos(struct.unpack('<f',struct.pack('<I',reg('XMM0')&0xffffffff))[0])),GAME+0x078cd778)
def typed_record():
    r,out,tag,_=args();assert tag in [0x41525259,0x53504c4e];incref(r);wq(out,r);return out
wq(VT+0x28,api(typed_record))
knots=[100,300,450,1050,1250,1440,1635,1825,2020,2200,2400];values=[0,1,2,5,6,7,8,9,10,11,12]
data=alloc(96);uc.mem_write(data,b''.join(struct.pack('<ii',x,y) for x,y in zip(knots,values))+struct.pack('<Q',11))
spline=record('native proximity spline');wq(spline+0x20,data)
tuning=record('school tuning',ref={14:spline});wq(context+0x88,tuning)
city=record('target city',{17:0.,18:0.,24:47});records[teams[0]]['ref'][23]=city
homecity=record('original Alaska hometown');records[player]['ref'][189]=homecity
def proximity(state):
    records[player]['scalar'][214]=state;wi(answer,99)
    handled=call(exports['DriTestProximity28'],context,teams[0],player,answer)
    return handled,ri(answer)
def expected(distance):
    x=int(distance)
    if x<=knots[0]:return values[0]
    if x>=knots[-1]:return values[-1]
    i=next(i for i in range(10) if x<knots[i+1])
    return values[i]+(x-knots[i])*(values[i+1]-values[i])//(knots[i+1]-knots[i])
coverage=0
for row in fixtures['seattle']:
    records[city]['scalar'].update({17:row['lat'],18:row['lon']})
    result=proximity(1);assert result==(1,expected(row['miles'])),(row['school'],result,row['miles'])
    coverage+=result[1]<=4
assert coverage==20
wq(teamref,teams[0]);assert call(exports['DriTestEpsGrade'],context,teamref,1,playerref)==expected(fixtures['seattle'][-1]['miles'])
assert call(exports['DriTestProximity28Direct'],context,teamref,playerref)==expected(fixtures['seattle'][-1]['miles'])
assert call(hook_proximity,context,teamref,playerref)==expected(fixtures['seattle'][-1]['miles'])
assert records[player]['ref'][189]==homecity
checks.append('Alaska→Seattle grades match all 138 schools through captured spline code; B coverage is 20 schools')
proximity_calls=[]
api(lambda:(proximity_calls.append(args()[:3]) or 5),native_proximity)
for state in [0,2,10,49]:
    assert proximity(state)==(0,99)
    assert call(exports['DriTestProximity28Direct'],context,teamref,playerref)==5
    assert proximity_calls[-1]==(context,teamref,playerref)
records[city]['scalar'][24]=1;assert proximity(1)==(0,99);records[city]['scalar'][24]=47
for state in [50,51]:
    assert proximity(state)==(1,4)
    assert call(exports['DriTestProximity28Direct'],context,teamref,playerref)==4
checks.append('Ordinary domestic/Hawaii grades remain native; Alaska-local schools remain native; NonUS/INVALID preserve neutral eligibility at B')
checks.append('Direct native Proximity hook used by CPU target cost agrees with generic eligibility/UI router and preserves native fallback arguments')
records[teams[0]]['ref'][23]=0;assert proximity(1)==(1,13);records[teams[0]]['ref'][23]=city
records[tuning]['ref'][14]=0;assert proximity(1)==(1,13);records[tuning]['ref'][14]=spline
checks.append('Missing geographic/spline data remains Incomplete rather than a fabricated grade')
for wanted in [0,-1]:
    call(exports['DriTestConfigure'],GAME,wanted,7);assert proximity(1)==(0,99)
    assert call(hook_proximity,context,teamref,playerref)==5
    assert proximity_calls[-1]==(context,teamref,playerref)
call(exports['DriTestConfigure'],GAME,1,7)
# Replay the actual trampoline prologs to the exact native resume addresses.
# Synthetic native-body epilogs isolate ABI correctness from native services.
def trampoline_abi(entry,native,resume,epilog,drop,*arguments):
    handlers.pop(native,None)
    uc.mem_write(native,image[native-BASE:native-BASE+32])
    uc.ctl_remove_cache(native,native+32)
    map_native(resume-GAME,1);uc.mem_write(resume,bytes.fromhex(epilog))
    values={name:0x123000+i*0x100 for i,name in enumerate(['RBX','RBP','RSI','RDI','R12','R13','R14','R15'])}
    for name,value in values.items():uc.reg_write(globals()['UC_X86_REG_'+name],value)
    xmm=[0xabc00000000000000000000000000000+i for i in range(10)]
    for i,value in enumerate(xmm,6):uc.reg_write(globals()['UC_X86_REG_XMM'+str(i)],value)
    visits=[];native_sp=[]
    def watch(u,at,size,data):
        if at==native:
            visits.append('native');native_sp.append(reg('RSP'));assert reg('RCX')==context
        if at==resume:
            visits.append('resume');assert reg('RSP')==native_sp[0]-drop
            assert reg('RBP')==native_sp[0]-0x5f
    handle=uc.hook_add(unicorn.UC_HOOK_CODE,watch)
    try:call(entry,*arguments)
    finally:uc.hook_del(handle)
    assert visits==['native','resume'],(visits,hex(entry),hex(native),hex(resume))
    assert reg('RSP')==STACK+0xf0000
    for name,value in values.items():assert reg(name)==value,(name,hex(reg(name)),hex(value))
    assert [uc.reg_read(globals()['UC_X86_REG_XMM'+str(i)]) for i in range(6,16)]==xmm
trampoline_abi(hook,native_update,GAME+0x0841f0f0,'4881c4a80000005f5dc3',0xb8,context)
records[player]['scalar'][214]=0
trampoline_abi(hook_proximity,native_proximity,GAME+0x0840b41e,'b805000000415e415d5f5e5b5dc3',0x30,context,teamref,playerref)
checks.append('Both actual entry-hook trampolines replay original prologs, resume once at verified addresses, and preserve stack/nonvolatile registers/XMM6-15')
call(exports['DriTestStaff28Reset'])
assert all(v==1 for v in refs.values()),[(records[r]['name'],v) for r,v in refs.items() if v!=1]
assert call(exports['DriTestQualityFault'])==0
checks.append('Disabled/diagnostic paths are native; no leaked SDF references or runtime fault')
(OUTPUT/'staff28_runtime_results.json').write_text(json.dumps({'checks':checks,'boolean_abi':bool_results,'live_game_verified':False},indent=2)+'\n')
print(json.dumps(checks,indent=2))
