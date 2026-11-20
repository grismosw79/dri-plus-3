"""Compiled contract callbacks + captured native removal; SDF/OS modeled.
The accepted terms are the oracle, never the reconstructed finalizer input.
"""
from car35_fixture import *
from copy import deepcopy

native_removes=[];native_withdraws=[];native_lookups=[]
def remove():
 a,i,_,_=args();rows=records[a]['array'];tail=rows.pop()
 if i<len(rows):rows[i]=tail
 native_removes.append((a,i));return i
api(remove,G+0x8052780)
api(lambda:records[reg('RCX')]['array'].clear(),G+0x8047250)
api(lambda:991,G+0x843e7b0)
def lookup():
 native_lookups.append(args());wq(reg('RDX'),0);return reg('RDX')
api(lookup,G+0x833f0a0)
def withdraw():
 native_withdraws.append(args());decref(rq(reg('R9')));return 7
api(withdraw,G+0x8356c00)

def setup(role=0,term=3,user=1,team_user=0,status=0):
 c=coach(**{'0':user,'12':6,'128':2});t=record('Purdue',{383:76},ref={407:c} if team_user else {})
 o=record('original accepted offer',{3:role,6:term,7:190,11:status},ref={9:c,12:t})
 other=record('other pending offer',{3:role,6:5,7:80,11:2},ref={9:coach(),12:t})
 a=record('local offers',array=[other,o]);j=record('selected job',{1:1,2:190,6:role},ref={0:a,9:c,10:t})
 g=record('global outstanding',array=[]);m=record('hiring',ref={21:record('jobs',array=[j]),24:g})
 w=alloc(32);wq(w+8,j)
 return c,t,o,other,a,j,m,w

# Each role, human coach / human employer / CPU, with arbitrary original terms.
cases=0
for role in [0,1,2]:
 for user,owner in [(1,0),(1,1),(0,1),(0,0)]:
  for term in [1,2,3,5]:
   c,t,o,other,a,j,m,w=setup(role,term,user,owner,0 if user else 2)
   assert call('car35_contract_remove_hook',a,1,ref(j),m)==1
   assert records[a]['array']==[other,o] and records[o]['scalar'][11]==0
   for _ in range(3):call('car35_contract_clear_hook',a,j,m)
   assert records[a]['array']==[o]
   assert call('car35_contract_max_hook',w)==0
   assert records[c]['scalar'][128]==2  # callbacks do not write offer counters
   for _ in range(2):
    incref(a);assert call('car35_contract_withdraw_hook',m,ref(j),ref(o),ref(a),ref(c),1)==1
   out=ref(0);assert call('car35_contract_lookup_hook',m,out,ref(t),role,ref(c),2,1)==out
   assert rq(out)==o and records[o]['scalar'][6]==term and records[o]['scalar'][7]==190
   decref(o);assert_refs();cases+=1
check(f'{cases} accepted-term lifecycles: all roles, human/CPU ownership, 1/2/3/5-year terms, repeated cleanup, zero duplicate reservation')

# Rehydrate a retained human agreement using only saved fields/references.
c,t,o,other,a,j,m,w=setup();call('car35_contract_clear_hook',a,j,m)
saved={k:deepcopy(records[v]['scalar']) for k,v in [('coach',c),('team',t),('offer',o),('job',j)]}
config()
c2=record('reloaded coach',saved['coach']);t2=record('reloaded team',saved['team'])
o2=record('reloaded offer',saved['offer'],ref={9:c2,12:t2});a2=record('reloaded offers',array=[o2])
j2=record('reloaded job',saved['job'],ref={0:a2,9:c2,10:t2});m2=record('reloaded module',ref={21:record('reloaded jobs',array=[j2])})
# Native finalization releases the prior contract before reconstruction.
records[c2]['scalar'].update({12:7,8:255})
out=ref(0);call('car35_contract_lookup_hook',m2,out,ref(t2),0,ref(c2),2,1)
assert rq(out)==o2 and records[o2]['scalar'][6]==3 and records[o2]['scalar'][7]==190
decref(o2);assert_refs()
check('Purdue 3-year/190 agreement survives record rehydration and pre-sign free-agent transition without process-side contract storage')

# Pending human offers must never be autoaccepted. Native statuses other than
# Accepted/Pending, invalid or superseded selection, and ambiguity stay native.
for label in ['pending_user','declined','other_status','unfilled','coach','team','role','points','length','duplicate']:
 c,t,o,other,a,j,m,w=setup()
 records[a]['array']=[o]
 if label=='pending_user':records[o]['scalar'][11]=2
 if label=='declined':records[o]['scalar'][11]=1
 if label=='other_status':records[o]['scalar'][11]=3
 if label=='unfilled':records[j]['scalar'][1]=0
 if label=='coach':records[j]['refs'][9]=coach()
 if label=='team':records[j]['refs'][10]=record('other team',{383:5})
 if label=='role':records[o]['scalar'][3]=1
 if label=='points':records[o]['scalar'][7]=191
 if label=='length':records[o]['scalar'][6]=0
 if label=='duplicate':records[a]['array'].append(o)
 before=deepcopy(records[o]['scalar'])
 incref(a);n=len(native_withdraws)
 assert call('car35_contract_withdraw_hook',m,ref(j),ref(o),ref(a),ref(c),1)==7
 assert len(native_withdraws)==n+1
 call('car35_contract_clear_hook',a,j,m)
 assert records[a]['array']==[] and records[o]['scalar']==before
 out=ref(0);call('car35_contract_lookup_hook',m,out,ref(t),0,ref(c),2,1);assert rq(out)==0
 assert_refs()
check('Ten nonaccepted/cancelled/superseded/ambiguous controls use native withdrawal/cleanup; no stale terms recreated')

# Supersession after retention, changed ownership, and foreign array controls.
c,t,o,other,a,j,m,w=setup();call('car35_contract_clear_hook',a,j,m)
records[j]['refs'][9]=coach()
out=ref(0);call('car35_contract_lookup_hook',m,out,ref(t),0,ref(c),2,1);assert rq(out)==0
c,t,o,other,a,j,m,w=setup();call('car35_contract_clear_hook',a,j,m)
foreign=record('unrelated array',array=[o]);incref(foreign)
assert call('car35_contract_withdraw_hook',m,ref(j),ref(o),ref(foreign),ref(c),1)==7
records[t]['refs'][407]=c;records[c]['scalar'][0]=0
out=ref(0);call('car35_contract_lookup_hook',m,out,ref(t),0,ref(c),2,1);assert rq(out)==o;decref(o)
config(0);call('car35_contract_clear_hook',a,j,m);assert not records[a]['array'];config();assert_refs()
check('Changed selected coach cannot revive an old agreement; control transfer preserves valid selection; observe mode remains native')

# Run the actual captured native per-offer removal around the compiled hook.
# It decrements NumContractOffers once, even when local retention holds the row.
cap=Path(os.environ['DRI_CAPTURE_DIR']);raw=(cap/'section_00_00001000.bin').read_bytes()
native=0x8356c00;u.mem_write(G+native,raw[native-0x1000:native-0x1000+336]);handlers.pop(G+native)
# Earlier negative controls executed the modeled one-byte function at this
# address. Discard that emulator translation before executing captured bytes.
u.ctl_remove_cache(G+native,G+native+336)
relay=G+0x106000;u.mem_write(relay,b'\xff\x25\0\0\0\0'+struct.pack('<Q',call('DriTestCar35Address',6)))
site=0x8356c3a;u.mem_write(G+site,b'\xe8'+struct.pack('<i',relay-G-site-5))
def find():
 rows=records[reg('RCX')]['array'];o=rq(reg('RDX'));return rows.index(o) if o in rows else 0xffffffff
api(find,G+0x80531a0)
def wrapper():
 out,cr,_,_=args();cc=rq(cr);incref(cc);wq(out,VT);wq(out+8,cc);wq(out+16,SCHEMA);return out
api(wrapper,G+0x8606aa0);api(lambda:0,G+0x8327df0)
c,t,o,other,a,j,m,w=setup();records[a]['array']=[o];incref(a)
assert call(G+native,m,ref(j),ref(o),ref(a),ref(c),1)&255==1
assert records[a]['array']==[o] and records[c]['scalar'][128]==1
incref(a);assert call('car35_contract_withdraw_hook',m,ref(j),ref(o),ref(a),ref(c),1)==1
assert records[c]['scalar'][128]==1
assert_refs();check('Captured native removal decrements the live pending counter exactly once; repeated accepted cleanup preserves the remaining unrelated offer count')

result={'passed':True,'checks':checks,'lifecycle_cases':cases,'compiled_callbacks':True,'native_removal_executed':True,'OS_SDF_modeled':True,'live_save_reload':False}
(ROOT/'r10_contract_results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
