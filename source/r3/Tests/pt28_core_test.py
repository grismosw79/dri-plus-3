"""Compile real PT28 C model and compare to approved independent Python model."""
from pathlib import Path
import ctypes as C
import importlib.util,json,math,os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
def reference_annual(ovr, peers, starters, useful, rotation):
    if starters<=0:return 0.
    higher=sum(x>ovr for x in peers);equal=sum(x==ovr for x in peers)
    ordered=sorted(peers,reverse=True)
    gap=ordered[starters-1]-ovr if len(ordered)>=starters else 0
    values=[]
    for rank in range(higher+1,higher+equal+2):
        if rank<=starters:value=12.
        elif rank==starters+1 and gap<=4:value=9.
        elif rank<=useful:value=(7. if rotation else 4.)-2*max(0.,min(1.,(rank-starters-1)/max(1,useful-starters-1)))
        else:value=1.
        values.append(value)
    return sum(values)/len(values)
def reference_combine(current,future):return min(12.,current+min(1.,.2*max(0.,future-current)))
class Player(C.Structure):
    _fields_=[('table',C.c_size_t),('id',C.c_uint32)]+[(n,C.c_int) for n in ['pos','ovr','year','hs','develop','redshirt','arrival_delay','rostered']]
class Room(C.Structure):
    _fields_=[(n,C.c_int) for n in ['starters','useful','rotation','count']]+[('players',Player*384)]
class Result(C.Structure):
    _fields_=[(n,C.c_int) for n in ['grade','rank','equals','starters','useful','rotation','peer_count','future_peer_count','projected_ovr','following_ovr','following_available']]+[(n,C.c_double) for n in ['immediate','following','score']]
output=Path(os.environ.get('DRI_TEST_OUTPUT_DIR',ROOT/'build'));output.mkdir(parents=True,exist_ok=True)
libpath=output/'pt28_core_test.so'
subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC',str(ROOT/'pt28_core.c'),'-lm','-o',str(libpath)],check=True)
lib=C.CDLL(str(libpath));lib.pt28_annual.argtypes=[C.c_int,C.POINTER(C.c_int),C.c_int,C.c_int,C.c_int,C.c_int];lib.pt28_annual.restype=C.c_double
lib.pt28_combine.argtypes=[C.c_double,C.c_double,C.c_int];lib.pt28_combine.restype=C.c_double
lib.pt28_letter.argtypes=[C.c_double];lib.pt28_letter.restype=C.c_int
lib.pt28_compute.argtypes=[C.POINTER(Room),C.POINTER(Player),C.c_int,C.c_int,C.c_int,C.POINTER(Result)]
def annual(o,peers,s,u,rot):
    return lib.pt28_annual(o,(C.c_int*len(peers))(*peers),len(peers),s,u,rot)
checks=0
for s,u,rot in [(0,0,0),(1,2,0),(1,3,1),(2,4,0),(2,6,1),(3,6,1),(4,8,0)]:
    for peers in [[],[80],[80,80],[80,80,80],[75,81,90,95],[90]*8,[99]*40]:
        last=-1
        for rating in range(40,100):
            score=annual(rating,peers,s,u,rot)
            expected=reference_annual(rating,peers,s,u,rot)
            assert abs(score-expected)<1e-12,(rating,peers,s,u,rot,score,expected)
            assert score>=last;last=score
            for added in [50,80,99]:assert annual(rating,peers+[added],s,u,rot)<=score+1e-12
            for future in range(13):
                final=lib.pt28_combine(score,future,1)
                assert abs(final-reference_combine(score,future))<1e-12
                assert score<=final<=min(12,score+1)
            checks+=1
approved=json.loads((Path(__file__).parent/'pt28_approved_cases.json').read_text())['rows']
for case in approved:
    id,rating,gap,ahead,starters,useful,rotation,expected=case
    score=annual(rating,[rating+gap]*ahead,starters,useful,rotation)
    assert score==expected,(id,score,expected)
def p(id,ovr,year=1,hs=0,redshirt=0):return Player(123,id,0,ovr,year,hs,not hs,redshirt)
def compute(candidate,peers,advance=1,s=1,u=2,rot=0,development=None):
    room=Room(s,u,rot,len(peers));room.players[:len(peers)]=peers;out=Result()
    assert lib.pt28_compute(C.byref(room),C.byref(candidate),2,advance,advance if development is None else development,C.byref(out))
    return out
# One remaining year uses the same immediate job score as a younger player.
junior=compute(p(1,85,2),[]);soph=compute(p(1,85,1),[])
assert junior.immediate==soph.immediate==junior.score==soph.score==12
assert not junior.following_available and soph.following_available
# Senior/graduating competitors are removed only when projecting across the boundary.
assert compute(p(1,85),[p(2,95,3)],advance=1).immediate==12
assert compute(p(1,85),[p(2,95,3)],advance=0).immediate==4
# Native annual advance preserves the year of actively redshirting players.
redshirt=compute(p(1,85,2,redshirt=1),[p(2,95,2,redshirt=1)])
assert redshirt.following_available and redshirt.peer_count==1 and redshirt.future_peer_count==1
# Training can finish before class rollover; no second development increment.
trained=compute(p(1,85,2),[p(2,95,3)],advance=1,development=0)
assert trained.projected_ovr==85 and trained.following_ovr==87 and not trained.following_available and trained.peer_count==0
# Candidate identity is excluded; tied identities get identical occupied-rank means.
a=compute(p(1,80),[p(1,80),p(2,80)]);b=compute(p(2,80),[p(1,80),p(2,80)])
assert a.peer_count==b.peer_count==1 and a.immediate==b.immediate==10.5
# An HS/JUCO entrant does not receive college development or class advancement.
hs=compute(p(1,80,0,hs=1),[p(2,80)],advance=1)
juco=compute(p(1,80,2,hs=1),[],advance=1)
assert hs.projected_ovr==80 and juco.following_available
assert compute(p(1,99),[]).projected_ovr==99
future_hs=p(5,95,0,hs=1);future_hs.arrival_delay=1
current=compute(p(1,80,1),[future_hs],advance=0,development=0)
assert current.peer_count==0 and current.future_peer_count==1 and current.immediate==12 and current.following==4
assert current.score==12
# A junior with a blocked job gets no invented future rescue; younger bonus is <=1.
blocked_junior=compute(p(1,70,2),[p(2,95,2),p(3,90,2)])
blocked_soph=compute(p(1,70,1),[p(2,95,2),p(3,90,2)])
assert blocked_junior.score==1 and blocked_soph.score==2
assert annual(80,[],0,0,0)==0 and lib.pt28_letter(0)==12
for i in range(12):assert lib.pt28_letter(i+.499)==12-i and lib.pt28_letter(i+.5)==11-i
for score in [-1,13,float('nan'),float('inf')]:assert lib.pt28_letter(score)==-1
report={'controlledComparisons':len(approved),'propertyInputCases':checks,'checks':['approved model parity','rating and competition monotonicity','equal-rating tie averaging','no-grade boost by class alone','bounded future opportunity','last-season starter','current versus projected graduation','native active-redshirt class preservation','HS/JUCO class and development handling','self identity removal','0-12 native grade boundaries'],'live_game_verified':False}
(output/'pt28_core_results.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
