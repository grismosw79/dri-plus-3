"""Read-only replay of the original 48 recruits' actual saved W7 shortlists.
Uses the compiled detour and native fallback, not a full league simulation.
"""
from pathlib import Path
import json,struct
src=(Path(__file__).parent/'test28r11_committable_test.py').read_text()
exec(compile(src.split('reset29()\nr,p=make(91000')[0],'shared fixture','exec'))
data=json.loads((ROOT/'Tests/Fixtures/test28r13_w7_shortlists.json').read_text())
stage={'Top10':0,'Top5':1,'Top3':2,'Battle':3,'SoftCommitted':4,'HardCommitted':5,'Signed':6}
entry=call(exports['DriTestHsShortlistHook'])
uc.mem_write(GAME+0x0862d990,b'\xff\x25\0\0\0\0'+struct.pack('<Q',entry)+b'\x90\x90')
results=[]
for x in data['recruits']:
 reset29();st=stage[x['RecruitStage']]
 r,p=make(x['rid'],tuple((z['TeamId'],z['TeamInfluence']) for z in x['top']),[],ovr=x['ovr'],stage=st)
 records[r]['scalar'][3]=x['CommitScore'];before=snap8(r)
 native=[];opened=[]
 for tid in range(138):
  configure(0);native.append(locked(r,tid));configure(1);opened.append(locked(r,tid))
 assert snap8(r)==before
 assert (not any(opened)) if st<5 else (opened==native)
 results.append({'rid':x['rid'],'name':x['name'],'stage':x['RecruitStage'],'native_shortlist_exclusions':sum(native),'R13_shortlist_exclusions':sum(opened),'records_unchanged':True})
assert len(results)==48 and sum(stage[x['stage']]>=5 for x in results)==15
call(exports['DriTestHs29Reset']);call(exports['DriTestOwnershipReset'])
assert all(v==1 for v in refs.values()) and call(exports['DriTestQualityFault'])==0
report={'source_save_sha256':data['source_save_sha256'],'saved_recruits':48,'school_comparisons':48*138,'unsigned':33,'hard_committed':15,'previous_exclusions_removed':sum(x['native_shortlist_exclusions'] for x in results if stage[x['stage']]<5),'results':results,'live_game_verified':False,'scope':'Actual saved shortlist fields, compiled hook, captured native fallback; school eligibility/capacity/spending outcomes are not simulated by this check.'}
(OUTPUT/'test28r13_saved_lists_results.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:v for k,v in report.items() if k!='results'},indent=2))
