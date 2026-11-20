"""Captured native initial-interest list and direct stage-write loop.

The original condition evaluator runs with controlled expression leaf services.
This traces creation-time behavior; it does not reconstruct a live generation.
"""
from pathlib import Path
import json,hashlib
tests=Path(__file__).resolve().parent
src=(tests/'runtime_fixture_test.py').read_text()
exec(compile(src.split('\nfill_hs_position(0,ideals[0])\n')[0],str(tests/'runtime_fixture_test.py'),'exec'))
def native_api(offset,fn):return api(fn,GAME+offset)
for begin,end in [(0x08154cc0,0x081551d3),(0x08627910,0x08627ba0)]:
    uc.mem_map(GAME+begin,end-begin)
    uc.mem_write(GAME+begin,raw[begin-0x1000:end-0x1000])
uc.mem_map(GAME+0x0da92840,8)
native_api(0x078cb430,lambda:reg('RAX'))
writes=[];evaluations=[]
def write_scalar():
    table,index,field,value=args();r=tables[table][index]
    records[r]['scalar'][field-0x100000]=value&0xffffffff
    if field==0x100009:writes.append((ri(r+0x18),value))
    return 0
wq(TABLEVT+0x148,api(write_scalar))
native_api(0x08059580,lambda:0)
native_api(0x0862db70,lambda:0)
def create_pair():
    manager,out,_kind,_=args();r=record('new native TopSchool pair')
    incref(r);wq(out,r);return out
native_api(0x08166cb0,create_pair)
def append_pair():
    target,holder,_,_=args();r=rq(holder);records[target]['array'].append(r);return 0
native_api(0x08045cd0,append_pair)
transitions=[record('initial transition '+str(s),{1:s}) for s in range(1,6)]
metadata=[record('stage metadata '+str(s),ref={0:t}) for s,t in enumerate(transitions)]
def stage_metadata():
    manager,out,stage,_=args();r=metadata[stage] if stage<len(metadata) else 0
    incref(r);wq(out,r);return out
native_api(0x081793a0,stage_metadata)
def variant_type():wi(reg('RCX'),reg('RDX'));return reg('RCX')
def copy_variant():
    dest,source,_,_=args();uc.mem_write(dest,bytes(uc.mem_read(source,24)))
    if ri(source) in [7,9]:incref(rq(source+8))
    return dest
def expression():
    out,ref,field,other=args();assert field==0x100002 and rq(ref)==rq(other)
    transition=rq(ref);vector=rq(reg('RSP')+0x28);begin=rq(vector)
    r=rq(begin+8);assert rq(begin+32)==tuning
    next_stage=records[transition]['scalar'][1]
    points=records[records[records[r]['ref'][13]]['array'][0]]['scalar'][1]
    threshold=records[r]['scalar'][3]
    passed=points*100 >= threshold*([0,20,40,60,100,100][next_stage])
    evaluations.append((ri(r+0x18),next_stage,points,threshold,passed))
    wi(out,1);wq(out+8,int(passed));return out
native_api(0x0806cf90,variant_type);native_api(0x037eec00,copy_variant);native_api(0x08069b20,expression)

def initialize(rid,points,threshold,stage=0,existing=False,offer=0,cls=0):
    writes.clear();evaluations.clear()
    player=record('PRE player '+str(rid),{3:1,191:73,231:3,261:255})
    prior=record('already populated pair',{0:TEST_TEAM_ID,1:points}) if existing else 0
    top=record('PRE top list',array=[prior] if prior else [])
    r=record('PRE recruit '+str(rid),{2:cls,3:threshold,9:stage,14:int(offer>=2)},ref={5:player,13:top},row_id=rid)
    if offer:
        records[arr]['array'].append(record('actual initial offer '+str(rid),{12:offer},ref={10:r}))
    holder=alloc(8);wq(holder,r)
    entries=alloc(16);wq(entries,team);wi(entries+8,0);wi(entries+12,points)
    vector=alloc(24);wq(vector,entries);wq(vector+8,entries+16);wq(vector+16,entries+16)
    call(GAME+0x08154cc0,eval_ptr,holder,vector)
    pairs=[(records[p]['scalar'][0],records[p]['scalar'][1]) for p in records[top]['array']]
    assert pairs==[(TEST_TEAM_ID,points)] and records[r]['scalar'][14]==int(offer>=2)
    assert records[player]['scalar'][261]==255
    assert all(v==1 for v in refs.values()),[(records[k]['name'],v) for k,v in refs.items() if v!=1]
    return {'rid':rid,'points':points,'threshold':threshold,'stage':records[r]['scalar'][9],
            'writes':[x[1] for x in writes],'evaluations':list(evaluations),'offers':int(offer>=2),'owner':255}

legacy=[initialize(310310,584,459),initialize(310311,577,530),initialize(310312,582,565)]
assert all(r['stage']==5 and r['writes']==[1,2,3,4,5] for r in legacy),legacy
below=initialize(310313,459,500);assert below['stage']==3 and below['writes']==[1,2,3]
populated=initialize(310314,584,459,stage=0,existing=True)
assert populated['stage']==0 and not populated['writes'] and not populated['evaluations']
report={'status':'NATIVE_BYPASS_REPRODUCED','native_code_sha256':hashlib.sha256(raw).hexdigest(),
        'native_initial_interest_function':'08154cc0..081551d2','direct_stage_write':'08155125',
        'first_initialization_only_guard':'08154eb0 test eax,eax / jne08155191',
        'native_initial_interest_cases':legacy,'below_commit_threshold':below,'already_populated_list':populated,
        'condition_expression_leaf':'controlled thresholds with captured08627910 evaluator',
        'full_FTC_VM_executed':False,'live_game_verified':False}
(OUTPUT/'test31_initial_interest_trace.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
if os.environ.get('DRI31_PRE_TRACE_ONLY')=='1':raise SystemExit(0)
assert 'DriTest31InitialHook' in exports,'Creation-time compiled hook is required for release acceptance'
configure(1)
# Real record lookup reaches the listed school; no aggregate offer counter is
# substituted for its board's actual target status.
other_teams=[record('PRE other team '+str(i),{383:i},ref={33:record('other incoming '+str(i),array=[])}) for i in range(2)]
teams=record('PRE teams',array=other_teams+[team])
records[hs_fixture_franchise]['ref'][45]=teams
records[season]['scalar'].update({4:0,5:0})
hook=call(exports['DriTest31InitialHook'])
# call() returns the low32 bits for ordinary scalar exports. Read full pointer.
hook=reg('RAX')
uc.mem_write(GAME+0x081550fc,b'\xff\x25\0\0\0\0'+struct.pack('<Q',hook)+b'\x90\x90')
uc.ctl_remove_cache(GAME+0x08154cc0,GAME+0x081551d3)
guarded=[initialize(310320,584,459),initialize(310321,577,530),initialize(310322,582,565)]
assert all(r['stage']==3 and r['writes']==[1,2,3] for r in guarded),guarded
assert call(exports['DriTestQualityFault'])==0
guard_below=initialize(310323,459,500)
assert guard_below['stage']==3 and guard_below['writes']==[1,2,3]
existing_hard=initialize(310324,584,459,stage=5,existing=True)
assert existing_hard['stage']==5 and not existing_hard['writes']
configure(0);diagnostic=initialize(310325,584,459)
assert diagnostic['stage']==5
configure(1)
# Native eligibility is a controlled leaf; the production callback still
# performs its exact manager-layout/type validation and calls that leaf.
manager=alloc(0x300);wq(eval_ptr+0xc8,manager)
for offset in [0x80,0xa0,0x180,0x1b0]:wq(manager+offset,0xabcdef)
native_api(0x08059c00,lambda:1)
eligible=True
native_api(0x0816b130,lambda:int(eligible))
records[team]['ref'][407]=record('actual user coach')
earned=initialize(310326,584,459,offer=3)
assert earned['stage']==5 and earned['writes']==[1,2,3,4,5],earned
eligible=False;ineligible=initialize(310327,584,459,offer=3)
assert ineligible['stage']==3
eligible=True;withdrawn=initialize(310328,584,459,offer=1)
assert withdrawn['stage']==3
# Transfer class initialization is outside this HS/JUCO correction.
portal_case=initialize(310329,584,459,cls=4)
assert portal_case['stage']==5
assert call(exports['DriTestQualityFault'])==0
report.update(status='PASS',guarded_native_initial_interest_cases=guarded,
              guarded_below_threshold=guard_below,existing_secured_preserved=existing_hard,
              actual_eligible_user_offer=earned,ineligible_offer=ineligible,withdrawn_offer=withdrawn,
              diagnostic_mode=diagnostic,transfer_class_unchanged=portal_case,
              compiled_assembly_hook_verified=True,points_and_owner_unchanged=True,references_balanced=True)
(OUTPUT/'test31_initial_interest_results.json').write_text(json.dumps(report,indent=2)+'\n')
print('PASS actual compiled assembly guard inside captured native initializer; native1-3 and real eligible offer preserved, zero/withdrawn/ineligible offers held before commitment')
