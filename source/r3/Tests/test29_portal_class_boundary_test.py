"""Same-advance native signing callback invalidates a paid HS quantity pledge."""
from pathlib import Path
import json
base=(Path(__file__).parent/'test29_class_source_test.py').read_text()
exec(compile(base.split('# Four native funding entries:')[0],'test29_class_source_test.py','exec'))
assert reserved()==[1]+[0]*21
# The native board-signing hook accepts its existing legal HS primary. The
# actual native stage/publication then follows; the callback cannot sign it.
pf=alloc(0x800);pb=pf+0x400
wq(pf+10*8,pb);wq(pf+11*8,t);wq(pf+128+0x30,primary);wq(pb+0x57,context)
assert call(sign_cb,pf)==0
records[primary]['scalar'][9]=6
records[records[t]['ref'][33]]['array'].append(p)
assert sum(reserved())==0,'accepted signing path left a stale paid HS reservation'
shot=alloc(158*4);assert call(exports['DriTestHsSnapshot'],t,context,shot)==1
assert ri(shot+22*4)==4,'secured HS joins actual projected count once'
portal,player=rr(109999,ovr=80,stage=0,schools=((10,300),),offers=[],portal=True)
assert call(exports['DriTestPortalDecision'],portal,t,context)!=18,'paid HS reservation was counted again after native publication'
assert call(exports['DriTestQualityFault'])==0
call(exports['DriTestOwnershipReset']);call(exports['DriTestHsPlanReset']);call(exports['DriTestHs29Reset'])
assert all(v==1 for v in refs.values())
checks=['Paid HS primary reserves exactly one quantity vacancy before signing',
 'Actual SIGN_BOARD callback invalidates cached reservations before native Signed/publication completes',
 'Same-advance snapshot counts secured HS exactly once and portal admission no longer sees its former reserved vacancy']
(OUTPUT/'test29_portal_class_boundary_results.json').write_text(json.dumps({'checks':checks,'native_publication':'modeled after actual compiled admission callback','live_game_verified':False},indent=2)+'\n')
print('\n'.join('PASS '+s for s in checks))
