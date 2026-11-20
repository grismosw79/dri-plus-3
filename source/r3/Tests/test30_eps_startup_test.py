"""Test30 on-demand startup publication through the compiled grade callback.
Controlled SDF records and Windows snapshot I/O; captured native cache setters.
This does not claim reproduction of the historical practice exception.
"""
from pathlib import Path
src=(Path(__file__).parent/'test28r3_eps_publication_test.py').read_text().split('\nquery()\nif control:')[0]
exec(compile(src,'EPS publication fixture','exec'))
checks=[]
def grade(want):
 global next_heap
 heap_before=next_heap
 rsp=STACK+0xf0000-8;wq(rsp,STOP);uc.reg_write(UC_X86_REG_RSP,rsp)
 for name,v in zip(['RCX','RDX','R8','R9'],[my,teamref,0,0]):uc.reg_write(globals()['UC_X86_REG_'+name],v)
 uc.emu_start(exports['DriTestEpsGeneric'],STOP+1,count=150000000)
 assert reg('RIP')==STOP and reg('RAX')&0xffffffff==want
 assert call(exports['DriTestQualityFault'])==0
 next_heap=heap_before
def all_saved(want,rank):
 assert all(records[r]['scalar'][f]==want for r in grades for f in grade_fields)
 assert all(records[r]['scalar'][f]==rank for r in ranks for f in rank_fields)
records[season]['scalar'].update({3:0,4:0,5:0})
records[game]['scalar'].update({61:7,62:1})
grade(13);all_saved(13,0)
assert len(writes)==138*63*2
checks.append('First-season PRE grade query publishes N/A to all saved grade/rank cells without NativeEpsUpdate')
n=len(writes);nf=len(file_writes);grade(13)
assert len(writes)==n and len(file_writes)==nf
checks.append('Repeated same-scope query does not rewrite saved cells or disk')
for r in grades:
 for f in grade_fields:records[r]['scalar'][f]=8
records[season]['scalar'].update({3:1,4:1,5:1})
grade(13);all_saved(13,0)
call(exports['DriTestEpsReset']);grade(13);all_saved(13,0)
checks.append('W1 query and a fresh runtime restore the same held startup snapshot')
records[season]['scalar'][4]=8;grade(4);all_saved(4,39)
records[season]['scalar'][4]=9;grade(4);all_saved(4,39)
checks.append('W8 on-demand refresh publishes completed weeks 0..7 and W9 holds it')
records[season]['scalar'].update({3:2,4:0,5:6});grade(4)
assert final in files
call(exports['DriTestEpsReset'])
records[season]['scalar'].update({2:2027,6:1,3:0,4:0,5:0})
grade(4);all_saved(4,39)
checks.append('Next preseason carries and publishes the verified prior final after restart')
call(exports['DriTestEpsReset'])
records[season]['scalar'][24]=0;n=len(writes);nf=len(file_writes)
assert call(exports['DriTestEpsEnsure'],my,1,1)==0
assert len(writes)==n and len(file_writes)==nf and call(exports['DriTestQualityFault'])==0
checks.append('Inactive dynasty returns before any publication or snapshot write')
records[season]['scalar'].update({24:1,2:1800})
assert call(exports['DriTestEpsEnsure'],my,1,1)==0
assert len(writes)==n and len(file_writes)==nf and call(exports['DriTestQualityFault'])!=0
checks.append('Invalid season identity fails closed before publication')
call(exports['DriTestEpsReset']);assert all(v==1 for v in refs.values()) and not handles
(OUTPUT/'test30_eps_startup_results.json').write_text(json.dumps({'passed':True,'checks':checks,'practice_fault_fixed':False,'live_game_verified':False},indent=2)+'\n')
print(json.dumps(checks,indent=2))
