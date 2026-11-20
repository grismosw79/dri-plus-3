"""Execute captured native destination-search loop with compiled Test02 hook.
Native school-eligibility predicate and SDF storage are fixtures; search order,
reference lifetimes, loop continuation and the DLL callback are real code.
"""
exec(compile((__import__('pathlib').Path(__file__).parent/'quality_runtime_test.py').read_text(),'quality_runtime_test.py','exec'))
configure(1);records[team]['scalar'][384]=9
records[departures]['array']=[];records[hslist]['array']=[];records[tplist]['array']=[];records[committed]['array']=[];records[arr]['array']=[]
records[roster]['array']=[pp('returning QB',74)]
first,first_t,first_p=pr(4000,83,stage=5)
# A near-equal QB duplicates the secured 83; a distinctly better 91 is now
# intentionally eligible while this room still has capacity (Test13).
target,target_t,target_p=pr(4001,85)
team2=record('second eligible school',{383:1,384:9},ref={64:config,239:record('second roster',array=[pp('second returning QB',74,tid=1)]),33:record('second commits',array=[]),224:record('second board',ref={0:record('second board array',array=[])})})
records[teams]['array']=fixture_team_array(team2);records[records[target]['ref'][13]]['array']=[record('leader',{0:TEST_TEAM_ID,1:255}),record('runner up',{0:1,1:230})]
wq(context+0xa8,franchise)
map_game(0x081acd10,0x500);uc.mem_write(GAME+0x081acd10,raw[0x081acd10-0x1000:0x081ad210-0x1000])
# Install only the destination-selection hook in the captured function.
offsets=[];at=0
while (at:=image.find(prefix,at))>=0:offsets.append(at);at+=1
idx=offsets[11];end=offsets[12]
ins=list(cs.disasm(image[idx:end],BASE+idx));jumps=[i for i in ins if i.mnemonic=='jmp' and 'rip' in i.op_str]
for j,i in enumerate(jumps):wq(i.address+i.size+i.operands[0].mem.disp,GAME+(0x081ad00e if j==0 else 0x081ad0a4))
uc.mem_write(GAME+0x081acfff,b'\xff\x25\0\0\0\0'+struct.pack('<Q',BASE+idx)+b'\x90')
visited=[];native_denied=set()
def eligibility():
 ev,tr,rr,_=args();assert ev==context and rq(rr)==target
 t=rq(tr);tid=records[t]['scalar'][383];visited.append(tid);return int(tid not in native_denied)
game_api(0x0816b600,eligibility)
outref=alloc(8);targetref=alloc(8);wq(targetref,target)
def search():
 visited.clear();wq(outref,0);call(GAME+0x081acd10,context,outref,targetref)
 result=rq(outref)
 if result:decref(result)
 return result
assert search()==team2 and visited==[TEST_TEAM_ID,1],(rq(outref),visited)
checks2=['Captured native loop skips roster-ineligible first school and returns the next eligible school in native order']
# Both schools already have an incoming starter: no forced destination.
second,second_t,second_p=pr(4002,88,stage=5)
records[records[second]['ref'][13]]['array']=[record('second commitment owner',{0:1,1:255})]
valid_offer(second,destination=team2)
assert search()==0 and visited==[TEST_TEAM_ID,1]
checks2.append('When every school fails the roster rule, native loop returns no destination')
records[second]['scalar'][9]=1;native_denied.add(1)
assert search()==0 and visited==[TEST_TEAM_ID,1]
checks2.append('Native eligibility rejection is respected; custom hook cannot make rejected school valid')
records[team]['ref'][407]=record('user destination');native_denied.clear()
assert search()==team and visited==[TEST_TEAM_ID]
checks2.append('User-controlled destination follows original native selection')
records[team]['ref'].pop(407)
call(exports['DriTestOwnershipReset'])
assert all(v==1 for v in refs.values()),[(records[r]['name'],v) for r,v in refs.items() if v!=1]
assert call(exports['DriTestQualityFault'])==0
checks2.append('All native temporary/returned references balanced, no runtime fault')
(OUTPUT/'destination_search_results.json').write_text(json.dumps({'checks':checks2,'live_game_verified':False,'mocked':['native eligibility predicate','SDF storage','Windows APIs']},indent=2)+'\n');print(json.dumps(checks2,indent=2))
