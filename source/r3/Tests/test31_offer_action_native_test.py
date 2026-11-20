"""Universal offer authorization: compiled callback/assembly and captured
native payment preflight. Native game record/selection services are modeled.
"""
from pathlib import Path
import json,copy,struct
exec(compile((Path(__file__).parent/'test29_portal_fixture.py').read_text(),'portal fixture','exec'))
records[season]['scalar'].update({4:3,5:6})
records[tuning]['ref'][49]=record('actions',array=[record('scholarship',{0:3,2:5,5:1,6:1}),record('social',{0:5,1:10,2:5,5:1,6:0})])
for tid,t in enumerate(fixture_dummy_teams):
 records[records[t]['ref'][224]]['scalar'].update({1:0,2:0,3:100 if 10<=tid<28 else 0})
records[board]['scalar'][3]=0
hot,ht,hp=pr(31201,80,stage=0,portal=False)
records[hot]['scalar'].update({0:0,14:0})
records[records[hot]['ref'][13]]['array']=[]
records[hp]['scalar'][231]=3
call(exports['DriTestHs32Begin'],context)
assert call(exports['DriTestQualityFault'])==0
authorized=[tid for tid in range(10,28)if call(exports['DriTestHs30Authorized'],hot,tid)]
assert len(authorized)==16,authorized
call(exports['DriTest31OfferRegister'],context)
school_holder=alloc(8);target_holder=alloc(8);wq(target_holder,ht)
f=alloc(0x200);wq(f+104,manager);wq(f+96,school_holder);wq(f+56,target_holder);wi(f+128+0x28,3)
before=copy.deepcopy(records)
for tid in range(10,28):
 wq(school_holder,fixture_dummy_teams[tid]);records[ht]['scalar'][12]=0
 expected=int(tid not in authorized)
 assert call(exports['DriTest31OfferAction'],f)==expected
 assert call(exports['DriTest31OfferAction'],f)==expected,'same school cannot create another reservation'
assert all(records[r]==x for r,x in before.items()if r!=ht)
# Repeat the same actual action gate in a regular-season market snapshot.
records[season]['scalar'][5]=1;call(exports['DriTestHs32Reset']);call(exports['DriTestHs32Begin'],context)
regular=[tid for tid in range(10,28)if call(exports['DriTestHs30Authorized'],hot,tid)]
assert len(regular)==16
for tid in range(10,28):
 wq(school_holder,fixture_dummy_teams[tid]);assert call(exports['DriTest31OfferAction'],f)==int(tid not in regular)
records[season]['scalar'][5]=6;call(exports['DriTestHs32Reset']);call(exports['DriTestHs32Begin'],context)
assert authorized==[tid for tid in range(10,28)if call(exports['DriTestHs30Authorized'],hot,tid)]
blocked=next(t for t in range(10,28)if t not in authorized)
wq(school_holder,fixture_dummy_teams[blocked]);records[ht]['scalar'][12]=3
assert call(exports['DriTest31OfferAction'],f)==0,'legitimate old offer is grandfathered'
records[ht]['scalar'][12]=0
records[fixture_dummy_teams[blocked]]['ref'][407]=record('human coach')
assert call(exports['DriTest31OfferAction'],f)==0
records[fixture_dummy_teams[blocked]]['ref'].pop(407)
wi(f+128+0x28,5);assert call(exports['DriTest31OfferAction'],f)==0;wi(f+128+0x28,3)
records[season]['scalar'][4]+=1
assert call(exports['DriTest31OfferAction'],f)==1,'old phase cannot authorize new offers'
records[season]['scalar'][4]-=1

# Exact entry ABI: branch must preserve original arguments, all nonvolatile
# GPRs and all XMM registers. The permitted native body is a controlled service.
call(exports['DriTest31OfferHook']);hook=reg('RAX');continued=[]
map_game(0x0819adf0,0x1000)
uc.mem_write(GAME+0x0819adf0,b'\xff\x25\0\0\0\0'+struct.pack('<Q',hook)+b'\x90')
def native_continuation():
 assert args()==(manager,school_holder,target_holder,0xabc123)
 sp0=reg('RSP');assert rq(sp0+0x18)==target_holder and rq(sp0+0x20)==0xabc123
 continued.append(True);return 1
game_api(0x0819adff,native_continuation)
entry_sp=STACK+0xf0000-8
for tid,expected in [(blocked,0),(authorized[0],1)]:
 wq(school_holder,fixture_dummy_teams[tid]);wi(entry_sp+0x28,3);wi(entry_sp+0x30,4)
 saved={name:0x60000000+i*0x101 for i,name in enumerate(['RBX','RBP','RSI','RDI','R12','R13','R14','R15'])}
 for name,value in saved.items():uc.reg_write(globals()['UC_X86_REG_'+name],value)
 for i in range(16):uc.reg_write(globals()['UC_X86_REG_XMM'+str(i)],(i+1)*0x1122334455667788)
 assert call(GAME+0x0819adf0,manager,school_holder,target_holder,0xabc123)==expected
 assert all(reg(name)==value for name,value in saved.items())
 assert all(uc.reg_read(globals()['UC_X86_REG_XMM'+str(i)])==(i+1)*0x1122334455667788 for i in range(16))
assert len(continued)==1

# The real common payment preflight returns error7 before any native budget
# or target write when our scholarship authorization is denied.
map_game(0x081b75d0,0x1000)
uc.mem_write(GAME+0x081b75d0,raw[0x081b75d0-0x1000:0x081b7611-0x1000])
wq(school_holder,fixture_dummy_teams[blocked]);wi(entry_sp+0x38,3);wq(entry_sp+0x30,0xabc123)
before=copy.deepcopy(records)
assert call(GAME+0x081b75d0,manager,school_holder,target_holder,0)==7
assert records==before and len(continued)==1
# Dormant portal offers still occupy the hard10 ceiling. They need not have
# a positive score, current funding or remaining school budget to be counted.
portal,portal_target,portal_player=pr(31202,88,pos=18,stage=0,portal=True)
records[portal]['scalar'].update({0:1,14:10})
records[records[portal]['ref'][13]]['array']=[]
for tid in range(10,20):
 valid_offer(portal,destination=fixture_dummy_teams[tid])
 records[records[fixture_dummy_teams[tid]]['ref'][224]]['scalar'][3]=0
def portal_scores():
 scorer,selection,is_portal,tr=args();candidate=rq(reg('RSP')+0x30)
 tid=records[rq(tr)]['scalar'][383]
 wi(candidate+40,300 if tid>=20 else 0);return 0
game_api(0x08185840,portal_scores)
call(exports['DriTestMarket29PortalBegin'],context)
assert call(exports['DriTestQualityFault'])==0
wq(target_holder,portal_target);records[portal_target]['scalar'][12]=0
for tid in range(20,28):
 wq(school_holder,fixture_dummy_teams[tid])
 assert call(exports['DriTest31OfferAction'],f)==1,'dormant actual offers must consume hard10 space'
records[portal_target]['scalar'][12]=3
assert call(exports['DriTest31OfferAction'],f)==0,'never strip a legitimate old offered pursuit'
records[portal_target]['scalar'][12]=0
call(exports['DriTest31FundingReset'])
assert call(exports['DriTest31OfferAction'],f)==1,'untracked CPU route is explicitly denied'
call(exports['DriTestOwnershipReset']);call(exports['DriTestHsPlanReset']);call(exports['DriTestCommitReset'])
assert all(x==1 for x in refs.values()),[(records[r]['name'],x)for r,x in refs.items()if x!=1]
checks=['18 fresh CPU schools share exactly16 frozen HS authorizations across regular/offseason action creation; repeated calls do not mint permissions',
 'Ten dormant portal scholarships block all fresh offers even with negative native score or zero funding; existing offers remain grandfathered',
 'Established offered pursuits, user school and non-scholarship actions retain native eligibility',
 'Stale phase and missing evaluator cannot silently bypass authorization',
 'Actual entry hook preserves original Windows x64 arguments/nonvolatile registers/all16XMM with aligned save area',
 'Captured common native payment preflight returns error7 before any budget or scholarship write on denied new offer']
(OUTPUT/'test31_offer_action_native_results.json').write_text(json.dumps({'checks':checks,'captured_native_payment_preflight':'081b75d0..081b7611','permitted_eligibility_body':'modeled','live_game_verified':False},indent=2)+'\n')
print('\n'.join('PASS '+x for x in checks))
