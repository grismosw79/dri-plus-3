"""Actual compiled EPS query/publication: 8-week holds, reloads and rewinds.
Controlled SDF + snapshot I/O; numerical model/collector have separate replay.
"""
from pathlib import Path
src=(Path(__file__).parent/'test28r3_eps_publication_test.py').read_text().split('\nquery()\nif control:')[0]
exec(compile(src,'EPS publication fixture','exec'))
records[season]['scalar'].update({3:1,4:5,5:1})
records[game]['scalar'].update({61:7,62:1})
checks=[]
def grade(want):
    global next_heap
    heap_before=next_heap
    rsp=STACK+0xf0000-8;wq(rsp,STOP);uc.reg_write(UC_X86_REG_RSP,rsp)
    for name,v in zip(['RCX','RDX','R8','R9'],[my,teamref,0,0]):uc.reg_write(globals()['UC_X86_REG_'+name],v)
    uc.emu_start(exports['DriTestEpsGeneric'],STOP+1,count=150000000)
    assert reg('RIP')==STOP
    assert reg('RAX')&0xffffffff==want,(records[season]['scalar'][4],reg('RAX'))
    assert call(exports['DriTestQualityFault'])==0
    next_heap=heap_before # DLL freed all refresh scratch allocations
# No grades based on four games at W5: the first refresh waits for week 8.
grade(13);firstwrites=len(file_writes)
records[season]['scalar'][4]=7;grade(13)
assert len(file_writes)==firstwrites
records[season]['scalar'][4]=8;grade(4)
checks.append('First-season W5/W7 remain N/A; W8 refresh reads only completed weeks 0..7')
# Add a new game at week 8, with invalid caches. Between refreshes this must
# never enter the collector, even after an explicit native-update publish.
game2=record('week8 game',{63:0,47:0,51:0,61:8,62:1},ref={42:teamlist[0],18:teamlist[1]},row_id=78)
for side in range(2):
    stat=record('week8 stats '+str(side),{24:50,19:10,11:100,5:10},ref={3:teamlist[1-side],4:game2});records[stat]['schema']=stat_schema
    player=records[rosters[side]]['array'][0];history=records[player]['ref'][146];records[history]['array'].append(stat)
records[season]['scalar'][4]=9;grade(4)
assert call(exports['DriTestEpsEnsure'],my,1,1)==1
assert all(records[r]['scalar'][f]==4 for r in grades for f in grade_fields)
records[season]['scalar'][4]=15;grade(4)
call(exports['DriTestEpsReset']);grade(4)
checks.append('W9..W15 and native-update publication hold W8 grades; fresh launch excludes later games and restores the same result')
# At W16, both games enter the snapshot identity. A verified model output
# with a different grade proves the refresh actually occurs at the boundary.
records[game2]['ref'].update({43:hs,19:aws})
sig2=1469598103934665603^4
for gid in [77,78]:
    for side in range(2):
        h=1469598103934665603
        for v in [gid,side,1-side,100,10,50,10,0]:h=((h^v)*1099511628211)&mask
        sig2=(sig2+h)&mask
payload2=bytearray(payload);struct.pack_into('<i',payload2,4,4)
for t in range(138):
    for p in range(23):struct.pack_into('<ddii',payload2,250656+(t*23+p)*24,80.,20.,2,1)
files[folder+f'\\b9e7c28d-2026-{sig2:016x}.bin']=struct.pack('<4Q6I',0x4550533154455354,sig2,fnv(payload2),0,3118973581,2026,0,0,len(payload2),1)+payload2
records[season]['scalar'][4]=16;grade(2)
records[season]['scalar'][4]=17;grade(2)
call(exports['DriTestEpsReset']);grade(2)
checks.append('W16 refresh includes weeks 0..15 and persists across a W17 reload')
records[season]['scalar'][4]=5;grade(13)
records[season]['scalar'][4]=8;grade(4)
checks.append('Rewinding to W5/W8 never imports a later block grade')
# Practice game deliberately has no opponent or caches. It cannot change
# the eligible game set or contaminate a scheduled refresh.
records[game2]['scalar'][51]=1;records[game2]['ref'].pop(43);records[game2]['ref'].pop(19)
call(exports['DriTestEpsReset']);records[season]['scalar'][4]=16;grade(4)
checks.append('Practice flag excludes a malformed practice record before reading team caches')
records[season]['scalar'].update({3:2,4:0,5:6});grade(4)
assert final in files
checks.append('Completed-season publication still refreshes and writes a verified final for portal/prior-year use')
call(exports['DriTestEpsReset']);assert all(v==1 for v in refs.values()) and not handles
(OUTPUT/'test28r12_eps_cadence_results.json').write_text(json.dumps({'checks':checks,'live_game_verified':False,'practice_fault_fixed':False},indent=2)+'\n')
print(json.dumps(checks,indent=2))
