"""R6 paired HS offers at NSD: no influence floor, no temporary-offer withdrawal."""
exec(compile((__import__('pathlib').Path(__file__).parent/'recovery_runtime_test.py').read_text(),'recovery_runtime_test.py','exec'))
records[season]['scalar'].update({4:6,5:6});reset_recovery()
a,ap=rr(62000,ovr=75,stage=3,schools=((10,30),(20,20)),offers=(10,20),portal=False)
b,bp=rr(62001,ovr=70,stage=3,schools=((10,29),(11,10)),offers=(10,11),portal=False)
for p in [ap,bp]:records[p]['scalar'][231]=1
fill(10,(65,64,63));fill(20,(65,64,63));fill(11,(65,64,63))
def winner(r,prune=0):
 value=call(exports['DriTestNsdWinner'],r,context,prune)
 if value:decref(value)
 assert call(exports['DriTestQualityFault'])==0
 return value
assert winner(a)==allteams[10]
assert winner(b,1)==allteams[11] and offer_status(b,10)
checks=['Primary wins its highest valid offer; backup uses next eligible destination without withdrawing the temporarily deferred offer']
# A third inherited offer waits for the earlier candidates' destinations;
# mere plan priority must not permanently destroy an otherwise valid offer.
c,cp=rr(62002,ovr=68,stage=3,schools=((10,25),),offers=(10,),portal=False)
records[cp]['scalar'][231]=1
assert winner(c,1)==0 and offer_status(c,10)
checks.append('An inherited third offer is deferred, not withdrawn, while the position still has a genuine opening')
# An offered school that cannot win the primary must not strand its backup.
top=records[a]['ref'][13]
records[records[top]['array'][0]]['scalar'][1]=10
records[records[top]['array'][1]]['scalar'][1]=40
# Direct fixture edits bypass the native event boundary that resets R8 matching.
if 'DriTestNsdReset' in exports:call(exports['DriTestNsdReset'])
assert winner(a)==allteams[20] and winner(b)==allteams[10]
checks.append('Primary has a stronger valid offer elsewhere: school immediately accepts its backup')
# Actual first commitment is counted before Team.CommittedPlayers publication.
records[records[top]['array'][0]]['scalar'][1]=50
records[a]['scalar'][9]=5
if 'DriTestNsdReset' in exports:call(exports['DriTestNsdReset'])
assert winner(b,1)==allteams[11] and not offer_status(b,10)
assert records[b]['scalar'][9]==3 and offer_status(a,10)
checks.append('Hard primary ownership closes its last quantity opening; native pruning withdraws only the now-invalid fallback offer')
call(exports['DriTestOwnershipReset'])
assert all(v==1 for v in refs.values()),[(records[r]['name'],v)for r,v in refs.items()if v!=1]
(OUTPUT/'test28r6_nsd_pairs_results.json').write_text(json.dumps({'checks':checks,'live_game_verified':False},indent=2)+'\n')
print(json.dumps(checks,indent=2))
