"""Regression for redirected NSD ownership, using compiled DLL callbacks.
The captured native Signed-state writer executes; SDF and Windows are fixtures.
The six actual Test03a false-admission cases are replayed as room snapshots,
not as predicted Test04 destinations or a counterfactual season simulation.
"""
exec(compile((__import__('pathlib').Path(__file__).parent/'quality_runtime_test.py').read_text(),'quality_runtime_test.py','exec'))
configure(1)
audit_path=ROOT.parent/'Audit/signing_ownership_cases.json'
audit=json.loads(audit_path.read_text())
POS='QB HB FB WR TE LT LG C RG RT LE RE DT LOLB MLB ROLB CB FS SS K P LS'.split()
checks=[]

# This is the exact native sequence after the destination vector append.
map_game(0x081b5c6a,0x50)
uc.mem_write(GAME+0x081b5c6a,raw[0x081b5c6a-0x1000:0x081b5ca2-0x1000])
game_api(0x081b5ca2,lambda:0)
native_bp=STACK+0x2b000
def native_signed(rec):
 wq(native_bp-0x70,0xabcdef);wq(native_bp-0x78,rec)
 uc.reg_write(UC_X86_REG_RBP,native_bp)
 call(GAME+0x081b5c6a)
 assert records[rec]['scalar'][9]==6

of=STACK+0x29000;osp=of+128;obp=osp+0x200;wq(of+10*8,obp)
def pending(rec,destination=team,board=False):
 wq(of+10*8,obp) # Rebuild this synthetic frame after another emulated native call uses its stack region.
 wq(osp+0x30,rec);wq(of,destination);wq(of+11*8,destination)
 wq(obp+0x40,context);wq(obp+0x57,context)
 return call(sign_cb if board else deadline_cb,of)
def quality(pos=0,destination=team):
 out=alloc(1584);assert call(exports['DriTestQualityRoom'],destination,context,pos,out)==1
 return struct.unpack('<394i',uc.mem_read(out,1576))
def listed(rec,tid):
 records[records[rec]['ref'][13]]['array']=[record('old list leader',{0:tid,1:100})]
 if tid==records[team]['scalar'][383] and records[rec]['scalar'].get(2,0) in range(4,8) and records[rec]['scalar'][9] in [4,5]:valid_offer(rec)
def reset(tid=TEST_TEAM_ID,prestige=8,pos=0,ideal=4,starters=1):
 call(exports['DriTestOwnershipReset']);configure(1)
 records[team]['scalar'].update({383:tid,384:prestige})
 records[teams]['array']=fixture_team_array()
 records[team]['ref'].pop(407,None)
 for a in [departures,hslist,tplist,committed,arr,roster]:records[a]['array']=[]
 records[ideal_entries[pos]]['scalar'].update({2:ideal,0:0,4:starters})


def isolate_archived_room(pos):
 # Archive files record one position only. Do not infer global83/84 flex from
 # unrelated ideals left by a previous case. Keep this room exact and use an
 # explicit total85 outside it; complete-team flex has its own Test15 fixtures.
 for p,e in enumerate(ideal_entries):
  if p!=pos:records[e]['scalar'][2]=ideals[p]
 other=3 if pos!=3 else 16
 total=sum(records[e]['scalar'][2] for e in ideal_entries)
 records[ideal_entries[other]]['scalar'][2]+=85-total
 assert sum(records[e]['scalar'][2] for e in ideal_entries)==85
 assert 0<=records[ideal_entries[other]]['scalar'][2]<=30

# A remembered permission alone is not a signing; a native Signed state is.
reset();records[roster]['array']=[pp('returning QB',74)]
signed,signed_t,signed_p=pr(6000,83,stage=4);listed(signed,1)
assert pending(signed)==0
assert quality()[:6]==(0,4,1,1,1,-1)
native_signed(signed)
q=quality();assert q[:6]==(0,4,1,2,1,83),q[:6]
assert signed_p not in records[committed]['array']
second,second_t,second_p=pr(6001,85)
assert decide(second)==2
checks.append('Pending destination is ignored until native Signed state; then counts immediately before Team.CommittedPlayers is flushed and blocks a near-equal second QB')

# Native own_commit still identifies the stale list leader; our roster does not.
oldteam=record('stale leader school',{383:1,384:8},ref={64:config,239:record('old roster',array=[pp('old QB',74,tid=1)]),33:record('old commits',array=[])})
wr=alloc(32);wq(wr,GAME+0x0bd2e980);wq(wr+8,signed);wq(wr+16,0xabcdef);wq(wr+24,1)
tr2=alloc(8);wq(tr2,oldteam)
assert call(GAME+0x0862d6d0,wr,tr2)==1
assert quality(destination=oldteam)[:6]==(0,4,1,1,1,-1)
checks.append('Captured original ownership helper reproduces the false old-leader ownership; corrected room has no ghost signing')

# Once persisted, ownership survives cache reset/reload without the ledger.
records[committed]['array']=[signed_p]
call(exports['DriTestOwnershipReset'])
assert quality()[:6]==q[:6]
assert quality(destination=oldteam)[:6]==(0,4,1,1,1,-1)
checks.append('Published committed-player list preserves correct destination and incoming-portal classification without any transient entries')

# Actual six observed false admissions, with original roster/OVR/quality bands.
cases=[]
for ci,event in enumerate(audit['cases']):
 pos=POS.index(event['position'])
 reset(event['tid'],event['prestige'],pos,event['ideal'],event['starters']);isolate_archived_room(pos)
 for pl in event['actual_before']:
  if not pl['incoming']:records[roster]['array'].append(pp(pl['name'],pl['ovr'],pos=pos,tid=event['tid']))
 for pi,pl in enumerate(x for x in event['actual_before'] if x['incoming']):
  rec,t,player=pr(6100+ci*10+pi,pl['ovr'],pos=pos,stage=4)
  listed(rec,(event['tid']+1)%138)
  assert pending(rec)==0,(event['team'],pl['name'])
  native_signed(rec)
 candidate,t,player=pr(6100+ci*10+9,event['ovr'],pos=pos)
 # Test24 removes the extra +2-per-position rotation exception. These
 # archived rooms retain their exact ownership and normal +1 cap.
 expected=event['actual_reason']
 if event['actual_count']-event['ideal']>=2:expected=8
 revisions=json.loads((ROOT/'Tests/Fixtures/test29r4_archive_quality_permissions.json').read_text())['cases']
 matches=[x for x in revisions if x['archive']=='signing_ownership_cases.json' and (x['team'],x['player'])==(event['team'],event['name'])]
 if matches:
  revision=matches[0];assert revision['rating']==event['ovr'] and revision['rank']<=revision['useful_depth'] and 4*event['ovr']>=revision['floor_q4']
  expected=revision['expected']
 got=decide(candidate);assert got==expected,(event['team'],got,expected)
 current=quality(pos)
 assert current[3]==event['actual_count']
 cases.append({'team':event['team'],'player':event['name'],'reason':got,'count':current[3]})
checks.append('All six Test03a rooms retain exact ownership/counts and apply documented R4 prestige-qualified depth permissions while retaining nonqualifying rejections')

# HS/JUCO final signings affect portal room size but are not portal additions.
reset();records[roster]['array']=[pp('returner',74)]
hs2,hs2_t,hs2_p=pr(6300,72,stage=4,portal=False);listed(hs2,1)
assert pending(hs2,board=True)==0;native_signed(hs2)
assert quality()[:6]==(0,4,1,2,2,-1)
checks.append('Existing board-signing callback tracks redirected HS signing as base roster depth, preserving HS recruiting behavior')

# Calendar changes discard stale transient identity. Actual roster still wins.
records[committed]['array']=[hs2_p]
records[season]['scalar'][4]+=1
assert quality()[:6]==(0,4,1,2,2,-1)
assert refs[hs2]==1
checks.append('Week transition releases transient recruit references; persisted ownership is retained')

# Loading another franchise in the same week must not reuse pending ownership.
reset();r,t,p=pr(6350,80,stage=4);listed(r,1);assert pending(r)==0;native_signed(r)
assert quality()[3:6]==(1,0,80)
franchise2=record('different franchise, same week',ref=dict(records[franchise]['ref']))
records[context_record]['ref'][6]=franchise2
assert quality()[3:6]==(0,0,-1)
assert refs[r]==1 and refs[franchise]==1
records[context_record]['ref'][6]=franchise
checks.append('Franchise identity change clears prior pending ownership even when calendar values match')

# Diagnostic/disabled callbacks leave native decisions untouched and do not
# create pending ownership. Their later Signed fixture has no published owner.
for mode_value in [0,-1]:
 reset();configure(mode_value)
 r,t,p=pr(6400+mode_value,80,stage=4);listed(r,1)
 assert pending(r)==0;native_signed(r)
 configure(1);assert quality()[3:6]==(0,0,-1)
checks.append('Diagnostic/disabled modes do not record prospective signings')

call(exports['DriTestOwnershipReset'])
assert call(exports['DriTestQualityFault'])==0
assert all(v==1 for v in refs.values()),[(records[r]['name'],v) for r,v in refs.items() if v!=1]
checks.append('All retained records released; no fixture faults')
(OUTPUT/'signing_ownership_results.json').write_text(json.dumps({'checks':checks,'actual_false_admission_cases':cases,'native_signed_state_writer':'0x081b5c6a–0x081b5ca2','live_game_verified':False},indent=2)+'\n')
print(json.dumps({'checks':checks,'actual_false_admission_cases':cases},indent=2))
