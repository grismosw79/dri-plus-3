"""Actual W5 graph replay, source isolation and invalid-data controls.
GCC is a portable offline harness; Windows release builds remain Zig 0.13.0.
"""
from pathlib import Path
import ctypes as c, collections, json, subprocess, hashlib, gzip, math, os
S=Path(__file__).resolve().parents[1]
A=S.parent/'Audit/TU1029R4';I=A/'Inputs'
B=Path(os.environ.get('DRI_TEST_OUTPUT_DIR',A/'Validation/portable'));B.mkdir(parents=True,exist_ok=True)
f=json.loads(gzip.decompress((I/'w5_graph.json.gz').read_bytes()));records={int(k):v for k,v in f['records'].items()}
assert f['sha256']=='05deeaa8fa819c5c7041a85b8b4b9e9a58d491ce1f70b23811b5fce638429159'
class Row(c.Structure):
 _fields_=[('team',c.c_int),('opponent',c.c_int),('game',c.c_int)]+[(s,c.c_double) for s in ['pass_yards','pass_attempts','rush_yards','rush_attempts','sacks']]+[('group',(c.c_double*28)*10)]
class Result(c.Structure):
 _fields_=[(s,c.c_int) for s in ['success','rows','teams','players','player_games','fcs_games','reconciliations']]+[('error',c.c_char_p),('schedule_hash',c.c_uint64)]
REF=c.CFUNCTYPE(c.c_size_t,c.c_size_t,c.c_int);AT=c.CFUNCTYPE(c.c_size_t,c.c_size_t,c.c_uint)
COUNT=c.CFUNCTYPE(c.c_uint,c.c_size_t);VALUE=c.CFUNCTYPE(c.c_int32,c.c_size_t,c.c_int,c.c_int)
KEY=c.CFUNCTYPE(c.c_uint64,c.c_size_t);SAME=c.CFUNCTYPE(c.c_int,c.c_size_t,c.c_size_t);OWN=c.CFUNCTYPE(None,c.c_size_t);SF=c.CFUNCTYPE(c.c_int,c.c_size_t)
class API(c.Structure):
 _fields_=[('ref',REF),('array',REF),('at',AT),('count',COUNT),('value',VALUE),('key',KEY),('same',SAME),('keep',OWN),('drop',OWN),('stat_fields',SF)]
refs=collections.Counter();errors=[]
def keep(r):
 if r:refs[r]+=1
def drop(r):
 if r:
  refs[r]-=1
  if refs[r]<0:errors.append(('overrelease',r))
def get(r,n):
 try:return records[r]['fields'][str(n)]
 except Exception as e:errors.append(('field',r,n,str(e)));return 0
def ref(r,n):
 v=get(r,n)
 if v and v not in records:errors.append(('missing',r,n,v));return 0
 keep(v);return v
def at(r,n):
 try:v=records[r]['items'][n];keep(v);return v
 except Exception as e:errors.append(('at',r,n,str(e)));return 0
callbacks=[REF(ref),REF(ref),AT(at),COUNT(lambda r:len(records[r]['items'])),VALUE(lambda r,n,m:get(r,n)),KEY(lambda r:r&131071),SAME(lambda a,b:a==b),OWN(keep),OWN(drop),SF(lambda r:records[r]['stat_fields'])]
api=API(*callbacks)
class Metric(c.Structure):
 _fields_=[(s,c.c_double) for s in ['value','observed','numerator','denominator','prior_exposure','prior_value']]
class Grade(c.Structure):
 _fields_=[('score',c.c_double),('rank',c.c_double),('grade',c.c_int),('status',c.c_int)]
class Output(c.Structure):
 _fields_=[('year',c.c_int),('rows',c.c_int),('teams',c.c_int),('selected_prior_games',c.c_double*4),('exposure',(c.c_double*11)*138),('metric',(Metric*36)*138),('grade',(Grade*23)*138)]
def library(collector,name):
 path=B/(name+'.so')
 subprocess.run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror','-fPIC','-shared','-I'+str(S),str(collector),str(S/'eps_core.c'),'-o',str(path),'-lm'],check=True)
 lib=c.CDLL(str(path));lib.eps_collect_through.argtypes=[c.POINTER(API),c.c_size_t,c.c_int,c.POINTER(Row),c.c_int];lib.eps_collect_through.restype=Result
 lib.eps_reconcile_row.argtypes=[c.POINTER(Row)];lib.eps_reconcile_row.restype=c.c_int
 lib.eps_compute.argtypes=[c.POINTER(Row),c.c_int,c.c_int,c.c_int,c.POINTER(Output),c.POINTER(Output)];lib.eps_compute.restype=c.c_int
 return lib
old=library(I/'eps_collect_r3.c','r3_control');new=library(S/'eps_collect.c','r4_sources')
new.eps_row_valid.argtypes=[c.POINTER(Row)];new.eps_row_valid.restype=c.c_int
def collect(lib,cutoff=8):
 refs.clear();errors.clear();rows=(Row*4096)();result=lib.eps_collect_through(c.byref(api),f['franchise'],0,rows,cutoff)
 assert not errors,errors[:20]
 assert not any(refs.values()),'Reference ownership imbalance'
 return result,rows
before=hashlib.sha256(json.dumps(records,sort_keys=True).encode()).hexdigest()
r3,oldrows=collect(old);r4,rows=collect(new)
assert not r3.success and r3.error==b'team_game_reconciliation'
assert r4.success and r4.error is None
assert (r4.rows,r4.teams,r4.players,r4.player_games,r4.fcs_games)==(668,143,11730,29485,116)
assert r4.reconciliations==3340 and r3.schedule_hash==r4.schedule_hash==12763643364389594007
assert bytes(rows)==bytes(oldrows),'Changing admission must not change production'
diffs=[]
for x in rows[:r4.rows]:
 assert new.eps_row_valid(c.byref(x))
 status=new.eps_reconcile_row(c.byref(x))
 if status!=1:diffs.append({'game':x.game,'team':x.team,'opponent':x.opponent,'status':status,'pass_yards':x.pass_yards-x.group[9][2],'pass_attempts':x.pass_attempts-x.group[9][3],'rush_yards':x.rush_yards-x.group[9][0],'rush_attempts':x.rush_attempts-x.group[9][1],'sack_halves':2*x.sacks-2*x.group[9][7]-x.group[9][8]})
assert diffs==json.loads((I/'r3_w5_replay.json').read_text())['residuals']
original=bytes(rows);output=Output();assert new.eps_compute(rows,r4.rows,r4.teams,2026,None,c.byref(output))
assert bytes(rows)==original
assert all(math.isfinite(g.score) and math.isfinite(g.rank) and 0<=g.grade<=12 and 0<=g.status<=2 for t in output.grade for g in t)
(B/'w5_output.bin').write_bytes(bytes(output));(B/'w5_rows.bin').write_bytes(original[:r4.rows*c.sizeof(Row)])

# Every logged W8 residual shape is admitted without rescaling. These are
# constructed rows using reported differences, NOT a replay of absent W8 data.
modeled=0
for e in json.loads((I/'r3_runtime_fault_events.json').read_text()):
 v=e['row']
 if v[0]!='EPS_TEAM_RESIDUAL':continue
 d={v[i]:int(v[i+1]) for i in range(7,len(v),2)}
 x=Row();x.pass_yards=x.rush_yards=1000;x.pass_attempts=x.rush_attempts=100;x.sacks=10
 x.group[9][2]=1000-d['pass_yards'];x.group[9][3]=100-d['pass_attempts'];x.group[9][0]=1000-d['rush_yards'];x.group[9][1]=100-d['rush_attempts']
 halves=20-d['sack_halves'];x.group[9][7]=halves//2;x.group[9][8]=halves%2
 raw=bytes(x);assert new.eps_row_valid(c.byref(x));new.eps_reconcile_row(c.byref(x));assert bytes(x)==raw;modeled+=1
assert modeled==37
# Surpluses are also differences, not corruption by themselves.
x=Row();x.pass_yards=100;x.group[9][2]=400;x.pass_attempts=10;x.group[9][3]=40;x.rush_yards=50;x.group[9][0]=148;x.rush_attempts=10;x.group[9][1]=24;x.sacks=1;x.group[9][7]=2
assert new.eps_row_valid(c.byref(x)) and new.eps_reconcile_row(c.byref(x))==0

# Model routing: two games would invoke opponent fitting only at four games,
# so this pair independently exposes raw authoritative numerators.
pair=(Row*2)()
for side in range(2):
 x=pair[side];x.team=side;x.opponent=1-side;x.game=100;x.pass_yards=300;x.pass_attempts=30;x.rush_yards=150;x.rush_attempts=25;x.sacks=3
 x.group[9][2]=14;x.group[9][3]=3;x.group[9][0]=52;x.group[9][1]=11;x.group[9][7]=1
 x.group[2][4]=47;x.group[2][5]=4;x.group[3][4]=91;x.group[5][7]=1
def compute_pair():
 out=Output();assert new.eps_compute(pair,2,138,2026,None,c.byref(out));return out
out=compute_pair();assert out.metric[0][27].numerator==300 and out.metric[0][26].numerator==30 and out.metric[0][33].numerator==150
assert out.metric[0][5].numerator==47 and out.metric[0][20].numerator==91 and out.metric[0][0].numerator==1
assert out.metric[0][34].numerator==3 # team sacks, not the individual total of 1
pair[0].group[9][2]+=286;pair[0].group[9][3]+=27;assert bytes(compute_pair())==bytes(out)
pair[0].group[2][4]+=13;out2=compute_pair();assert out2.metric[0][5].numerator==60 and out2.metric[0][27].numerator==300
pair[0].pass_yards+=200;out3=compute_pair();assert out3.metric[0][27].numerator==500 and out3.metric[0][5].numerator==60

# Existing raw-value controls are retained independently of equality.
negative=0
for field in ['pass_yards','pass_attempts','rush_yards','rush_attempts','sacks']:
 for bad in [math.nan,math.inf,-math.inf,.25]:
  x=Row();setattr(x,field,bad);assert not new.eps_row_valid(c.byref(x));negative+=1
for stat in [0,1,2,3,7,8]:
 for bad in [math.nan,math.inf,-math.inf,.25]:
  x=Row();x.group[9][stat]=bad;assert not new.eps_row_valid(c.byref(x));negative+=1
for field in ['pass_attempts','rush_attempts','sacks']:
 x=Row();setattr(x,field,-1);assert not new.eps_row_valid(c.byref(x));negative+=1
for stat in [1,3,7,8]:
 x=Row();x.group[9][stat]=-1;assert not new.eps_row_valid(c.byref(x));negative+=1
# Full graph rejects bad links/schema/ranges; balanced ownership on every exit.
stat=next(v for v in records.values() if v.get('stat_fields')==26 and v['fields']['5']>0)
teamstats=next(v for v in records.values() if v['kind']=='TeamStats')
controls=[]
for label,container,key,bad,expected in [
 ('missing_game',stat['fields'],'4',0,b'player_game_link'),
 ('missing_opponent',stat['fields'],'3',0,b'player_game_opponent'),
 ('unknown_schema',stat,'stat_fields',25,b'player_stat_range'),
 ('out_of_range_stat',stat['fields'],'11',100001,b'player_stat_range'),
 ('negative_team_attempts',teamstats['fields'],'33',-1,b'game_team_or_cache'),
 ('negative_player_attempts',stat['fields'],'5',-10000,b'team_game_values')]:
 saved=container[key];container[key]=bad;res,_=collect(new);container[key]=saved
 assert not res.success and res.error==expected,(label,res.success,res.error);controls.append(label)
saved=teamstats['fields']['27'];teamstats['fields']['27']+=286
res,changed=collect(new);teamstats['fields']['27']=saved;assert res.success
assert any(x.pass_yards-y.pass_yards==286 for x,y in zip(changed[:res.rows],rows[:r4.rows]))
assert before==hashlib.sha256(json.dumps(records,sort_keys=True).encode()).hexdigest()
report={'passed':True,'old_fault_reproduced':True,'new_w5_collection_and_model_succeed':True,'w5_games':r4.rows//2,'w5_player_games':r4.player_games,'w5_source_differences':diffs,'native_reference_balance':True,'input_and_collected_production_unchanged':True,'modeled_w8_residual_shapes':modeled,'modeled_w8_is_live_replay':False,'source_routing_checks':True,'malformed_value_cases':negative,'graph_negative_controls':controls,'snapshot_signature_unchanged':True,'live_game_verified':False}
(B/'tu1029r4_eps_sources_results.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
