"""Compiled EPS hooks and captured required-grade evaluator, with mock records.
This exercises native threshold selection and cleanup, not a running game.
"""
from pathlib import Path
source=(Path(__file__).parent/'runtime_fixture_test.py').read_text()
exec(compile(source.split("season=record('season'")[0],'runtime_fixture_test.py','exec'))
def configure(m):call(exports['DriTestConfigure'],GAME,m,7)
configure(1)
for rva,size in [(0x08432da0,0x1000),(0x0860cda0,0x2000),(0x08055000,0x1000),(0x082a1ce0,0x1000),(0x08615da0,0x1000)]:
 uc.mem_map(GAME+rva,size);uc.mem_write(GAME+rva,raw[rva-0x1000:rva-0x1000+size])
PLAYER_SCHEMA=0x123400;RECRUIT_SCHEMA=0x123500
wq(VT+0x38,api(lambda:records[reg('RCX')].get('schema',0xabcdef)))
wq(TABLEVT+0xf8,api(native_scalar))
api(lambda:int(reg('RCX')==reg('RDX')),GAME+0x08059c00)
allowance=2
api(lambda:allowance,GAME+0x082a2690)
# Native transfer-class check tests Recruit.RecruitingClass (4..7).
api(lambda:int(4<=records[rq(reg('RCX')+8)]['scalar'].get(2,0)<=7),GAME+0x0862db70)
season=record('season',{2:2026,3:2,4:1,5:6,6:0,24:1})
franchise=record('franchise',{28:1549427592},ref={44:season})
myrec=record('MySchoolGradeEval',ref={2:franchise});my=alloc(0x1400)
wq(my+0x10,rq(myrec+0x10));wi(my+0x18,ri(myrec+0x18));wq(my+0x30,0xabcdef)
eval_ptr=alloc(0x500);wq(eval_ptr+0x60,PLAYER_SCHEMA);wq(eval_ptr+0x68,RECRUIT_SCHEMA);wq(eval_ptr+0xc8,my)
effects=record('coach talent effects');team=record('team',{383:2},ref={32:effects});teamref=alloc(8);wq(teamref,team)
records[franchise]['ref'][45]=record('fixture school list',array=[team])
player=record('player',{3:0,191:85,202:0,231:4,242:1});records[player]['schema']=PLAYER_SCHEMA
playerref=alloc(8);wq(playerref,player)
recruit=record('recruit',{2:5},ref={5:player});records[recruit]['schema']=RECRUIT_SCHEMA
recruitref=alloc(8);wq(recruitref,recruit)
portal_values={i:-100 for i in range(13)};portal_values.update({10:0,9:1,7:2,4:3,3:4})
hs_values={i:-100 for i in range(13)};hs_values.update({12:0,11:1,8:2,6:3,4:4})
# A deliberately different legacy OVR curve tests base replacement independently.
old_values={i:-1 for i in range(13)};old_values.update({10:0,7:65,4:75,3:80,2:85,1:90})
oldcurve=record('old OVR curve',old_values);portalcurve=record('portal star curve',portal_values);hscurve=record('HS star curve',hs_values)
info=record('style info',{3:8},ref={4:oldcurve,5:hscurve,6:portalcurve});inforef=alloc(8);wq(inforef,info)
addresses=alloc(64);call(exports['DriTestEpsAddresses'],addresses);native=list(struct.unpack('<8Q',uc.mem_read(addresses,64)))
call(exports['DriTestEpsSeed'],franchise,2026,1,6)
checks=[]
assert call(exports['DriTestEpsGrade'],my,teamref,0,playerref)==4
call(exports['DriTestEpsSet'],2,0,6,1)
assert call(exports['DriTestEpsGrade'],my,teamref,0,playerref)==6
assert call(exports['DriTestEpsGeneric'],my,teamref,0,0)==6
checks.append('Player grade and archetype overview use the same supported EPS profile')
# Actual P position selects placement/power independently of the shared KP enum.
records[player]['scalar'].update({202:61,3:19})
call(exports['DriTestEpsSet'],2,19,9,1);call(exports['DriTestEpsSet'],2,22,1,1)
assert call(exports['DriTestEpsGrade'],my,teamref,0,playerref)==9
records[player]['scalar'][3]=20
assert call(exports['DriTestEpsGrade'],my,teamref,0,playerref)==1
assert call(exports['DriTestEpsGeneric'],my,teamref,0,61)==9
checks.append('Same KP_Accurate archetype: K uses accuracy, P uses placement; generic KP overview explicitly uses K')
records[player]['scalar'].update({202:0,3:0})
def required(ref=playerref):return call(exports['DriTestEpsRequired'],eval_ptr,inforef,ref,teamref)
def native_required(ref=playerref):return call(native[2],eval_ptr,inforef,ref,teamref)
before=native_required();assert before==4,before
assert required()==6
assert native_required()==before,'No native curve or player mutation is permitted'
cases=0
for allowance in [0,1,2]:
 for age in range(4):
  for ovr in [0,67,68,69,70,72,73,74,75,79,80,81,82,87,88,89,90,99]:
   quality=ovr+(2 if age==0 else 1 if age==1 else 0);star=sum(quality>=v for v in [70,75,82,90])
   records[player]['scalar'].update({191:ovr,242:age,231:star});records[recruit]['scalar'][2]=4+age
   expected=[10,9,7,4,3][star]+allowance
   assert required()==expected,(ovr,age,allowance,required(),expected)
   assert required(recruitref)==expected
   assert native_required(recruitref)==expected
   cases+=1
checks.append(f'{cases} OVR/age/coach combinations: home and portal requirements agree; captured native coach allowance preserved')
records[recruit]['scalar'][2]=0
assert required(recruitref)==native_required(recruitref)==6
checks.append('HS uses its original star requirement, including the original five-star threshold')
call(exports['DriTestEpsSet'],2,0,8,0)
assert call(exports['DriTestEpsGrade'],my,teamref,0,playerref)==13
assert required()==13 and required(recruitref)==13
assert 13<=required() and 13-required()==0
checks.append('Provisional N/A grade and requirement pass native <= gate and give a neutral PLV grade difference')
call(exports['DriTestEpsSet'],2,0,5,2)
assert call(exports['DriTestEpsGrade'],my,teamref,0,playerref)==5
assert required()==[10,9,7,4,3][star]+allowance
checks.append('Verified prior-year carried profile remains a real grade and requirement')
records[info]['scalar'][3]=5
assert required()==native_required()
records[info]['scalar'][3]=8
checks.append('Coach Prestige retains its native requirement; shared college policies have separate Test28 coverage')
# The passthrough wrappers call original functions with their exact four arguments.
seen=[]
api(lambda:(seen.append(args()) or 7),native[0]);api(lambda:(seen.append(args()) or 8),native[1])
for wanted in [0,-1]:
 configure(wanted)
 assert call(exports['DriTestEpsGrade'],my,teamref,0,playerref)==7
 assert seen[-1]==(my,teamref,0,playerref)
 assert call(exports['DriTestEpsGeneric'],my,teamref,0,61)==8
 assert seen[-1]==(my,teamref,0,61)
 assert required()==native_required()
configure(1)
assert call(exports['DriTestEpsGrade'],my,teamref,6,playerref)==7
assert call(exports['DriTestEpsGeneric'],my,teamref,6,61)==8
checks.append('Diagnostic/disabled modes and every non-Playing-Style grade call retain native arguments and result')
call(exports['DriTestEpsReset'])
assert all(v==1 for v in refs.values()),[(records[r]['name'],v) for r,v in refs.items() if v!=1]
assert call(exports['DriTestQualityFault'])==0
checks.append('Every native record reference balanced; no runtime fault')
(OUTPUT/'eps_runtime_results.json').write_text(json.dumps({'checks':checks,'nativeRequirementCases':cases,'live_game_verified':False},indent=2))
print(json.dumps(checks,indent=2))
