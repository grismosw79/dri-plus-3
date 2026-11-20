"""Compiled production callbacks, modeled native/Win32 services.
Return-address thunks exercise the real scope boundary. Not live acceptance.
"""
from car35_fixture import *
checks.clear()

def jump(address,target):
    u.mem_write(address,b'\xff\x25\0\0\0\0'+struct.pack('<Q',target))

def call_thunk(start,site,target,stack_args=False):
    # Windows x64 stack alignment and the three original stack arguments.
    code=b'\x48\x83\xec\x38' if stack_args else b'\x48\x83\xec\x28'
    if stack_args:
        for source,dest in [(0x60,0x20),(0x68,0x28),(0x70,0x30)]:
            code+=b'\x48\x8b\x44\x24'+bytes([source])+b'\x48\x89\x44\x24'+bytes([dest])
    assert len(code)<=site-start
    code+=b'\x90'*(site-start-len(code))
    relay=G+0x108000+(0x100 if stack_args else 0x200)
    jump(relay,target)
    code+=b'\xe8'+struct.pack('<i',relay-(G+site+5))
    code+=b'\x48\x83\xc4'+bytes([0x38 if stack_args else 0x28])+b'\xc3'
    u.mem_write(G+start,code)

jump(G+0x8361b70,exports['DriTestEmergencyNativeFill'])
jump(G+0x8345870,exports['DriTestEmergencyNativeBuilder'])
call_thunk(0x8336290,0x83362d3,exports['car35_builder_hook'],True)
call_thunk(0x8361df0,0x8361e0f,exports['car35_sign_hook'])
api(lambda: records[rq(reg('RDX'))]['scalar'].get(490,1),G+0x8330e30)
signed=[]
def create_offer():
    c,t,_,_=args()
    return record('emergency offer',{3:0,6:5,7:0,11:0},{9:c,12:t})
api(create_offer,G+0x200100)
def sign():
    manager,tr,offr,flag=args();t=rq(tr);o=rq(offr);c=records[o]['refs'][9]
    signed.append((c,t));records[t]['refs'][111]=c
    records[c]['scalar'].update({12:0,16:0,8:records[t]['scalar'][383],10:5,13:5})
    return 0
api(sign,G+0x85aeda0)

def setup(grades,native=None,user=0,occupied=False,flags=0,coach_user=0,status=7,active=1):
    config(active);signed.clear()
    season=record('season',{3:0,4:5,5:7,6:2028,12:0})
    tuning=record('tuning',{118:0,119:0})
    owner=record('owner');manager=record('manager');m=record('module')
    wq(m+0x138,season);wq(m+0x148,tuning);wq(m+0x178,manager);wq(m+0x28,owner);wi(m+0x1f8,240)
    t=record('CPU vacancy',{383:1});other=record('other team',{383:55})
    if user:records[t]['refs'][407]=coach('user')
    if occupied:records[t]['refs'][111]=coach('incumbent',**{'16':0})
    pool=[]
    for i,g in enumerate(grades):
        pool.append(coach(f'candidate {i}',**{'0':coach_user,'12':status,'16':1,'17':1,'112':g,'134':0,'490':1 if native is None else native[i]}))
    p=alloc(max(8,len(pool)*8))
    for i,c in enumerate(pool):wq(p+i*8,c)
    call('DriTestEmergencySet',p,len(pool),flags,other)
    return m,t,pool

def run(m,t):
    result=call('car35_fill_staff_hook',m,ref(t),1,0)&255
    assert tls.get(73,0)==0
    assert_refs()
    return result

def stat(n):return call('DriTestEmergencyStat',n)

m,t,p=setup([8,7,6]);assert run(m,t)==1 and signed==[(p[2],t)] and stat(0)==1
check('C+ chosen before lower grades regardless of pool order; no fallback attempt when C+ survives native checks')
m,t,p=setup([8,7,6],native=[1,1,0]);assert run(m,t)==1 and signed==[(p[1],t)]
assert [stat(i) for i in [0,1,2,5,6,7,8,9,11]]==[2,6,7,7,6,6,6,1,6]
assert stat(10)==2
check('Native-rejected C+ escalates to C, stops before C-, authorizes only matching direct emergency sign once')
m,t,p=setup([8,7,6],native=[1,0,0]);assert run(m,t)==1 and signed==[(p[0],t)]
assert [stat(i)for i in [0,1,2,3,5,9]]==[3,6,7,8,8,1]
check('C- reached only after C+ and C yield no native-eligible candidate')
for grades,native in [([9,10,11,12,13,-1],None),([8,7,6],[0,0,0]),([],None)]:
    m,t,p=setup(grades,native);assert run(m,t)==0 and not signed and stat(0)==3
check('D+/D/D-/F/invalid and native-rejected pools leave vacancy explicit after three attempts; no lower floor')
for kw in [{'user':1},{'occupied':True},{'coach_user':1},{'status':0},{'status':4}]:
    m,t,p=setup([7,8],**kw);assert run(m,t)==0 and not signed
    assert stat(0)==(1 if 'user'in kw or 'occupied'in kw else 3)
check('User schools, occupied HC slots, user coaches and employed/renewing coaches never receive lower-grade permission')
m,t,p=setup([7,8],active=0);run(m,t);assert stat(0)==1 and stat(5)==6 and stat(9)==0
check('Observe mode preserves native behavior and creates no emergency permission')
for flags in [1,2,4]:
    m,t,p=setup([7,8],flags=flags);assert run(m,t)==0 and not signed and stat(0)==1
check('Malformed vector, invalid build context and changed owner stop escalation immediately')
m,t,p=setup([7]);run(m,t)
# A new team, a fresh invocation, and an ordinary eligibility check must not
# reuse the prior hire permission (including a reloaded record clone).
records[p[0]]['scalar'].update({12:7,16:1,112:7})
assert call('DriTestCar35Eligible',p[0],0,0)==0
copy=coach('reloaded',**{str(k):v for k,v in records[p[0]]['scalar'].items()})
assert call('DriTestCar35Eligible',copy,0,0)==0
assert call('DriTestEmergencyFloor',m,t,copy,0)==6
check('Permission cleared after return and cannot attach to an ordinary search or reloaded record')
m,t,p=setup([7],flags=8);assert run(m,t)==1 and stat(12)==1 and stat(5)==7
check('Nested native fill call restores outer permission without exposing it to the nested call')
import car35_fixture as fixture
normal_win=fixture.win
fail_kind='';tls_sets=0
def failing_win(name):
    global tls_sets
    if name=='TlsSetValue' and reg('RCX')==73:
        tls_sets+=1
        if (fail_kind=='clear' and tls_sets==1) or (fail_kind=='install' and reg('RDX')!=0) or (fail_kind=='restore' and tls_sets==3):return 0
    if name=='HeapAlloc' and fail_kind=='allocate':return 0
    return normal_win(name)
for failure in ['clear','install','allocate','restore']:
    m,t,p=setup([7]);fail_kind=failure;tls_sets=0;fixture.win=failing_win
    result=call('car35_fill_staff_hook',m,ref(t),1,0)&255
    fixture.win=normal_win
    if failure=='restore':
        assert result==1 and tls.get(73,0)
        disabled=tls[73];assert ri(disabled+48)==0
        assert all(rq(disabled+i*8)==0 for i in range(6))
        assert call('DriTestEmergencyFloor',m,t,p[0],0)==6
    else:assert result==0 and not signed and tls.get(73,0)==0
    assert_refs()
check('TLS entry/install/restore and allocation failures cannot leak a live exception or a freed record pointer')
assert_refs()
result={'passed':True,'checks':checks,'production_callbacks_compiled':True,'native_services_modeled':True,'live_acceptance':False}
(ROOT/'emergency-workflow-results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
