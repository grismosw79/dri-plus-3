"""Compile exact pure snapshot functions from parent/current source on137 W9 rosters.
This is a capacity model, not native eligibility, funding or signing simulation.
"""
from pathlib import Path
import os,json,gzip,ctypes as C,subprocess,hashlib
S=Path(__file__).resolve().parents[1];O=Path(os.environ['DRI_TEST_OUTPUT_DIR']);O.mkdir(parents=True,exist_ok=True)
B=Path(os.environ['DRI_BASELINE_DIR'])/'Runtime/Portal/Source'
fixture=json.loads(gzip.decompress((S/'Tests/Fixtures/tu1029r5_w9_pool.json.gz').read_bytes()))
def function(text,name):
 start=text.index('static int '+name+'(');brace=text.index('{',start);depth=1;i=brace+1
 while depth:
  depth+=(text[i]=='{')-(text[i]=='}');i+=1
 return text[start:i]
def build(src,label):
 hs=(src/'hs15.inc').read_text();ret=(src/'retention.c').read_text()
 types=ret[ret.index('#define HS15_MAX_PLAYERS'):ret.index('} Hs15Snapshot;')+len('} Hs15Snapshot;')]
 annual_include='\n#include "class31_policy.h"' if (src/'class31_policy.h').exists() else ''
 pieces=['#include <stdint.h>\n#include <string.h>\n#define POSITIONS 22\ntypedef uint64_t U;\n#include "policy.h"\n#include "portal_policy.h"'+annual_include,types,function((src/'portal.inc').read_text(),'portal_projected_ovr'),function(hs,'hs15_portal_rooms')]
 if 'static int hs29_quality_extra(' in hs:pieces.append(function(hs,'hs29_quality_extra'))
 pieces.append(function(hs,'hs15_snapshot_values'))
 pieces.append(r'''
int assess(const int *ideals,const int *starters,const int *players,int n,int prestige,int step,int pos,int stars,int ovr) {
 Hs15Snapshot s={0};s.prestige=prestige;s.projection_step=step;
 for(int p=0;p<22;p++){s.ideal[p]=ideals[p];s.starters[p]=starters[p];s.total_ideal+=ideals[p];s.flex_floor[p]=100;}
 for(int i=0;i<n;i++) {
  const int *r=players+i*6;int p=r[0],v=r[1],q=portal_projected_ovr(v,(r[3]||r[4])?step:0);
  s.players[s.used++]=(Hs15Player){.rid=(uint32_t)i,.pos=p,.ovr=v,.stars=r[2],.returning=r[3],.incoming=r[4],.hs=r[5]};
  s.count[p]++;s.total++;s.returning[p]+=r[3];s.hs[p]+=r[5];
  if(r[5]){s.four[p]+=r[2]==3;s.five[p]+=r[2]==4;}
  if(q<s.flex_floor[p])s.flex_floor[p]=q;
  if(!p && q>=85)s.qb85++;
 }
 int reserved=0;for(int p=0;p<22;p++)reserved+=s.count[p]>s.ideal[p]?s.count[p]:s.ideal[p];
 if((s.total_ideal==83||s.total_ideal==84)&&reserved<85)
  for(int p=1;p<22;p++)if(p!=19&&p!=20&&s.ideal[p]>0&&s.count[p]==s.ideal[p])s.flex[p]=1;
 return hs15_snapshot_values(&s,pos,stars,ovr);
}
''')
 path=O/(label+'.c');path.write_text('\n'.join(pieces));lib=O/(label+'.so')
 subprocess.run(['gcc','-std=c11','-shared','-fPIC','-O2','-Wall','-Wextra','-Werror','-Wno-unused-function','-I'+str(src),str(path),'-o',str(lib)],check=True)
 dll=C.CDLL(str(lib));fn=dll.assess;fn.argtypes=[C.POINTER(C.c_int)]*3+[C.c_int]*6;fn.restype=C.c_int
 return fn
old=build(B,'parent_snapshot');new=build(S,'current_snapshot')
changes=[];checks=0;okoro=[]
for t in fixture['cases']:
 ideal=(C.c_int*22)(*t['ideal']);starters=(C.c_int*22)(*t['starters']);flat=sum(t['players'],[]);players=(C.c_int*len(flat))(*flat)
 for r in fixture['elites']:
  args=(ideal,starters,players,len(t['players']),t['prestige'],fixture['projection_step'],r['pos'],r['stars'],r['ovr'])
  a=old(*args);b=new(*args);checks+=1
  assert a>=0 and b>=0
  if not a:assert not b,'No previously permissible acquisition loses capacity'
  if a!=b:assert a in (1,3) and b in (0,8), (t['tid'],r['rid'],a,b)
  if a and b==0:
   assert r['pos'] not in [0,19,20]
   room=[p for p in t['players']if p[0]==r['pos']];hs=[p for p in room if p[5]]
   assert len(room)==t['ideal'][r['pos']] and hs and r['ovr']>=max(p[1]for p in hs)+4
   changes.append({'team':t['tid'],'school':t['name'],'rid':r['rid'],'recruit':r['name'],'ovr':r['ovr'],'old_reason':a,'new_reason':b})
  if r['rid']==1655 and t['tid'] in [23,29,47,71,100]:okoro.append({'team':t['tid'],'school':t['name'],'old_reason':a,'new_reason':b})
assert checks==137*68
result={'passed':True,'comparisons':checks,'schools':137,'open_elites':68,'new_capacity_edges':len(changes),'distinct_recruits':len({r['rid']for r in changes}),'distinct_schools':len({r['team']for r in changes}),'changes':changes,'okoro_capacity_only':okoro,'inputs':{k:fixture[k]for k in ['source_sha256','config_inputs','projection_step','scope']},'live_game_verified':False,'native_eligibility_verified':False,'limits':['Already spent shared allowance still closes a school. These counts are permissive roster edges, not recruiting destinations, offers or forecast signings.','Service academy eligibility is applied by the runtime outside this pure capacity function; do not count capacity edges as permission to recruit elite HS at academies.']}
if (S/'class31_policy.h').exists():
 result['scope']='Historical R5 quality-exception regression with annual_valid=0 in both extracted inputs; Test31 production annual planning is covered by test31_class_* tests, not these legacy capacity edges'
(O/'tu1029r5_pool_results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items()if k not in ['changes','inputs','limits']},indent=2))
