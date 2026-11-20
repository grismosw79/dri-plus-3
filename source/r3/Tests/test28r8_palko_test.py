"""Observed Palko allocation role must survive the actual DLL funding rebuild."""
exec(compile((__import__('pathlib').Path(__file__).parent/'portal_runtime_test.py').read_text(),'portal_runtime_test.py','exec'))
import ctypes as C
fixture=json.loads((ROOT/'Tests/Fixtures/test28r8_palko.json').read_text())
assert any(e['text']=='HS28_PAIR,3,2311,3,127,2,30,0,1,5,58'for e in fixture['events'])
assert any(e['text']=='HS28_FUNDING,3,2311,1,17,0'for e in fixture['events'])
configure(1);call(exports['DriTestOwnershipReset']);call(exports['DriTestHsPlanReset'])
records[season]['scalar'].update({2:2026,4:6,5:1});records[team]['scalar'][383]=3;records[teams]['array']=fixture_team_array()
for x in [departures,roster,hslist,tplist,committed,arr]:records[x]['array']=[]
for pos,e in enumerate(ideal_entries):records[e]['scalar'].update({2:ideals[pos],4:1})
records[ideal_entries[0]]['scalar'][2]=4;records[ideal_entries[3]]['scalar'][2]=10
for pos in range(22):
 count=4 if pos==0 else 10 if pos==3 else ideals[pos]
 records[roster]['array'] += [pp('returner '+str(pos)+' '+str(j),65,pos=pos,tid=3)for j in range(count)]
items={}
for item in fixture['players']:
 r,t,p=pr(item['rid'],item['ovr'],year=0,pos=3,stage=item['stage'],portal=False);items[item['rid']]=(r,t,p)
 records[p]['scalar'][231]=3;records[r]['scalar'][3]=item['commit_score']
 top=records[r]['ref'][13];records[top]['array']=[record('saved interest',{0:x['TeamId'],1:x['TeamInfluence']})for x in item['top_schools']]
 records[t]['scalar'][8]=next(x['TeamInfluence']for x in item['top_schools']if x['TeamId']==3)
 if item['rid']!=2311:valid_offer(r,t)
 else:records[arr]['array'].append(t);records[t]['scalar'][12]=0;records[r]['scalar'][14]=0
class Choice(C.Structure):
 _fields_=[(k,C.c_uint64)for k in ['table','rec','target']]+[('rid',C.c_uint32)]+[(k,C.c_int)for k in ['pos','ovr','stars','stage','rank','points','leader','cut_points','commit_score','native_score','legal','offered','existing','viable','urgent','index','final','selected','slot','choice','weight','order']]+[('player_table',C.c_uint64),('player_row',C.c_uint32)]
seed=(Choice*2)()
for j,rid in enumerate([1885,2311]):
 r,t,p=items[rid];q=seed[j];q.table=rq(r+0x10);q.rid=rid;q.rec=r;q.target=t;q.player_table=rq(p+0x10);q.player_row=ri(p+0x18)
 q.pos=3;q.selected=1;q.slot=127;q.choice=j+1;q.weight=30;q.order=j+1;q.native_score=475 if rid==2311 else 300
mem=alloc(C.sizeof(seed));uc.mem_write(mem,bytes(seed));call(exports['DriTestHsPlanStore'],mem,2,team,context)
role=lambda rid,f=0:call(exports['DriTestHsPlanRole'],items[rid][0],team,context,f)
assert (role(1885),role(2311),role(183))==(1,2,0)
assert (role(1885,1),role(2311,1))==(30,30)
ff=STACK+0x55000;fb=ff+0x400;wq(ff+10*8,fb);wq(ff+9*8,context);tr=alloc(8);wq(tr,team);wq(fb-0x60,tr)
vec=alloc(216);ind=alloc(24);wq(fb-0x50,vec);wq(fb-0x48,vec+216);wq(fb+0x58,ind);wq(fb+0x60,ind+24)
for j,rid in enumerate([1885,183,2311]):
 r,t,p=items[rid];wq(vec+j*72,r);wi(vec+j*72+40,300);wq(vec+j*72+48,t);wi(vec+j*72+64,0 if rid==2311 else 2);wq(ind+j*8,j)
call(hooks[23],ff)
assert role(2311,1)==30 and role(2311)==2
assert ri(vec+40)+ri(vec+112)==600 and ri(vec+184)==300
assert records[items[2311][1]]['scalar'][12]==0,'Planning must not fabricate the unobserved offer'
call(exports['DriTestOwnershipReset']);call(exports['DriTestHsPlanReset'])
assert call(exports['DriTestQualityFault'])==0
assert all(v==1 for v in refs.values())
result={'checks':['Captured R7 Arizona W6 plan had Palko backup weight30, followed by weight1 funding','R8 retains the selected Palko backup at weight30 across the actual native funding callback','Native stage totals conserved; no fabricated offer or influence; record references balanced'],'scope':fixture['scope'],'source_csv_sha256':fixture['source_sha256'],'live_game_verified':False}
(OUTPUT/'test28r8_palko_results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
