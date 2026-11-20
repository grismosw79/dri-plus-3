"""Replay the reported save through both collectors; test reconciliation limits.

The graph was extracted read-only from the actual autosave. Native ordinals and
ownership callbacks match the captured field descriptors. No game is running.
"""
from pathlib import Path
import ctypes as c
import collections
import gzip
import hashlib
import json
import math
import subprocess

S=Path(__file__).resolve().parents[1]
B=S/'build';B.mkdir(exist_ok=True)
A=S.parent/'Audit/Test28R2'
F=S.parent/'Audit/EPS1/Replay/test27-build'
manifest=json.loads((A/'fixture-manifest.json').read_text())
def fixture(name):
    data=gzip.decompress((A/name).read_bytes())
    assert hashlib.sha256(data).hexdigest()==manifest[name]['uncompressed_sha256']
    assert len(data)==manifest[name]['uncompressed_bytes']
    return data

class Row(c.Structure):
    _fields_=[('team',c.c_int),('opponent',c.c_int),('game',c.c_int)]+[(s,c.c_double) for s in ['pass_yards','pass_attempts','rush_yards','rush_attempts','sacks']]+[('group',(c.c_double*28)*10)]
class Result(c.Structure):
    _fields_=[(s,c.c_int) for s in ['success','rows','teams','players','player_games','fcs_games','reconciliations']]+[('error',c.c_char_p),('schedule_hash',c.c_uint64)]
class Metric(c.Structure):
    _fields_=[(s,c.c_double) for s in ['value','observed','numerator','denominator','prior_exposure','prior_value']]
class Grade(c.Structure):
    _fields_=[('score',c.c_double),('rank',c.c_double),('grade',c.c_int),('status',c.c_int)]
class Output(c.Structure):
    _fields_=[('year',c.c_int),('rows',c.c_int),('teams',c.c_int),('selected_prior_games',c.c_double*4),('exposure',(c.c_double*11)*138),('metric',(Metric*36)*138),('grade',(Grade*23)*138)]
REF=c.CFUNCTYPE(c.c_size_t,c.c_size_t,c.c_int);AT=c.CFUNCTYPE(c.c_size_t,c.c_size_t,c.c_uint)
COUNT=c.CFUNCTYPE(c.c_uint,c.c_size_t);VALUE=c.CFUNCTYPE(c.c_int32,c.c_size_t,c.c_int,c.c_int)
KEY=c.CFUNCTYPE(c.c_uint64,c.c_size_t);SAME=c.CFUNCTYPE(c.c_int,c.c_size_t,c.c_size_t);OWN=c.CFUNCTYPE(None,c.c_size_t)
STATFIELDS=c.CFUNCTYPE(c.c_int,c.c_size_t)
class API(c.Structure):
    _fields_=[('ref',REF),('array',REF),('at',AT),('count',COUNT),('value',VALUE),('key',KEY),('same',SAME),('keep',OWN),('drop',OWN),('stat_fields',STATFIELDS)]

def library(collector,name):
    path=B/(name+'.so')
    subprocess.run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror','-fPIC','-shared','-I'+str(S),str(S/'eps_core.c'),str(collector),'-o',str(path),'-lm'],check=True)
    lib=c.CDLL(str(path))
    lib.eps_collect.argtypes=[c.POINTER(API),c.c_size_t,c.c_int,c.POINTER(Row)];lib.eps_collect.restype=Result
    lib.eps_compute.argtypes=[c.POINTER(Row),c.c_int,c.c_int,c.c_int,c.POINTER(Output),c.POINTER(Output)];lib.eps_compute.restype=c.c_int
    return lib
old=library(A/'eps_collect_r1.c','eps_r1_control')
new=library(S/'eps_collect.c','eps_r2')
new.eps_reconcile_row.argtypes=[c.POINTER(Row)];new.eps_reconcile_row.restype=c.c_int

f=json.loads(fixture('fault-graph.json.gz'));records={int(k):v for k,v in f['records'].items()}
descriptors={t['rva']:t['fields'] for t in json.loads((F/'native-field-tables.json').read_text())}
mapping={'Franchise':'0xbded330','Team':'0xbdcb0b0','Player':'0xbdc2e00','SeasonGame':'0xbdd32d0','TeamStats':'0xbde8150','GameOffensiveStats':'0xbdf78d0','GameOffensiveKPReturnStats':'0xbdf7da0','GameDefensiveStats':'0xbdf6910','GameDefensiveKPReturnStats':'0xbdf6db0','GameKickingStats':'0xbdf73d0'}
fields={k:descriptors[v] for k,v in mapping.items()};fields['UnpublishedGameStats']=['Player','GameStats']
fields['GameOLineStats']=next(t for t in descriptors.values() if t[:5]==['DOWNSPLAYED','GAMERATING','GAMESSTARTED','OpposingTeam','SeasonGame'] and 'OLINEPANCAKES' in t)
refs=collections.Counter();errors=[]
def keep(r):
    if r:refs[r]+=1
def drop(r):
    if r:
        refs[r]-=1
        if refs[r]<0:errors.append(('overrelease',r))
def get(r,n):
    try:return records[r]['fields'][fields[records[r]['kind']][n]]
    except Exception as e:errors.append(('field',r,n,str(e)));return 0
def ref(r,n):
    v=get(r,n)
    if v and v not in records:errors.append(('missing',r,n,v));return 0
    keep(v);return v
def at(r,n):
    try:v=records[r]['items'][n];keep(v);return v
    except Exception as e:errors.append(('at',r,n,str(e)));return 0
def count(r):return len(records[r]['items'])
def value(r,n,m):
    v=get(r,n)
    if not isinstance(v,(int,bool)):errors.append(('noninteger',r,n,v));return 0
    return v
callbacks=[REF(ref),REF(ref),AT(at),COUNT(count),VALUE(value),KEY(lambda r:r&131071),SAME(lambda a,b:a==b),OWN(keep),OWN(drop),STATFIELDS(lambda r:len(fields[records[r]['kind']]))]
api=API(*callbacks)
expected_rows=fixture('fault-rows.bin.gz')
def collect(lib):
    refs.clear();errors.clear();rows=(Row*4096)()
    result=lib.eps_collect(c.byref(api),f['franchise'],0,rows)
    assert not errors,errors[:20]
    assert not any(refs.values()),'Native reference ownership imbalance'
    assert (result.rows,result.teams,result.players,result.player_games,result.fcs_games)==(1890,143,11730,84500,127)
    assert result.schedule_hash==10300959315151838986
    assert bytes(rows)[:result.rows*c.sizeof(Row)]==expected_rows,'Collector changed team or player production'
    return result,rows
r1,_=collect(old)
assert not r1.success and r1.error==b'team_game_reconciliation' and r1.reconciliations==5310
r2,rows=collect(new)
assert r2.success and not r2.error and r2.reconciliations==9450
residuals=[]
before=bytes(rows)
for x in rows[:r2.rows]:
    status=new.eps_reconcile_row(c.byref(x));assert status in (1,2)
    if status==2:
        residuals.append({'game':x.game,'team':x.team,'opponent':x.opponent,'rush_attempts':x.rush_attempts-x.group[9][1],'rush_yards':x.rush_yards-x.group[9][0],'sacks':x.sacks-x.group[9][7]-.5*x.group[9][8]})
assert bytes(rows)==before
expected={(172,131,51,1,-1,0),(126,131,59,2,0,0),(175,62,117,0,0,1),(366,131,133,1,1,0)}
assert {tuple(x.values()) for x in residuals}==expected,residuals
out=Output();assert new.eps_compute(rows,r2.rows,r2.teams,2026,None,c.byref(out))
assert bytes(rows)==before,'Model mutated source production'
assert all(math.isfinite(g.score) and math.isfinite(g.rank) and 0<=g.status<=2 for team in out.grade for g in team)

# Prior valid snapshot is bit-for-bit unchanged, including every metric/grade.
week6=fixture('week6-rows.bin.gz');wrows=(Row*790).from_buffer_copy(week6)
assert all(new.eps_reconcile_row(c.byref(x))==1 for x in wrows)
wout=Output();assert new.eps_compute(wrows,790,143,2026,None,c.byref(wout))
assert bytes(wout)==fixture('week6-output.bin.gz')
assert bytes(wrows)==week6

# Exercise the entire rushing/sack boundary grid, including both yard signs.
cases=0
for attempts in [-1,0,1,2,3]:
    for yards in range(-11,12):
        for halves in [-1,0,1,2,3]:
            x=Row();x.rush_attempts=40;x.group[9][1]=40-attempts
            x.rush_yards=150;x.group[9][0]=150-yards
            x.sacks=3;x.group[9][7]=(6-halves)//2;x.group[9][8]=(6-halves)%2
            accepted=0<=attempts<=2 and abs(yards)<=5*attempts and 0<=halves<=2
            expected_status=(1 if attempts==yards==halves==0 else 2) if accepted else 0
            original=bytes(x)
            assert new.eps_reconcile_row(c.byref(x))==expected_status,(attempts,yards,halves)
            assert bytes(x)==original;cases+=1
for field,stat in [('pass_yards',2),('pass_attempts',3)]:
    for diff in [-1,1]:
        x=Row();setattr(x,field,100);x.group[9][stat]=100+diff
        assert new.eps_reconcile_row(c.byref(x))==0;cases+=1
for field in ['pass_yards','pass_attempts','rush_yards','rush_attempts','sacks']:
    for v in [math.nan,math.inf,-math.inf,.25]:
        x=Row();setattr(x,field,v);assert new.eps_reconcile_row(c.byref(x))==0;cases+=1
for stat in [0,1,2,3,7,8]:
    for v in [math.nan,math.inf,-math.inf,.25]:
        x=Row();x.group[9][stat]=v;assert new.eps_reconcile_row(c.byref(x))==0;cases+=1

# TU1029R4: valid differences are diagnostic, including larger passing/rushing
# and sack gaps. Invalid values/links are tested in tu1029r4_eps_sources_test.
mutations=[('OFFPASSYARDS',1),('PASSATTEMPTS',1),('RUSHATTEMPTS',3),('OFFRUSHYARDS',30),('SACKS',2)]
stats=next(v['fields'] for v in records.values() if v.get('kind')=='TeamStats')
for key,delta in mutations:
    assert key in stats,key
    saved=stats[key];stats[key]+=delta
    refs.clear();errors.clear();mut_rows=(Row*4096)()
    result=new.eps_collect(c.byref(api),f['franchise'],0,mut_rows)
    stats[key]=saved
    assert result.success and not result.error,(key,result.error)
    assert not errors and not any(refs.values())

report={'status':'PASS_OFFLINE_LIVE_PENDING','r1_reproduced_fault':True,'r2_games':945,'r2_player_games':84500,'r2_reconciliations':9450,'residuals':residuals,'native_reference_balance':True,'production_byte_identical':True,'week6_output_byte_identical':True,'boundary_cases':cases,'full_graph_nonfatal_difference_controls':[k for k,d in mutations],'live_game_verified':False}
(B/'test28r2_eps_reconciliation_results.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
