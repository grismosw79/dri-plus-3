"""Execute compiled R4 staging/restore/planner with controlled SDF/native services.

UMass uses the observed TE values; both missed-school IDs exercise an unrelated
board recruit's observed position/stage reset. Transport is modeled, not Win32.
"""
from pathlib import Path
exec(compile((Path(__file__).parent/'test31_class_fixture.py').read_text(),'class fixture','exec'))
checks=[];plan_out=alloc((4+128*4)*4)
def plan():
 n=call(exports['DriTest34R2PlanSnapshot'],team,context,plan_out);assert 0<=n<=128
 a=list(struct.unpack('<'+str(4+n*4)+'i',uc.mem_read(plan_out,(4+n*4)*4)))
 return {a[i]:a[i+1:i+4] for i in range(4,len(a),4)}
def progress(r,points,gain):
 target=next(t for t in records[arr]['array'] if records[t]['ref'][10]==r)
 for j,e in enumerate(records[records[r]['ref'][13]]['array']):
  records[e]['scalar'][0]=TEST_TEAM_ID if j==0 else 80+j
  records[e]['scalar'][1]=points if j==0 else 100-j
 records[target]['scalar'].update({12:3,7:gain,8:points,9:points});return target
def prepare(tid=119):
 global TEST_TEAM_ID
 TEST_TEAM_ID=tid;reset();call(exports['DriTestHsPlanReset']);call(exports['DriTest34R3CacheIO'],1)
 records[franchise]['scalar'][28]=3184391076;records[season]['scalar'].update({4:10,5:1})
 records[teams]['array']=dummy_teams.copy();records[teams]['array'][tid]=team
 for e,i in zip(ideal_entries,[4,5,4,6,5,3,3,3,3,3,4,4,6,2,4,4,8,4,4,2,2,0]):records[e]['scalar'][2]=i
 records[board]['scalar'].update({1:0,2:0,3:1500})
 primary,_=prospect(pos=4,ovr=61,stars=1,stage=4);records[primary]['scalar'][3]=321;progress(primary,224,40)
 unrelated,_=prospect(pos=16 if tid==119 else 18,ovr=68,stars=2,stage=1);progress(unrelated,117,20)
 def score():
  candidate=rq(reg('RSP')+0x30);r=rq(candidate);wi(candidate+40,380 if r==primary else 410);return 0
 game_api(0x08185840,score)
 assert plan()[ri(primary+0x18)][0]==1
 backup,_=prospect(pos=4,ovr=60,stars=1,stage=0);records[backup]['scalar'][3]=364;progress(backup,241,40)
 warm=plan();assert warm[ri(primary+0x18)][0]==1 and warm[ri(backup+0x18)][0]==2,warm
 call(exports['DriTest34R3Checkpoint'],context)
 call(exports['DriTestHsPlanReset']);records[season]['scalar'][4]=11
 return primary,backup,unrelated
def roles(primary,backup):
 r=plan();return r[ri(primary+0x18)][0],r[ri(backup+0x18)][0]
def snapshot_fields():return {r:dict(v['scalar']) for r,v in records.items()}
for tid in [119,18]:
 primary,backup,unrelated=prepare(tid)
 records[unrelated]['scalar'][9]=0
 assert roles(primary,backup)==(2,1),'Control must reproduce late-lookup loss'
 records[unrelated]['scalar'][9]=1;call(exports['DriTestHsPlanReset'])
 before=snapshot_fields();call(exports['DriTest34R4Preload'],context)
 assert snapshot_fields()==before,'Preload wrote native fields'
 assert call(exports['DriTest34R4Preloaded'],team,context)==1
 records[unrelated]['scalar'][9]=0
 before=snapshot_fields();assert roles(primary,backup)==(1,2)
 assert snapshot_fields()==before,'Restore/planning wrote native fields'
 checks.append(f'School {tid}: earlier unrelated stage1-to0 reproduces late-lookup loss; preload retains primary with no influence/offer/budget/stage writes')

# Scope and cache refusal controls preserve the strict original matching rule.
for why in ['checksum','foreign_cache','future_cache','changed_board']:
 primary,backup,unrelated=prepare()
 if why=='changed_board':records[unrelated]['scalar'][9]=0
 else:
  if why=='future_cache':records[season]['scalar'][4]=10
  call(exports['DriTest34R3CacheCorrupt'],{'checksum':0,'foreign_cache':1,'future_cache':2}[why])
 call(exports['DriTest34R4Preload'],context)
 assert call(exports['DriTest34R4Preloaded'],team,context)==0,why
 assert roles(primary,backup)==(2,1),why
 checks.append(why+': rejected before maintenance; normal selection remains available')

for field,value in [(4,12),(5,2),(2,2027)]:
 primary,backup,unrelated=prepare();call(exports['DriTest34R4Preload'],context)
 records[season]['scalar'][field]=value
 assert call(exports['DriTest34R4Preloaded'],team,context)==0
checks.append('Preloaded roles cannot cross week, phase or year')
primary,backup,unrelated=prepare();call(exports['DriTest34R4Preload'],context)
records[franchise]['scalar'][28]+=1
assert call(exports['DriTest34R4Preloaded'],team,context)==0
checks.append('Foreign league in a reused fixture context cannot consume staged roles')

primary,backup,unrelated=prepare();call(exports['DriTest34R4Preload'],context)
records[unrelated]['scalar'][9]=0
game_api(0x0816b130,lambda:0 if rq(reg('R8'))==primary else fixture_eligibility())
assert roles(primary,backup)==(0,1)
game_api(0x0816b130,fixture_eligibility)
checks.append('A freshly ineligible retained primary is still refused and backup can promote')

primary,backup,unrelated=prepare()
records[team]['ref'][407]=record('user coach')
call(exports['DriTest34R4Preload'],context)
assert call(exports['DriTest34R4Preloaded'],team,context)==0
records[team]['ref'].pop(407)
checks.append('User school is excluded from CPU preloading')
for mode_value in [0,0xffffffff]:
 primary,backup,unrelated=prepare();configure(mode_value)
 before=snapshot_fields();call(exports['DriTest34R4Preload'],context)
 assert call(exports['DriTest34R4Preloaded'],team,context)==0 and snapshot_fields()==before
checks.append('Diagnostic/disabled modes neither preload nor write native state')
configure(1);call(exports['DriTest34R3CacheIO'],0);call(exports['DriTestHsPlanReset'])
call(exports['DriTestOwnershipReset'])
assert call(exports['DriTestQualityFault'])==0, 'Runtime faulted'
assert all(v==1 for v in refs.values()), [(records[r]['name'],v) for r,v in refs.items() if v!=1]
(OUTPUT/'test34_r4_preload_runtime_results.json').write_text(json.dumps({'passed':True,'checks':checks,'compiled_runtime_executed':True,'native_services_modeled':True,'sidecar_transport_modeled':True,'live_game_verified':False},indent=2)+'\n')
print(json.dumps(checks,indent=2))
