"""Independent native scorer call-boundary checks for R8.

Executes compiled helper; validates all seven Windows x64 arguments, signed
score writes, missing-dependency behavior and unchanged output canaries.
"""
exec(compile((__import__('pathlib').Path(__file__).parent/'portal_runtime_test.py').read_text(),'portal_runtime_test.py','exec'))
configure(1);call(exports['DriTestOwnershipReset'])
r,t,p=pr(63900,79,year=0,pos=3,stage=0,portal=False)
outblock=alloc(16);out=outblock+4
week_expected=8;return_score=455;corrupt=False;seen_calls=0

def checked_native_score():
 global seen_calls
 scorer,selection,portal,teamref=args();stack=reg('RSP')
 assert scorer==fixture_hs_scorer and selection==0 and portal==0
 assert teamref and rq(teamref)==team
 cache=rq(stack+0x28)
 if 'DriTestHs31ScoreInvalidate' in exports:
  assert cache and rq(cache)==cache+0x18 and rq(cache+0xa8)==cache+0xc0 and rq(cache+0x150)==cache+0x168,'Test31 passes the native inline school context'
 else:assert cache==0,'Legacy helper omitted the school cache'
 candidate=rq(stack+0x30)
 assert candidate and rq(candidate)==r
 assert ri(stack+0x38)==week_expected
 assert bytes(uc.mem_read(candidate+8,40))==bytes(40),'All ten candidate score fields start zeroed'
 wi(candidate+40,return_score)
 if corrupt:wq(candidate,0)
 seen_calls+=1
 return 0

game_api(0x08185840,checked_native_score)
checks=[]
for week_expected,return_score in [(0,0),(8,455),(63,-355),(8,2147483647),(8,-2147483648)]:
 records[season]['scalar'][4]=week_expected
 wi(outblock,0x13579bdf);wi(out,123);wi(out+4,0x2468ace0)
 before=dict(refs)
 assert call(exports['DriTestHsNativeScore'],r,team,context,out)==1
 assert struct.unpack('<i',uc.mem_read(out,4))[0]==return_score
 assert ri(outblock)==0x13579bdf and ri(out+4)==0x2468ace0
 assert refs==before
 checks.append({'week':week_expected,'score':return_score})
# A corrupt native candidate identity is rejected, preserving caller output.
corrupt=True;wi(out,123);records[season]['scalar'][4]=8;week_expected=8
assert call(exports['DriTestHsNativeScore'],r,team,context,out)==0 and ri(out)==123
corrupt=False
# Missing dependencies and out-of-range native calendar never enter scorer.
before_calls=seen_calls
for missing in ['scorer','season','calendar']:
 saved_scorer=rq(context+0xf8);saved_season=records[franchise]['ref'][44]
 if missing=='scorer':wq(context+0xf8,0)
 elif missing=='season':records[franchise]['ref'][44]=0
 else:records[season]['scalar'][4]=64
 wi(out,123)
 assert call(exports['DriTestHsNativeScore'],r,team,context,out)==0 and ri(out)==123
 wq(context+0xf8,saved_scorer);records[franchise]['ref'][44]=saved_season;records[season]['scalar'][4]=8
assert seen_calls==before_calls
if 'DriTestHs31ScoreInvalidate' in exports:
 initializer=handlers[GAME+0x08142630]
 call(exports['DriTestHs31ScoreInvalidate'],team)
 game_api(0x08142630,lambda:0) # malformed native cache must never fall back to NULL
 wi(out,123)
 assert call(exports['DriTestHsNativeScore'],r,team,context,out)==0 and ri(out)==123
 assert seen_calls==before_calls
 game_api(0x08142630,initializer)
assert all(v==1 for v in refs.values())
result={'cases':checks,'seven_argument_windows_abi':True,'canaries_and_reference_balance':True,'invalid_native_identity_preserves_output':True,'missing_dependencies_skip_native_call':True,'live_game_verified':False}
(OUTPUT/'test28r8_score_abi_results.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
