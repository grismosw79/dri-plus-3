"""R8 HS rival-board cleanup through captured native RemoveFromBoard.

Uses the compiled cleanup authorization hook, native offer/hour removal, and
controlled SDF records. No live season outcome is implied.
"""
from pathlib import Path
fixture_path=Path(__file__).parent/'portal_cleanup_test.py'
source=fixture_path.read_text().split('\nreset();r,p=make(20000)')[0]
exec(compile(source,str(fixture_path),'exec'))
hs_pool=record('HS cleanup class',array=[])
records[franchise]['ref'][39]=hs_pool
old_reset=reset
def reset():
 old_reset();records[hs_pool]['array']=[]
def hs(rid,stage=4,leader=1):
 r,p=make(rid,stage=stage,leader=leader,is_portal=False)
 records[hs_pool]['array'].append(r)
 return r,p
def hs_clean(tid=None):
 result=call(exports['DriTestHsRivalCleanup'],schools[tid] if tid is not None else 0,context)
 assert result==1 and call(exports['DriTestQualityFault'])==0
 return result
checks=[]

# Confirmed acquisition is authoritative even while recruit state looks open.
for stage in [0,1,3,4,5,6]:
 reset();r,p=hs(63000+stage,stage=stage)
 loser=target(r,1,visit_hours=25);winner=target(r,2);user=target(r,3,user=True)
 records[commits[2]]['array']=[p]
 before_interest=list(records[records[r]['ref'][13]]['array'])
 if stage>=5:assert direct_remove(r,1)==7
 hs_clean()
 assert records[boards[1]]['array']==[] and records[loser]['ref'][10]==0
 assert records[boards[2]]['array']==[winner] and records[boards[3]]['array']==[user]
 assert records[commits[2]]['array']==[p] and records[r]['scalar'][9]==stage
 assert records[r]['scalar'][14]==2
 assert records[records[r]['ref'][13]]['array']==before_interest
 assert ('refund_hours',25) in native_events
 assert not any(e[0]=='user_hours_adjust' for e in native_events)
 before=snapshot();hs_clean();assert snapshot()==before
checks.append('Published HS stages 0/1/3/4/5/6 lose only CPU rival targets, preserving acquiring school, user board, influence identity, stage and acquired Player; repeat sweep is inert')

reset();r,p=hs(63100,stage=5,leader=2)
loser=target(r,1);winner=target(r,2)
hs_clean(1)
assert records[boards[1]]['array']==[] and records[boards[2]]['array']==[winner]
assert not records[commits[2]]['array'] and records[r]['scalar'][9]==5
checks.append('Offer-backed native hard ownership frees rival board capacity before incoming-class publication')

reset();orphan,op=hs(63101,stage=5,leader=2);target(orphan,1)
soft,sp=hs(63102,stage=4,leader=2);target(soft,1);target(soft,2)
unknown,up=hs(63103,stage=6,leader=2);target(unknown,1);target(unknown,2)
before=snapshot();hs_clean();assert before==snapshot() and not native_events
checks.append('Offerless hard leader, unacquired soft commitment and unknown Signed destination do not imply ownership')

reset();r,p=hs(63110,stage=6,leader=1);target(r,1);target(r,2);records[commits[2]]['array']=[p]
records[schools[1]]['ref'][407]=record('user control')
before=snapshot();hs_clean(1);assert before==snapshot() and not native_events
checks.append('Explicit user-team cleanup request passes through without touching its board')

reset();r,p=hs(63111,stage=6,leader=1);target(r,1);target(r,2);target(r,4);records[commits[2]]['array']=[p]
hs_clean(1)
assert not records[boards[1]]['array'] and len(records[boards[4]]['array'])==1
hs_clean(4);assert not records[boards[4]]['array'] and len(records[boards[2]]['array'])==1
checks.append('Per-school safe-boundary cleanup does not compact another CPU board being processed elsewhere')

for mode_value in [0,-1]:
 reset();r,p=hs(63120+mode_value,stage=6);target(r,1);target(r,2);records[commits[2]]['array']=[p]
 configure(mode_value);before=snapshot();assert call(exports['DriTestHsRivalCleanup'],0,context)==1
 assert before==snapshot() and not native_events
checks.append('Diagnostic and disabled modes preserve all HS data')

reset();r,p=hs(63130,stage=4);target(r,1);target(r,2)
records[commits[2]]['array']=[p];records[commits[4]]['array']=[p]
before=snapshot();assert call(exports['DriTestHsRivalCleanup'],0,context)==0
assert call(exports['DriTestQualityFault'])==1 and snapshot()==before and not native_events
checks.append('Conflicting published HS owner fails preflight before any target or offer changes')

reset();r,p=hs(63131,stage=4);target(r,1);target(r,1);target(r,2);records[commits[2]]['array']=[p]
before=snapshot();assert call(exports['DriTestHsRivalCleanup'],0,context)==0
assert call(exports['DriTestQualityFault'])==1 and snapshot()==before and not native_events
checks.append('Duplicate rival target is refused before a partially applied cleanup')
call(exports['DriTestOwnershipReset']);configure(1)
assert all(n==1 for n in refs.values()),[(records[r]['name'],n)for r,n in refs.items()if n!=1]
checks.append('Record references balance across successful, repeated, excluded and rejected sweeps')
result={'checks':checks,'native_remove':'081A3DA0..081A4722','compiled_hook':'081A409E','live_game_verified':False}
(OUTPUT/'test28r8_hs_rival_cleanup_results.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
