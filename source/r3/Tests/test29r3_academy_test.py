"""Test30 academy eligibility: four-star HS normal gates, no portal pursuit."""
from pathlib import Path
exec(compile((Path(__file__).parent/'portal_runtime_test.py').read_text().split("keep=pp('StClair'")[0],__file__,'exec'))
configure(1);checks=[]
r,t,p=pr(129700,70,0,stage=0,portal=False)
for tid in [0,7,55,2,67]:
 records[team]['scalar'][383]=tid
 for cls in range(4):
  records[r]['scalar'][2]=cls
  for star in range(5):
   records[p]['scalar'][231]=star
   assert call(exports['DriTest29R3HsBlock'],r,team)==int(tid in [0,7,55] and cls==0 and star==4)
records[team]['scalar'][383]=7;records[r]['scalar'][2]=0;records[p]['scalar'][231]=4
for stage in [5,6]:
 records[r]['scalar'][9]=stage
 assert call(exports['DriTest29R3HsBlock'],r,team)==0
records[r]['scalar'][9]=0
for tid in [0,7,55]:
 records[team]['scalar'][383]=tid;records[teams]['array']=fixture_team_array()
 for star in [4]:
  records[p]['scalar'][231]=star
  assert call(exports['DriTestHsRecoveryDestination'],r,team,context,0)==0
  assert call(exports['DriTestQualityFault'])==0
checks.append('Only five-star HS at all three academies blocked; four-star, ordinary schools, JUCO and secured commitments preserved')
# The lifted restriction reaches the real HS capacity decision, including an
# academy controlled by the user; it does not merely change a display flag.
for tid in [0,7,55]:
 records[team]['scalar'][383]=tid;records[teams]['array']=fixture_team_array()
 records[p]['scalar'][231]=3
 call(exports['DriTestOwnershipReset'])
 assert call(exports['DriTestHsDecision'],r,team,context)==0
 records[team]['ref'][407]=record('user academy coach')
 assert call(exports['DriTestHsDecision'],r,team,context)==0
 records[p]['scalar'][231]=4
 assert call(exports['DriTestHsDecision'],r,team,context)==20
 records[team]['ref'].pop(407)
checks.append('Four-star ordinary vacancy accepted at all academies; user academy still cannot bypass five-star prohibition')
r,t,p=pr(129701,90,1,stage=3,portal=True)
for tid in [0,7,55]:
 records[team]['scalar'][383]=tid;records[teams]['array']=fixture_team_array()
 for origin in [0,7,55,2,255]:
  records[p]['scalar'][230]=origin
  assert call(exports['DriTestPortalDecision'],r,team,context)==5
checks.append('All academy portal pursuits blocked, including their own entrants and cross-academy players')
assert call(exports['DriTestQualityFault'])==0
(OUTPUT/'test29r3_academy_results.json').write_text(json.dumps({'passed':True,'checks':checks,'live_game_verified':False},indent=2))
print(checks)
