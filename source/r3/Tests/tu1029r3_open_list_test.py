"""Compiled full-market recovery: actual PRE list fields plus malformed controls.
SDF storage, native eligibility and Windows services are controlled, not a live advance.
Run with DRI_EXPECT_PREVIOUS_FAULT=1 against the retained R2 fixture for the negative control.
"""
from pathlib import Path
import copy,os,json
src=(Path(__file__).parent/'test28r8_abandoned_test.py').read_text()
exec(compile(src.split('reset8();r,p=recruit8(85000)')[0],__file__,'exec'))
data=json.loads((ROOT.parent/'Audit/TU1029R3/Inputs/pre_interest.json').read_text())
positions='QB HB FB WR TE LT LG C RG RT LE RE DT LOLB MLB ROLB CB FS SS K P LS'.split()
stages={'Top10':0,'Top5':1,'Top3':2,'Battle':3,'SoftCommitted':4,'HardCommitted':5,'Signed':6}
classes={'HighSchool':0,'JuniorCollege_Sophomore':1,'JuniorCollege_Junior':2,'JuniorCollege_Senior':3}
stars={'ONE_STAR':0,'TWO_STAR':1,'THREE_STAR':2,'FOUR_STAR':3,'FIVE_STAR':4}

def saved(x):
    r,p=rr(x['rid'],score=x['threshold'],ovr=x['player']['ovr'],stage=stages[x['stage']],
           schools=tuple((e['tid'],e['points'])for e in x['top']),offers=(),portal=False)
    records[r]['scalar'][2]=classes[x['cls']]
    records[p]['scalar'].update({3:positions.index(x['player']['pos']),231:stars[x['player']['stars']],261:x['player']['team']})
    if int(x['topReference'],2)==0:records[r]['ref'].pop(13)
    return r,p

def reset_pre():
    reset8();call(exports['DriTestHs29Reset']);allowed8.clear()
    records[season]['scalar'].update({2:2026,4:0,5:0})

case=next(x for x in data['all']if x['rid']==34)
assert case['player']['name']=='Iosefa Bootle' and not int(case['topReference'],2) and case['stage']=='Top10'
reset_pre();r,p=saved(case);before=copy.deepcopy(records)
negative=os.environ.get('DRI_EXPECT_PREVIOUS_FAULT')=='1'
recover8(fault=int(negative));assert records==before and not native_events
assert not call(exports['DriTestHs28WasRecovered'],r)
call(exports['DriTestOwnershipReset']);assert all(v==1 for v in refs.values())
if negative:
    result={'passed':True,'previous_fault_reproduced':True,'recruit_row':34,'save_mutation':False,'live_game_verified':False}
    (OUTPUT/'tu1029r3_previous_fault.json').write_text(json.dumps(result,indent=2)+'\n')
    print('PREVIOUS_FAULT_REPRODUCED',json.dumps(result));raise SystemExit(0)
checks=['Original PRE row34 null Top10 list does not fault, mutate records or reserve recovery']
# An absent narrowed list remains fatal. An allocated empty list has the same
# stage-dependent result as before; malformed content is never silently skipped.
for stage in range(5):
    for shape in ['null','empty','nonarray','duplicate_tid','duplicate_record','invalid_tid','invalid_points','too_many','null_entry']:
        reset_pre();r,p=recruit8(87000+stage,stage=stage)
        top=records[r]['ref'][13];entries=records[top]['array']
        if shape=='null':records[r]['ref'].pop(13)
        elif shape=='empty':entries.clear()
        elif shape=='nonarray':records[top]['array']=None
        elif shape=='duplicate_tid':records[entries[1]]['scalar'][0]=10
        elif shape=='duplicate_record':entries[1]=entries[0]
        elif shape=='invalid_tid':records[entries[0]]['scalar'][0]=138
        elif shape=='invalid_points':records[entries[0]]['scalar'][1]=65536
        elif shape=='too_many':records[top]['array']=[record('many',{0:i,1:0})for i in range(65)]
        elif shape=='null_entry':entries[0]=0
        # Model a reference to an object that does not implement ARRAY.
        def typed_array():
            ar,out,tag,_=args();assert tag==0x41525259
            if records[ar]['array'] is None:wq(out,0);return out
            return query_array()
        api(typed_array,address=rq(VT+0x28))
        before=copy.deepcopy(records);expected=int(not(stage==0 and shape in ['null','empty']))
        recover8(fault=expected);assert records==before,(stage,shape)
        assert not native_events and not call(exports['DriTestHs28WasRecovered'],r),(stage,shape)
        call(exports['DriTestOwnershipReset']);assert all(v==1 for v in refs.values()),(stage,shape)
checks.append('45 stage/list cases: only null or empty Top10 skipped; narrowed missing lists, wrong interfaces, duplicates, bounds and unreadable elements still reject without mutation')
# Subsequent native population is observed on the next scan; no permanent skip.
reset_pre();allowed8.add(96);r,p=saved(case)
records[r]['scalar'][3]=1000
recover8();assert not call(exports['DriTestHs28WasRecovered'],r)
records[r]['ref'][13]=record('later native interest',array=[record('later school',{0:10,1:1000})])
before=copy.deepcopy(records);recover8();assert records==before and call(exports['DriTestHs28WasRecovered'],r)
assert not offer_status(r,96) and not native_events
checks.append('Later populated, progressed open list can reserve a qualified destination; no offer, influence or signing fabricated')
# Original 3,200 saved lists, after the existing PRE pass has reopened row1617.
# No candidate is made eligible for a synthetic destination in this read test.
reset_pre();replayed=[]
for x in data['all']:
    r,p=saved(x)
    if x['rid']==1617:records[r]['scalar'][9]=0
    replayed.append(r)
assert len(replayed)==3200
before=copy.deepcopy(records);recover8();assert records==before and not native_events
assert not any(call(exports['DriTestHs28WasRecovered'],r)for r in replayed)
call(exports['DriTestOwnershipReset']);assert all(v==1 for v in refs.values())
checks.append('All 3,200 actual PRE interest lists complete recovery inspection with no fault or record writes; native eligibility controlled false, not a season simulation')
result={'passed':True,'checks':checks,'saved_recruits':3200,'shape_stage_cases':45,'previous_failure_row':34,
        'records_unchanged':True,'references_balanced':True,'live_game_verified':False,
        'limits':['Native storage, eligibility and Windows services controlled.','No native offers, weekly advance or live performance demonstrated.','PRE row1617 Top3->Top10 is the observed earlier coordinator action; no other interest fields altered.']}
(OUTPUT/'tu1029r3_open_list_results.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
