"""Rebuild compact saved-data fixtures from externally extracted JSON inputs.

This generator never opens a game save or executes/includes the DLL. It applies
the documented eligibility, ownership, one-year projection and class-counting
rules independently to saved fields, yielding 157 expected outputs per team.
The test then supplies those saved fields to the compiled C implementation.

Inputs: the existing Test10 TP1 baseline JSON, Test15 TP3/TP4/NSD JSONs, and
current_configs.json (resolved saved scheme references). These large extracted
inputs are deliberately external to the delivery. SHA256s of each JSON, source
save, and config JSON are embedded in the fixture for audit/reproduction.
"""
from pathlib import Path
import argparse,json,gzip,hashlib
parser=argparse.ArgumentParser(description=__doc__)
for name in ['tp1','tp3','tp4','nsd','configs']:
 parser.add_argument('--'+name+'-json',type=Path,required=True)
parser.add_argument('--output',type=Path,default=Path(__file__).with_name('test15r1_real_rosters.json.gz'))
args=parser.parse_args()
cfgpath=args.configs_json
configs={t['tid']:t for t in json.loads(cfgpath.read_text())['teams']}
paths={phase:getattr(args,phase.lower()+'_json') for phase in ['TP1','TP3','TP4','NSD']}
pos={x:i for i,x in enumerate('QB HB FB WR TE LT LG C RG RT LE RE DT LOLB MLB ROLB CB FS SS K P LS'.split())}
stars={x:i for i,x in enumerate(['ONE_STAR','TWO_STAR','THREE_STAR','FOUR_STAR','FIVE_STAR','Count_5','Invalid'])}
years={x:i for i,x in enumerate(['Freshman','Sophomore','Junior','Senior'])}
classes={x:i for i,x in enumerate(['HighSchool','JuniorCollege_Sophomore','JuniorCollege_Junior','JuniorCollege_Senior','Transfer_Freshman','Transfer_Sophomore','Transfer_Junior','Transfer_Senior'])}
stages={x:i for i,x in enumerate(['Top10','Top5','Top3','Battle','SoftCommitted','HardCommitted','Signed'])}
offers={'Unknown':0,'None':0,'Unoffered':0,'Revoked':1,'New':2,'Offered':3,'Committed':4}
def rid(r):return int(r[-17:],2)
fixture={'version':1,'method':'Per-team complete real roster, published commits, and source-owned secured HS/JUCO/portal arrivals. Each compact franchise contains the current team; disjoint foreign player/recruit identities are omitted. This checks 552 complete roster snapshots, not full-league recruiting simulation or foreign-ownership resolution. Foreign-owner resolution has separate compiled tests. Expected counts are independently derived in Python from saved player/class/team fields: eligible returners minus recorded non-staying early NFL departures, identity-unioned with published and secured arrivals; current HS/JUCO star counters remain separate. One native OVRProjection step is used for returners/portal arrivals, followed by independent QB85 and reserved-vacancy flex calculations. No runtime C helper computes expected values.','projection_step':2,'config_sha256':hashlib.sha256(cfgpath.read_bytes()).hexdigest(),'sources':[],'cases':[]}
for phase,path in paths.items():
 raw=Path(path).read_bytes();d=json.loads(raw);ts={x['name']:x['rows'] for x in d['tables']};players={x['_row']:x for x in ts['Player']};ss=ts['SeasonInfo'][0]
 fixture['sources'].append({'phase':phase,'save_sha256':d['sha256'],'parsed_json_sha256':hashlib.sha256(raw).hexdigest(),'season':{k:ss[k] for k in ['CurrentSeasonYear','CurrentWeek','CurrentWeekType']}})
 published={rid(p):t['tid'] for t in d['teams'] for p in t['committed']}
 deps={rid(x['Player']) for x in ts['LeavingPlayer'] if x['LeaveType'].startswith('EarlyNFL_') and x['LeaveStatus']!='Staying'}
 classids=set(rid(x) for x in d['franchiseArrays']['RecruitingClassPlayers']);portalids=set(rid(x) for x in d['franchiseArrays']['Transfers'])
 rs={x['_row']:x for x in d['recruits']};assert not classids&portalids
 for t in sorted(d['teams'],key=lambda x:x['tid']):
  tid=t['tid'];roster=list(map(rid,t['roster']));pub=list(map(rid,t['committed']));ideal=[0]*22
  for ent in configs[tid]['entries']:ideal[ent['pos']]=ent['ideal']
  board={rid(x['Recruit']):offers[x['ScholarshipStatus']] for x in t['targets']}
  incoming=[]
  for rr in sorted(classids|portalids):
   r=rs[rr];stage=stages.get(r['RecruitStage'],-1)
   if stage not in [5,6]:continue
   pid=r['pid'];leader=r['top'][0]['TeamId'] if r['top'] else 255;portal=rr in portalids
   owned=(published.get(pid)==tid) if stage==6 else ((published.get(pid,leader)==tid and (pid in published or board.get(rr,0)>=2) and tid not in [0,7,55]) if portal else leader==tid)
   if owned:incoming.append([rr,pid,stage,classes[r['Class']],leader,board.get(rr,0)])
  trigger=None
  if phase=='TP1' and tid==8:
   r=rs[1038];trigger=[1038,r['pid'],stages[r['RecruitStage']],classes[r['Class']],r['top'][0]['TeamId'],board[1038]]
  relevant=set(roster)|set(pub)|{r[1] for r in incoming}|({trigger[1]} if trigger else set())
  pl=[[p,pos[players[p]['Position']],players[p]['OverallRating'],stars[players[p]['ProspectStarRating']],years[players[p]['SchoolYear']],players[p]['TeamIndex']] for p in sorted(relevant)]
  pmap={p[0]:p for p in pl};flags={}
  def add(p,index):
   v=flags.setdefault(p,[0,0,0])
   if index is not None:v[index]=1
  for p in roster:
   if pmap[p][4]<3 and pmap[p][5]==tid and p not in deps:add(p,0)
  for p in pub:add(p,None)
  for r in incoming:add(r[1],2 if r[0] in classids else 1)
  count=[0]*22;returning=[0]*22;hs=[0]*22;four=[0]*22;five=[0]*22;flex=[0]*22;qb85=0
  for pid,(ret,portal,h) in flags.items():
   _,p,ovr,s,_,_=pmap[pid];count[p]+=1;returning[p]+=bool(ret and not portal and not h);hs[p]+=h;four[p]+=bool(h and s==3);five[p]+=bool(h and s==4)
   qb85+=bool(p==0 and min(99,ovr+(2 if ret or portal else 0))>=85)
  if sum(ideal) in [83,84] and sum(max(a,b) for a,b in zip(count,ideal))<85:
   for p in range(1,22):flex[p]=int(p not in [19,20] and ideal[p]>0 and count[p]==ideal[p])
  expected=ideal+count+returning+hs+four+five+flex+[qb85,sum(count),sum(ideal)]
  fixture['cases'].append({'phase':phase,'tid':tid,'team':t['name'],'ideal':ideal,'players':pl,'roster':roster,'published':pub,'departures':sorted(set(roster)&deps),'recruits':incoming,'trigger':trigger,'expected':expected})
 print(phase,len([x for x in fixture['cases'] if x['phase']==phase]))
assert len(fixture['cases'])==552
out=args.output;raw=json.dumps(fixture,separators=(',',':')).encode();out.write_bytes(gzip.compress(raw,mtime=0));print(out,len(raw),out.stat().st_size)
a=next(x for x in fixture['cases'] if x['phase']=='TP1' and x['tid']==8);print('AUBURN',a['expected'][155],next(x for x in a['players'] if x[0]==13984),len(a['recruits']))
