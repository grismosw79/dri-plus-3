"""Saved PRE/W12 HB rooms through actual capacity and Playing Time policies.
This does not reproduce native dealbreaker requirements, offers or signings.
"""
from pathlib import Path
import ctypes as C
import json, os, subprocess
exec(compile((Path(__file__).parent/'tu1029r5_pool_test.py').read_text().split('old=build(B,')[0],__file__,'exec'))
fixture=json.loads((S/'Tests/Fixtures/test30_academy_hb.json').read_text())
capacity=build(S,'academy_capacity')
pt=O/'playing_time.so'
subprocess.run(['gcc','-std=c11','-shared','-fPIC','-O2','-I'+str(S),str(S/'pt28_core.c'),'-lm','-o',str(pt)],check=True)
class Player(C.Structure):
 _fields_=[('table',C.c_size_t),('id',C.c_uint32)]+[(n,C.c_int)for n in 'pos ovr year hs develop redshirt arrival_delay rostered'.split()]
class Room(C.Structure):
 _fields_=[(n,C.c_int)for n in 'starters useful rotation count'.split()]+[('players',Player*384)]
class Result(C.Structure):
 _fields_=[(n,C.c_int)for n in 'grade rank equals starters useful rotation peer_count future_peer_count projected_ovr following_ovr following_available'.split()]+[(n,C.c_double)for n in ['immediate','following','score']]
lib=C.CDLL(str(pt));calc=lib.pt28_compute
calc.argtypes=[C.POINTER(Room),C.POINTER(Player),C.c_int,C.c_int,C.c_int,C.POINTER(Result)];calc.restype=C.c_int
grades='Aplus A Aminus Bplus B Bminus Cplus C Cminus Dplus D Dminus F'.split()
observations=[]
for t in fixture['cases']:
 ideals=(C.c_int*22)(*t['ideal']);starters=(C.c_int*22)(*t['starters'])
 flat=sum(t['players'],[]);players=(C.c_int*len(flat))(*flat)
 reason=capacity(ideals,starters,players,len(t['players']),t['prestige'],2,1,3,74)
 # HB useful rotation depth is max(3, 2*starters) bounded by ideal.
 room=Room(starters=t['starters'][1],useful=min(t['ideal'][1],max(3,2*t['starters'][1])),rotation=1,count=len(t['hb']))
 for i,p in enumerate(t['hb']):
  room.players[i]=Player(table=1,id=p['pid'],pos=1,ovr=p['ovr'],year=p['year'],hs=p['hs'],develop=p['develop'],redshirt=p['redshirt'])
 candidate=Player(table=1,id=14422,pos=1,ovr=74,year=0,hs=1)
 result=Result();assert calc(C.byref(room),C.byref(candidate),2,1,1,C.byref(result))==1
 observations.append({'checkpoint':t['checkpoint'],'tid':t['tid'],'school':t['name'],'projected_hb_count':sum(p[0]==1 for p in t['players']),'ideal_hb':t['ideal'][1],'hard_hb':t['hard_hb'],'capacity_reason':reason,'playing_time_grade':grades[result.grade],'playing_time_grade_index':result.grade,'rank':result.rank,'immediate':result.immediate,'following':result.following,'score':result.score})
 # The new star permission must not override the real full-room four-star gate.
 if t['tid']==55:assert reason==3 and t['hard_hb']==[1729,762]
assert len(observations)==6
assert [(v['capacity_reason'],v['playing_time_grade_index'])for v in observations]==[(0,10),(0,7),(3,2),(8,10),(0,11),(3,2)]
report={'passed':True,'inputs':fixture['inputs'],'results':observations,'native_eligibility_verified':False,'live_game_verified':False,'scope':fixture['scope']}
(O/'test30_academy_capacity_results.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(observations,indent=2))
