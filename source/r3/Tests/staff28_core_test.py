"""Portable model replay against approved TP4/PLV outputs and real city records."""
from pathlib import Path
import ctypes as C, ctypes.util, json, math, os, subprocess, tempfile
HERE=Path(__file__).resolve().parent;SOURCE=HERE.parent
fixture=json.loads((HERE/'staff28_saved_fixtures.json').read_text())
class Scores(C.Structure):
    _fields_=[('score',C.c_int32*3),('present',C.c_ubyte*3)]
checks=[]
with tempfile.TemporaryDirectory(prefix='staff28-test-') as temp:
    lib=Path(temp)/'staff28.so'
    subprocess.run([os.environ.get('CC','cc'),'-std=c11','-Wall','-Wextra','-Werror','-O2','-shared','-fPIC',str(SOURCE/'staff28_core.c'),'-lm','-o',str(lib)],check=True)
    core=C.CDLL(str(lib));core.staff28_score.argtypes=[C.POINTER(Scores),C.c_int];core.staff28_score.restype=C.c_int64
    core.staff28_compute.argtypes=[C.POINTER(Scores),C.c_uint,C.POINTER(C.c_int*2)]
    for tag,rows in fixture['snapshots'].items():
        scores=(Scores*138)(*[Scores((C.c_int32*3)(*r['scores']),(C.c_ubyte*3)(1,1,1)) for r in rows]);grades=((C.c_int*2)*138)()
        assert core.staff28_compute(scores,138,grades)==1
        for row,got in zip(rows,grades):assert list(got)==row['grades'],(tag,row['school'],list(got),row['grades'])
        checks.append(f'{tag}: all 276 side grades match the approved 138-school model')
    assert [core.staff28_side(p) for p in range(22)]==[0]*10+[1]*9+[-1]*3
    assert core.staff28_side(-1)==core.staff28_side(22)==-1
    cuts=[5,10,15,25,35,45,55,65,75,90,105,120,250]
    for rank in range(251):assert core.staff28_rank_grade(rank)==next(i for i,c in enumerate(cuts) if rank<=c)
    checks.append('All 22 positions and every native grade-band rank verified')
    scores=(Scores*138)();grades=((C.c_int*2)*138)()
    for i in range(138):scores[i]=Scores((C.c_int32*3)(100 if i<8 else 10,0,0),(C.c_ubyte*3)(1,1,1))
    assert core.staff28_compute(scores,138,grades)==1
    assert all(list(grades[i])==[0,0] for i in range(8))
    assert all(list(grades[i])==[1,1] for i in range(8,138))
    checks.append('A tie spanning a grade boundary receives one best-occupied-rank grade')
    s=Scores((C.c_int32*3)(100,0,70),(C.c_ubyte*3)(1,0,1))
    assert core.staff28_score(s,0)==7500 and core.staff28_score(s,1)==9250
    s.present[1]=1;assert core.staff28_score(s,0)==7500
    s.present[0]=0;assert core.staff28_score(s,1)==1750
    s=Scores((C.c_int32*3)(2147483647,2147483647,2147483647),(C.c_ubyte*3)(1,1,1))
    assert core.staff28_score(s,0)==214748364700
    scores[0].score[0]=-1;assert core.staff28_compute(scores,138,grades)==0
    checks.append('Vacancies, legitimate zero scores, malformed negative scores, and 64-bit weighting verified')
    cosine=C.CFUNCTYPE(C.c_float,C.c_float)(('cosf',C.CDLL(ctypes.util.find_library('m'))))
    core.staff28_distance.argtypes=[C.c_float]*4+[type(cosine)];core.staff28_distance.restype=C.c_float
    for r in fixture['seattle']:
        got=core.staff28_distance(47.62110137939453,-122.32440185546875,r['lat'],r['lon'],cosine)
        assert abs(got-r['miles'])<.001,(r['school'],got,r['miles'])
        assert int(got)==int(r['miles'])
    assert core.staff28_distance(47.62110137939453,-122.32440185546875,47.62110137939453,-122.32440185546875,cosine)==0
    checks.append('Seattle distances reproduce all 138 supplied school locations and integer spline inputs')
print(json.dumps({'checks':checks,'live_game_verified':False},indent=2))
