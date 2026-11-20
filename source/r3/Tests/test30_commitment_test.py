"""Test30 compiled HS callback and captured native condition evaluator.
The SDF expression leaf has controlled threshold/boolean semantics; the real
captured evaluator builds arguments, dispatches the expression and returns it.
This proves ABI, choice/restore and freshness, not live FTC-expression results.
"""
from pathlib import Path
import json
src=(Path(__file__).parent/'test28r11_committable_test.py').read_text()
exec(compile(src.split('reset29()\nr,p=make(91000')[0],'HS fixture','exec'))
legacy_reset29=reset29
def reset29():
    legacy_reset29()
    if 'DriTest31ClassAnnual' in exports:
        # These transition controls require exactly one primary and one
        # conditional partner at each school. A toy QB ideal1 may now have
        # zero annual places; use the normal four-QB scheme and a known year
        # with one approved annual QB place for all three participating teams.
        records[ideal_entries[0]]['scalar'][2]=4
        records[season]['scalar'][2]=2027
        annual_out=alloc(46*4)
        for tid in [10,11,12]:
            assert call(exports['DriTest31ClassAnnual'],allteams[tid],context,annual_out)==1
            assert ri(annual_out+4)==1,tid
checks=[];evaluations=[];extra_condition=True;withdraw_during_condition=False;fault_during_condition=False
# Captured evaluator, unchanged bytes; mock only SDF expression/value services.
start,end=0x08627910,0x08627ba0
uc.mem_map(GAME+start,end-start)
uc.mem_write(GAME+start,raw[start-0x1000:end-0x1000])
uc.mem_map(GAME+0x0da92840,8)
game_api(0x078cb430,lambda:reg('RAX')) # native cookie check preserves the return value
def variant_type():
    wi(reg('RCX'),reg('RDX'));return reg('RCX')
def copy_variant():
    dest,src,_,_=args();uc.mem_write(dest,bytes(uc.mem_read(src,24)))
    if ri(src) in [7,9]:incref(rq(src+8))
    return dest
def expression():
    out,ref,field,other=args();assert field==0x100002 and rq(ref)==rq(other)
    transition=rq(ref);vector=rq(reg('RSP')+0x28);begin=rq(vector)
    r=rq(begin+8);tun=rq(begin+24+8)
    assert tun==tuning and records[transition]['scalar'][1]==4
    leader,points=values(r)[0];evaluations.append((ri(r+0x18),leader,points))
    if withdraw_during_condition:
        for target in records[board_targets(allteams[leader])]['array']:
            if records[target]['ref'].get(10)==r:records[target]['scalar'][12]=1
    if fault_during_condition:records[allteams[leader]]['ref'][64]=0
    result=int(points>=records[r]['scalar'].get(3,1000) and extra_condition)
    wi(out,1);wq(out+8,result);return out
game_api(0x0806cf90,variant_type);game_api(0x037eec00,copy_variant);game_api(0x08069b20,expression)
def own(r,tid):
    wr=alloc(32);wq(wr,GAME+0x0bd2e980);wq(wr+8,r);wq(wr+16,0xabcdef);wq(wr+24,1);tr=alloc(8);wq(tr,allteams[tid])
    return call(GAME+0x0862d6d0,wr,tr)&255
def native_attempt(r,next_stage=4):
    frame=alloc(0x180);bp=alloc(0x200)+0x80;transition=record('HS native transition',{1:next_stage})
    wq(frame+80,bp);wq(frame+128+0x30,GAME+0x0bd2eea0);wq(frame+128+0x38,transition);wq(frame+128+0x40,0xabcdef)
    wq(bp+0x68,r);wq(bp+0x60,context)
    registers=bytes(uc.mem_read(frame,128));stage=records[r]['scalar'][9];before=dict(values(r))
    result=call(hooks[16],frame)
    assert bytes(uc.mem_read(frame,128))==registers
    assert dict(values(r))==before and records[r]['scalar'][9]==stage
    return result
# Actual captured conditional branch: false skips our hook entirely.
uc.mem_map(GAME+0x081b98e5,0x120)
uc.mem_write(GAME+0x081b98e5,raw[0x081b98e5-0x1000:0x081b98ed-0x1000])
def transition_branch(u,address,size,data):
    if address in [GAME+0x081b98ed,GAME+0x081b9a01]:u.emu_stop()
branch_hook=uc.hook_add(unicorn.UC_HOOK_CODE,transition_branch)
for value,want in [(0,0x081b9a01),(1,0x081b98ed)]:
    uc.reg_write(UC_X86_REG_RAX,value);uc.emu_start(GAME+0x081b98e5,STOP,count=5)
    assert reg('RIP')==GAME+want
uc.hook_del(branch_hook)
checks.append('Captured original false-condition branch skips the DLL callback; no transition can be manufactured')
# This is the screenshot failure omitted by R1's already-higher-interest case.
reset29();r,p=make(30001,((10,73),(11,581),(12,433)),[10],73,2)
call(exports['DriTest29R1HsOrder'],r,context)
assert values(r)==[(11,581),(12,433),(10,73)]
assert records[r]['scalar'][9]==2
checks.append('Garr-shaped low-interest firm school does not lead the open list; exact earned pairs and stage retained')
# Qualified alternative succeeds only after the SAME native evaluator returns true.
reset29();r,p=make(30002,((10,1200),(11,1100),(12,1050)),[10,11,12],78,3);primary=conditional(30003,10)
assert call(exports['DriTestHs29Offer'],r,allteams[10],context)==1
assert native_attempt(r)==0 and values(r)==[(11,1100),(10,1200),(12,1050)]
assert evaluations[-1]==(30002,11,1100)
# The selected first school survives ordinary coordinator ordering once native
# has published the soft stage; current stage writes belong to native code.
records[r]['scalar'][9]=4;chosen=values(r);call(exports['DriTest29R1HsOrder'],r,context);assert values(r)==chosen
records[r]['scalar'][9]=5
assert own(r,11)==1 and own(r,10)==0
checks.append('Highest-interest firm alternative passes captured native evaluator; native own-commit helper recognizes precisely that chosen destination')
# Below-threshold alternative stays open; original order restored exactly.
reset29();r,p=make(30004,((10,1200),(11,73),(12,70)),[10,11],78,3);conditional(30005,10)
before=values(r);assert native_attempt(r)==1 and values(r)==before
checks.append('Low-interest firm option cannot borrow a conditional leader\'s native success; original list restored')
# Preserve additional native expression requirements even above the threshold.
reset29();r,p=make(30006,((10,1200),(11,1100)),[10,11],78,3);conditional(30007,10)
extra_condition=False;before=values(r);assert native_attempt(r)==1 and values(r)==before;extra_condition=True
checks.append('Additional native condition rejection is authoritative even when firm school exceeds threshold')
# Re-check school acceptance after expression dispatch, not a stale firm count.
reset29();r,p=make(30008,((10,1200),(11,1100)),[10,11],78,3);conditional(30009,10)
withdraw_during_condition=True;before=values(r);assert native_attempt(r)==1 and values(r)==before;withdraw_during_condition=False
checks.append('Offer withdrawal during condition invalidates previously firm alternative and restores exact preference order')
# User school is an eligible choice without CPU primary allocation.
reset29();r,p=make(30010,((10,1200),(11,1100)),[10,11],78,3);conditional(30011,10)
records[allteams[11]]['ref'][407]=record('user coach')
assert native_attempt(r)==0 and values(r)[0]==(11,1100)
checks.append('Real eligible user offer remains a valid native-qualified destination')
# Equal firm interest preserves existing preference tie order, not team ID.
reset29();r,p=make(30020,((10,1200),(12,1100),(11,1100)),[10,11,12],78,3);conditional(30021,10)
assert native_attempt(r)==0 and values(r)[0]==(12,1100)
checks.append('Equal-interest firm alternatives retain their existing preference order')
# Native first school already firm: no extra expression execution or reorder.
reset29();r,p=make(30012,((10,1200),(11,1100)),[10,11],78,3)
before=values(r);n=len(evaluations);assert native_attempt(r)==0 and values(r)==before and len(evaluations)==n
checks.append('Existing firm leader follows unchanged native path with no repeated expression evaluation')
# Acceptance freshness after a primary signs elsewhere; index only identifies
# boards and does not cache the conditional/firm result.
reset29();r,p=make(30013,((10,1200),(11,1100)),[10,11],78,3);primary_rec,primary_player=conditional(30014,10)
assert call(exports['DriTestHs29Offer'],r,allteams[10],context)==1
records[primary_rec]['scalar'][9]=5;entry=records[records[primary_rec]['ref'][13]]['array'][0];records[entry]['scalar'][0]=12
assert call(exports['DriTestHs29Offer'],r,allteams[10],context)==2
assert native_attempt(r)==0 and values(r)[0]==(10,1200)
checks.append('Primary signs elsewhere: backup becomes firm immediately without a week or reload')
# A soft recruit losing its own actual destination reopens but retains points.
reset29();r,p=make(30015,((10,1200),(11,1100)),[11],78,4)
before=dict(values(r));call(exports['DriTestHs29Reopen'],context)
assert records[r]['scalar'][9]==0 and dict(values(r))==before
checks.append('Lost soft destination reopens for recovery without deleting earned interest')
# A fault encountered while verifying the alternate cannot permit the already
# rejected original leader. The exact temporary view is still restored.
reset29();r,p=make(30030,((10,1200),(11,1100)),[10,11],78,3);conditional(30031,10)
saved_config=records[allteams[11]]['ref'][64];before=values(r);fault_during_condition=True
assert native_attempt(r)==1 and values(r)==before and call(exports['DriTestQualityFault'])==1
fault_during_condition=False;records[allteams[11]]['ref'][64]=saved_config;configure(1)
checks.append('Fresh acceptance decoding fault blocks this transition after restoring preferences; no conditional-leader fallthrough')
call(exports['DriTestHs29Reset']);call(exports['DriTestOwnershipReset'])
assert call(exports['DriTestQualityFault'])==0
assert all(v==1 for v in refs.values()),[(records[r]['name'],v) for r,v in refs.items() if v!=1]
checks.append('All record references balanced, frame preserved, no fault')
(OUTPUT/'test30_commitment_results.json').write_text(json.dumps({'checks':checks,'native_condition_dispatches':evaluations,'captured_evaluator_rva':'0x08627910','controlled_expression_leaf':True,'live_game_verified':False,'limit':'Existing hook executes only after original native expression succeeds; no fabricated transition on original false'},indent=2)+'\n')
print(json.dumps(checks,indent=2))
