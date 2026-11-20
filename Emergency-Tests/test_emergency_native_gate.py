"""Tiered production workflow with the captured native broad gate executed.
The builder, record services, native signer and Win32 remain modeled.
"""
from test_emergency_workflow import *
checks.clear()
cap=Path(os.environ['DRI_CAPTURE_DIR'])
raw=(cap/'section_00_00001000.bin').read_bytes()
u.mem_write(G+0x8330e30,raw[0x8330e30-0x1000:0x8330e30-0x1000+724])
handlers.pop(G+0x8330e30,None)
relay=G+0x109000;jump(relay,exports['car35_gate_hook'])
u.mem_write(G+0x8330f0c,b'\xe8'+struct.pack('<i',relay-(G+0x8330f0c+5)))
def wrapper():
    out,cref,_,_=args();c=rq(cref);incref(c);wq(out,VT);wq(out+8,c);wq(out+16,SCHEMA);return out
api(wrapper,G+0x8606aa0)
api(lambda:reg('RCX'),G+0x37f5ed0)
gate_seen=[]
def status_gate():
    c=rq(reg('RDX'));v=records[c]['scalar'].get(490,1);gate_seen.append((c,v));return v
api(status_gate,G+0x834eea0)
api(lambda:1,G+0x832cce0)
for r in [0x8346e40,0x832e800,0x8346de0]:api(lambda:0,G+r)
u.ctl_remove_cache(G+0x8330e30,G+0x8331104)
for native,winner,attempts in [([1,1,1],2,1),([1,1,0],1,2),([1,0,0],0,3)]:
    m,t,p=setup([8,7,6],native=native)
    result=run(m,t)
    assert result==1 and signed==[(p[winner],t)] and stat(0)==attempts,(native,result,signed,[stat(i)for i in range(13)],gate_seen)
check('Captured native broad gate participates in C+ -> C -> C- selection, retaining its real acceptance/rejection branches')
m,t,p=setup([7,8]);records[p[0]]['scalar'][128]=1
assert run(m,t)==1 and signed==[(p[1],t)] and stat(0)==3
check('Native outstanding-offer cap can reject C and trigger C-; lowered grade never bypasses that cap')
m,t,p=setup([7,8]);records[p[0]]['scalar'][128]=1;records[p[1]]['scalar'][128]=1
assert run(m,t)==0 and not signed
check('All tiers rejected by native offer cap leave vacancy unfilled')
m,t,p=setup([8,7,8,8])
teams=[t,record('Kent State vacancy',{383:39}),record('ULM vacancy',{383:63})]
for target in teams:assert run(m,target)==1
assert len(signed)==3 and len({c for c,t in signed})==3
assert signed[0][0]==p[1]
assert all(111 in records[target]['refs']for target in teams)
check('Three sequential CPU vacancies use distinct coaches: available C first, then C-; already-hired coaches leave the modeled native free-agent pool')
assert_refs()
result={'passed':True,'checks':checks,'captured_native_broad_gate_executed':True,'production_callbacks_compiled':True,'builder_record_signer_OS_services_modeled':True,'live_acceptance':False}
(ROOT/'emergency-native-gate-results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
