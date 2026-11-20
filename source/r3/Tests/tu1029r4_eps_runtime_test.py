"""Compiled Windows EPS refresh: discrepancies are nonfatal, bad data is fatal.
Actual DLL functions; controlled native records, Windows I/O and captured code.
"""
from pathlib import Path
src=(Path(__file__).parent/'test28r3_eps_publication_test.py').read_text().split('\nquery()\nif control:')[0]
exec(compile(src,'EPS publication fixture','exec'))
records[season]['scalar'].update({3:1,4:8,5:1})
records[game]['scalar'].update({61:7,62:1})
files.clear() # Must compute and publish, not merely reuse a frozen fixture.
for side in range(2):
 player=records[rosters[side]]['array'][0]
 stat=records[records[player]['ref'][146]]['array'][0]
 records[stat]['scalar'].update({11:1,5:1,24:1,19:1})
checks=[]
assert call(exports['DriTestEpsEnsure'],my,1,1)==1
assert call(exports['DriTestQualityFault'])==0
assert len(writes)==138*63*2 and file_writes
computed=files[snapshot]
body=computed[56:]
assert struct.unpack_from('<iii',body)==(2026,2,138)
# EpsOutput layout is independently asserted by the existing publication test.
# Official team passing production is 100; player-record sum is only 1.
import ctypes as c
class Metric(c.Structure):_fields_=[(k,c.c_double)for k in ['value','observed','numerator','denominator','prior_exposure','prior_value']]
class Grade(c.Structure):_fields_=[('score',c.c_double),('rank',c.c_double),('grade',c.c_int),('status',c.c_int)]
class Output(c.Structure):_fields_=[('year',c.c_int),('rows',c.c_int),('teams',c.c_int),('priors',c.c_double*4),('exposure',(c.c_double*11)*138),('metric',(Metric*36)*138),('grade',(Grade*23)*138)]
assert c.sizeof(Output)==len(body)
out=Output.from_buffer_copy(body)
assert out.metric[0][27].numerator==100 and out.metric[0][11].numerator==1
checks.append('W8 computes and publishes despite large team/individual differences; official passing and recorded QB rushing remain distinct')
fw=len(file_writes);ww=len(writes)
records[season]['scalar'][4]=9
assert call(exports['DriTestEpsEnsure'],my,1,1)==1
assert len(file_writes)==fw and len(writes)==ww+138*63*2
checks.append('W9 holds and republishes the W8 result without another collection or snapshot write')
call(exports['DriTestEpsReset']);records[season]['scalar'][4]=8
assert call(exports['DriTestEpsEnsure'],my,1,1)==1
assert call(exports['DriTestQualityFault'])==0
checks.append('Restart reloads the matching verified snapshot with the same differing sources')
# A cached output must never hide an invalid negative individual attempt count.
call(exports['DriTestEpsReset'])
player=records[rosters[0]]['array'][0];stat=records[records[player]['ref'][146]]['array'][0]
records[stat]['scalar'][5]=-1
ww=len(writes);fw=len(file_writes)
assert call(exports['DriTestEpsEnsure'],my,1,1)==0
assert call(exports['DriTestQualityFault'])!=0
assert len(writes)==ww and len(file_writes)==fw
checks.append('Invalid individual counts still stop; an existing matching snapshot cannot bypass value validation')
call(exports['DriTestEpsReset'])
assert all(v==1 for v in refs.values()) and not handles
(OUTPUT/'tu1029r4_eps_runtime_results.json').write_text(json.dumps({'passed':True,'checks':checks,'reference_balance':True,'live_game_verified':False},indent=2)+'\n')
print(json.dumps(checks,indent=2))
