"""Compiled R11 stage decisions and unchanged native shortlist/lockout helpers.
Native eligibility/boards/rosters are fixtures; no live-season claim.
"""
from pathlib import Path
source=(Path(__file__).parent/'test28r8_abandoned_test.py').read_text()
exec(compile(source.split('reset8();r,p=recruit8(85000)')[0],'abandoned fixture','exec'))
checks=[]
def reset29():
    reset8()
    if 'DriTestHs29Reset' in exports:call(exports['DriTestHs29Reset'])
    records[tuning]['scalar'].update({0x3c:5,0x3b:3,0x36:3,0x39:1,0x38:1,2:25,0x56:10})
    allowed8.clear();allowed8.update(range(138))
    # One real QB vacancy per school; a primary and its backup share it.
    records[ideal_entries[0]]['scalar'][2]=1
def make(rid,school_points,offers,ovr=78,stage=0):
    r,p=rr(rid,score=1000,ovr=ovr,stage=stage,schools=school_points,offers=offers,portal=False)
    records[p]['scalar'].update({3:0,231:1,261:255})
    return r,p
def conditional(rid,tid):
    # Stronger native-scored primary is deliberately viable at its own school.
    r,p=make(rid,((tid,500),),[tid],ovr=95)
    return r,p
def attempt(r,next_stage):
    frame=alloc(0x180);bp=alloc(0x200)+0x80;transition=record('HS stage transition',{1:next_stage})
    wq(frame+80,bp);wq(frame+128+0x38,transition);wq(bp+0x68,r);wq(bp+0x60,context)
    before=bytes(uc.mem_read(frame,128));snapshot=snap8(r)
    result=call(hooks[16],frame)
    assert bytes(uc.mem_read(frame,128))==before and snap8(r)==snapshot
    return result
reset29()
r,p=make(91000,((10,900),(11,850),(12,800),(13,700)),[10,11,12,13])
primary=[conditional(91100+tid,tid) for tid in [10,11,12]]
assert [call(exports['DriTestHsPairDecision'],r,allteams[t],context,0) for t in [10,11,12,13]]==[17,17,17,0]
if 'DriTestHs29Stage' not in exports:
    assert attempt(r,2)==0
    records[r]['scalar'][9]=2
    assert locked(r,13)==1
    print('R10_NEGATIVE_REPRODUCED: native Top3 transition permitted; only immediately committable school13 becomes locked out')
    raise SystemExit(0)
assert [call(exports['DriTestHs29Offer'],r,allteams[t],context) for t in [10,11,12,13]]==[1,1,1,2]
assert attempt(r,1)==0 and attempt(r,2)==1
assert call(exports['DriTestHs29Next'],r,allteams[13],context,1)==1
checks.append('Three higher-interest conditional offers cannot lock out the fourth-ranked committable offer: Top5 allowed, Top3 held through the actual HS callback; points and stage unchanged')
# A user-controlled school with a real eligible offer is committable; no
# primary/backup plan or board edit is imposed on the human team.
records[allteams[12]]['ref'][407]=record('user coach')
assert attempt(r,2)==0
records[allteams[12]]['ref'].pop(407)
allowed8.remove(13);assert attempt(r,1)==1;allowed8.add(13)
checks.append('Eligible user offer counts without CPU role enforcement; an ineligible outside offer does not falsely justify narrowing')
# Live acceptance changes after another recruit actually commits. Numeric
# offer index may remain, but neither role nor eligibility is cached.
records[primary[2][0]]['scalar'][9]=5
assert call(exports['DriTestHs29Offer'],r,allteams[12],context)==0
assert attempt(r,2)==1
records[primary[1][0]]['scalar'][9]=5
top=records[records[primary[1][0]]['ref'][13]]['array'];records[top[0]]['scalar'][0]=99
assert call(exports['DriTestHs29Offer'],r,allteams[11],context)==2
assert attempt(r,2)==0
checks.append('Primary commits here: former backup becomes unavailable; primary commits elsewhere: backup becomes committable immediately and Top3 can advance')

reset29();r,p=make(91200,((10,950),(11,900),(12,850)),[10,11,12]);primaries=[conditional(91300+t,t) for t in [10,11,12]]
assert attempt(r,1)==1 and attempt(r,2)==1 and attempt(r,4)==1
checks.append('All conditional offers hold the recruit open; funded backup status cannot masquerade as an available commitment')

reset29();r,p=make(91400,((10,950),(11,900)),[10,11]);conditional(91401,10)
records[tuning]['scalar'][0x39]=2
assert attempt(r,2)==0 and attempt(r,4)==1
checks.append('A firm alternative can support Top3, but a conditional interest leader cannot become the soft-commit destination even when the native soft cut includes two schools')

# Native rank/tie/Battle rules are reused exactly for a proposed next stage,
# without writing a temporary stage into the save. Compare actual captured
# lockout across all four stages and alternate configured cut sizes.
cases=0
for cut in [3,5,8]:
    reset29();records[tuning]['scalar'][0x3c]=cut
    r,p=make(91500+cut,tuple((t,1000-(t-10)*100) for t in range(10,20)),[10])
    for stage in [1,2,3,4]:
        records[r]['scalar'][9]=stage
        for tid in range(10,21):
            want=int(not locked(r,tid));got=call(exports['DriTestHs29Next'],r,allteams[tid],context,stage)
            assert got==want,(cut,stage,tid,got,want);cases+=1
checks.append(f'{cases} comparisons match the actual captured native lockout across Top5/Top3/Battle/Soft stages, including configured 3/5/8-school cuts')

# W6 Robinson-shaped case: persisted Top3 excludes Houston-like school13;
# a safe boundary reopens it without manufacturing interest or changing offers.
reset29();r,p=make(91600,((10,422),(11,372),(12,68),(13,52)),[13],stage=2)
assert locked(r,13)==1;before=snap8(r)
call(exports['DriTestHs29Reopen'],context)
assert records[r]['scalar'][9]==0 and locked(r,13)==0
assert values(r)==list(before[1]) and records[r]['scalar']==before[0]|{9:0}
assert offer_status(r,13)
assert call(exports['DriTestHs29Offer'],r,allteams[13],context)==2
assert attempt(r,2)==1
checks.append('Reloaded stranded Top3 recruit reopens with exact earned interest and offers preserved; the next same cutoff remains held until a firm school can survive it')
for kind in ['hard','published','roster']:
    reset29();r,p=make(91700+len(kind),((10,900),),[],stage=5 if kind=='hard' else 4)
    if kind!='hard':records[records[allteams[10]]['ref'][33 if kind=='published' else 239]]['array']=[p]
    before=snap8(r);call(exports['DriTestHs29Reopen'],context);assert snap8(r)==before,kind
checks.append('Hard commitments, published acquisitions and roster-owned players are never reopened')

# Brian Doubs-shaped stale candidate: native condition can still be true after
# an earlier board withdrawal; accepting it must not invent an offered owner.
reset29();r,p=make(91800,((10,1000),),[],stage=3)
assert attempt(r,5)==1
valid_offer(r,destination=allteams[10]);assert attempt(r,5)==0
checks.append('HS hard-commit callback rejects a withdrawn offer even with a stale native candidate; reinstated real offer can commit under the same roster policy')
# Coverage is HS plus JUCO, independent of star level; one firm offer is
# sufficient. A missing offer is never created merely to permit a stage.
for cls in range(4):
    reset29();r,p=make(91900+cls,((10,1000),),[10]);records[r]['scalar'][2]=cls
    assert attempt(r,1)==0 and attempt(r,2)==0 and attempt(r,4)==0
reset29();r,p=make(91910,((10,1000),),[])
assert attempt(r,1)==1 and records[r]['scalar'][14]==0
checks.append('All four HS/JUCO classes may narrow with a single real committable offer; a zero-offer recruit stays open without a fabricated scholarship')
# Fresh coordinator scope sees an offer added after the prior read. Removing
# one within a weekly batch is detected by live board revalidation.
reset29();r,p=make(91920,((10,1000),),[])
assert attempt(r,1)==1
valid_offer(r,destination=allteams[10]);call(exports['DriTestHs29Reset'])
assert attempt(r,1)==0
for target in records[board_targets(allteams[10])]['array']:records[target]['scalar'][12]=1
assert attempt(r,1)==1
checks.append('Fresh batch picks up new offers; same-batch withdrawal is read live despite a previously indexed school')
# Malformed identities and duplicate ownership abort before the first write.
for issue in ['duplicate_interest','invalid_points','duplicate_owner']:
    reset29();r,p=make(91930+len(issue),((10,1000),(11,900)),[],stage=2)
    top=records[records[r]['ref'][13]]['array']
    if issue=='duplicate_interest':records[top[1]]['scalar'][0]=10
    elif issue=='invalid_points':records[top[0]]['scalar'][1]=65536
    else:
        for tid in [10,11]:records[records[allteams[tid]]['ref'][33]]['array']=[p]
    before=snap8(r);call(exports['DriTestHs29Reopen'],context)
    assert snap8(r)==before and call(exports['DriTestQualityFault'])==1,issue
checks.append('Malformed interest and conflicting acquired ownership stop custom behavior before reopening')
reset29();r,p=make(91970,((10,1000),),[])
for mode_value in [0,-1]:
    configure(mode_value);assert attempt(r,1)==0
configure(1);call(exports['DriTestHs29Reset']);call(exports['DriTestOwnershipReset'])
assert call(exports['DriTestQualityFault'])==0
assert all(v==1 for v in refs.values()),[(records[r]['name'],v) for r,v in refs.items() if v!=1]
checks.append('Passive/disabled modes preserve native behavior; all record references balanced and no runtime fault')
import os,sys,subprocess,shutil
control=OUTPUT/'r10-control';control.mkdir(exist_ok=True)
shutil.copy2(os.environ['DRI_TEST28R10_CONTROL_DLL'],control/'retention_fixture.dll')
proc=subprocess.run([sys.executable,__file__],env=dict(os.environ,DRI_TEST_OUTPUT_DIR=str(control)),capture_output=True,text=True)
(OUTPUT/'r10-stranded-backup-negative.log').write_text(proc.stdout+proc.stderr)
assert proc.returncode==0 and 'R10_NEGATIVE_REPRODUCED' in proc.stdout,proc.stdout[-2000:]+proc.stderr
checks.append('Rebuilt, byte-fingerprinted R10 source reproduces the stranded-backup transition under the same fixture')
(OUTPUT/'committable_results.json').write_text(json.dumps({'checks':checks,'native_lockout_comparisons':cases,'live_game_verified':False},indent=2)+'\n')
print(json.dumps(checks,indent=2))
