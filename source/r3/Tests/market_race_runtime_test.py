"""Test14 read-only race metadata through the compiled Windows callback.
SDF/Windows storage are fixtures; rank/lockout runs captured native code.
"""
exec(compile((__import__('pathlib').Path(__file__).parent/'portal_runtime_test.py').read_text(),'portal_runtime_test.py','exec'))
import ctypes as C

class Choice(C.Structure):
 _fields_=[(k,C.c_int) for k in ['pos','role','score','deficit','existing','round','selected','index']]+[
  ('rid',C.c_uint),('rec',C.c_uint64)]+[(k,C.c_int) for k in [
   'ovr','slot','priority','native_base','plan_score','offered','influence','leader','commit_score','stage','race_ok','raw_ovr']]
assert C.sizeof(Choice)==96
configure(1)
records[tplist]['array']=[];records[hslist]['array']=[];records[committed]['array']=[];records[arr]['array']=[]
rec,target,player=pr(43000,74,stage=1)
records[rec]['scalar'][3]=120;records[target]['scalar'].update({12:3,8:999})
wq(context+0xd0,tuning)
top=records[rec]['ref'][13]
def schools(values):
 records[top]['array']=[record('race school '+str(tid),{0:tid,1:points}) for tid,points in values]
def read(existing=1):
 p=Choice();p.rec=rec;p.rid=43000;p.existing=existing;p.native_base=660;p.score=920;p.plan_score=920;p.ovr=76;p.raw_ovr=74
 address=alloc(C.sizeof(p));uc.mem_write(address,bytes(p))
 result=call(exports['DriTestPortalChoiceRace'],address,target,team,context)
 return result,Choice.from_buffer_copy(bytes(uc.mem_read(address,C.sizeof(p))))

schools([(TEST_TEAM_ID,89),(10,43)])
ok,p=read()
assert ok and (p.offered,p.influence,p.leader,p.commit_score,p.stage,p.race_ok)==(1,89,89,120,1,1)
assert (p.native_base,p.score,p.plan_score)==(660,920,920)
checks=['Real offered leader retains exact native score/base and reads 89/89 race progress against CommitScore120']

records[target]['scalar'][12]=1
ok,p=read();assert ok and p.offered==0 and p.race_ok==1
records[target]['scalar'][12]=3
ok,p=read(existing=0);assert ok and p.offered==0 and p.race_ok==1
checks.append('New/unoffered targets read seeded interest and native rank eligibility without manufacturing an offer')

schools([(10+i,100-i) for i in range(5)]+[(TEST_TEAM_ID,89)])
ok,p=read();assert ok and p.offered==1 and p.race_ok==0
checks.append('Captured native Top5 lockout rejects a still-offered school outside the finalists')

records[rec]['scalar'][9]=0;records[target]['scalar'][8]=89
schools([(10,70),(11,60)])
ok,p=read();assert ok and (p.influence,p.leader,p.race_ok)==(89,89,1)
checks.append('Reopened Top10 offer outside the interest list uses earned target influence and conservative max leader')

schools([(10,90),(TEST_TEAM_ID,12)])
records[target]['scalar'][8]=999
ok,p=read();assert ok and (p.influence,p.leader)==(12,90)
checks.append('Listed influence remains authoritative over a larger target total; no fabricated race lead')

records[rec]['scalar'][9]=5
ok,p=read();assert ok and p.offered==0 and p.race_ok==0
records[rec]['scalar'][9]=1
schools([(TEST_TEAM_ID,89),(TEST_TEAM_ID,43)])
ok,p=read();assert not ok
checks.append('Secured recruits receive no retention protection and duplicate school identities are refused')

call(exports['DriTestOwnershipReset'])
assert all(v==1 for v in refs.values()),[(records[r]['name'],v) for r,v in refs.items() if v!=1]
assert call(exports['DriTestQualityFault'])==0
checks.append('All record references remain balanced; metadata inspection does not mutate game records or fault')
(OUTPUT/'market_race_runtime_results.json').write_text(json.dumps({'checks':checks,'mocked':['SDF storage','Windows APIs'],'native':['rank/lockout helpers'],'live_game_verified':False},indent=2)+'\n')
print(json.dumps(checks,indent=2))
