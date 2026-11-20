"""Compiled PT28 adapter with native hard-owner helper and mocked SDF services.
No game is launched. This verifies adapter boundaries, not a lifecycle outcome.
"""
from pathlib import Path
fixture=(Path(__file__).parent/'runtime_fixture_test.py').read_text()
exec(compile(fixture.split('A,TA=recruit')[0],'runtime_fixture_test.py','exec'))
checks=[]
def configure():call(exports['DriTestConfigure'],GAME,1,7)
configure()
committed=record('PT committed',array=[]);records[team]['ref'][33]=committed
departures=record('PT final departures',array=[])
leaving=record('PT leaving evaluator',ref={7:departures})
franchise_eval=record('PT franchise evaluator',ref={13:leaving})
teams=record('PT all teams',array=[team])
franchise=record('PT dynasty',ref={9:franchise_eval,44:season,45:teams})
records[season]['scalar'].update({2:2026,3:2,4:5,5:1,8:6})
records[tuning]['scalar'][39]=2
myctxrecord=record('PT MySchoolGradeEval',ref={2:franchise,5:tuning})
ctx=alloc(0x1600);wq(ctx+0x10,rq(myctxrecord+0x10));wi(ctx+0x18,ri(myctxrecord+0x18));wq(ctx+0x30,0xabcdef)
rectxrecord=record('PT RecruitingEval',ref={6:franchise})
rectx=alloc(0x300);wq(rectx+0x10,rq(rectxrecord+0x10));wi(rectx+0x18,ri(rectxrecord+0x18));wq(rectx+0x30,0xabcdef)
# Dynamic Scheduler tables have raw indices, unlike native franchise records.
raw_types={};clock_change_on_read=False;clock_read_count=0
RAWVT=alloc(0x200);uc.mem_write(RAWVT,bytes(uc.mem_read(TABLEVT,0x200)))
def raw_scalar_get(method):
 global clock_read_count
 table,rid,field,_=args();rec=tables[table][rid]
 assert field in raw_types[rec],('unexpected raw field',records[rec]['name'],field)
 assert raw_types[rec][field]==method,('wrong raw accessor',records[rec]['name'],field,hex(method))
 value=records[rec]['scalar'][field]
 if rec==scheduler and field==1:
  clock_read_count+=1
  if clock_change_on_read and clock_read_count==2:value+=1
 return value
def raw_reference_get():
 table,out_ref,rid,field=args();rec=tables[table][rid]
 value=records[rec]['ref'].get(field,0)
 incref(value);wq(out_ref,value);return out_ref
for method in (0xf8,0x100,0x108):wq(RAWVT+method,api(lambda method=method:raw_scalar_get(method)))
wq(RAWVT+0x128,api(raw_reference_get))
def raw_record(name,values,types,refs_=None):
 r=record(name,values,ref=refs_);raw_types[r]=types;wq(rq(r+0x10),RAWVT);return r
def event_record(name):
 r=record(name);event_schema=alloc(0x40);cstring=alloc(len(name)+1)
 uc.mem_write(cstring,name.encode()+b'\0');wq(event_schema+0x18,cstring)
 evvt=alloc(0x200);uc.mem_write(evvt,bytes(uc.mem_read(VT,0x200)))
 wq(evvt+0x38,api(lambda event_schema=event_schema:event_schema));wq(r,evvt);return r
uc.mem_map(GAME+0x8055d10,0x1000)
uc.mem_write(GAME+0x8055d60,raw[0x8055d60-0x1000:0x8055d60-0x1000+32])
stop_event=event_record('ProcessWeeklyGamesStartEvent');training_event=event_record('TrainingResultsStartEvent')
recurrence=raw_record('seven-day stop recurrence',{0:0,1:3,2:7,8:0},{0:0x108,1:0x108,2:0x100,8:0x108})
stop=raw_record('Advance Week',{4:22201202},{4:0x100},{5:stop_event,9:recurrence})
training_appointment=raw_record('Training Results',{2:1},{2:0xf8},{5:training_event})
appointment_array=record('scheduler appointments',array=[stop,training_appointment])
scheduler=raw_record('persisted Scheduler',{1:22201202},{1:0x100},{2:appointment_array})
season_manager=record('SeasonManager',ref={8:scheduler});records[franchise_eval]['ref'][20]=season_manager
for entry in ideal_entries:records[entry]['scalar'].update({0:0,2:0,4:0})
records[ideal_entries[0]]['scalar'].update({2:4,4:1})
records[roster]['array']=[];records[arr]['array']=[]
out=alloc(80)
def reset():call(exports['DriTestPt28Reset'])
def player(name,ovr,pos=0,year=1,tid=TEST_TEAM_ID,previous=TEST_TEAM_ID,redshirt=0):
 return record(name,{3:pos,191:ovr,230:previous,234:redshirt,242:year,261:tid})
def grade(p):
 assert call(exports['DriTestPt28Grade'],ctx,team,p,out)==1
 ints=struct.unpack('<11i',uc.mem_read(out,44));scores=struct.unpack('<3d',uc.mem_read(out+48,24))
 return dict(zip(['grade','rank','equals','starters','useful','rotation','peers','future_peers','ovr','future_ovr','has_future'],ints),immediate=scores[0],following=scores[1],score=scores[2])
def incoming(id,ovr,klass=4,stage=5,offer=3,pos=0):
 r,t=recruit(id,pos,1,250,stage=stage);p=records[r]['ref'][5]
 records[r]['scalar'].update({2:klass,9:stage})
 records[p]['scalar'].update({191:ovr,230:TEST_TEAM_ID if klass>=4 else 255,261:255,242:max(0,klass-4) if klass>=4 else 0})
 records[t]['scalar'][12]=offer
 return r,t,p
c=player('candidate',75);senior=player('graduating senior',99,year=3)
records[roster]['array']=[c,senior]
assert grade(c)['score']==12 and grade(c)['peers']==0
assert grade(c)['ovr']==77
checks.append('Own identity excluded; confirmed upcoming graduation excluded; one native development step')
# All board targets are not competition. Only secured destinations enter.
r,t,p=incoming(80001,95,stage=1);records[arr]['array']=[t]
records[franchise]['ref'][46]=record('active portal recruiting class',array=[r])
assert grade(c)['peers']==0
records[r]['scalar'][9]=5
assert grade(c)['peers']==1 and grade(c)['score']==4
records[t]['scalar'][12]=0
assert grade(c)['peers']==0
records[t]['scalar'][12]=3;records[r]['scalar'][9]=6
assert grade(c)['peers']==0
checks.append('Unsigned targets and orphan Signed records excluded; portal hard commit needs its exact live offer')
# Published owner overrides saved stages/leader, and duplicate identity counted once.
records[committed]['array']=[p,p];reset()
assert grade(c)['peers']==1
records[roster]['array']=[c,senior,p]
assert grade(c)['peers']==1
checks.append('Published acquisitions are authoritative and deduplicate across sources')
# Native hard ownership ledger survives board removal before native bulk publication.
records[committed]['array']=[];records[roster]['array']=[c];records[r]['scalar'][9]=5
records[arr]['array']=[t];reset();owner=alloc(4)
assert call(exports['DriTestHardOwner'],r,rectx,owner)==1 and ri(owner)==TEST_TEAM_ID
records[arr]['array']=[];records[r]['scalar'][9]=6
assert grade(c)['peers']==1
# An accepted destination overrides a stale leader on another school's board.
records[r]['scalar'][9]=5;records[arr]['array']=[t]
leader=records[records[r]['ref'][13]]['array'][0]
records[team]['scalar'][383]=3;records[leader]['scalar'][0]=3
assert grade(c)['peers']==0
records[team]['scalar'][383]=TEST_TEAM_ID;records[leader]['scalar'][0]=TEST_TEAM_ID
# A published owner wins over a stale pending ledger destination.
other=record('published rival',{383:3},ref={33:record('rival commits',array=[p])})
records[teams]['array']=[team,other];reset()
assert grade(c)['peers']==0
records[teams]['array']=[team];records[arr]['array']=[];records[r]['scalar'][9]=6
call(exports['DriTestOwnershipReset']);reset();assert grade(c)['peers']==0
checks.append('Accepted ledger survives board removal, defeats a stale rival leader, yields to published ownership, and clears with its scope')
# HS and JUCO entrants do not receive college class/development advancement.
h,ht,hp=incoming(80002,80,klass=0);records[arr]['array']=[ht]
assert grade(c)['score']==9
records[committed]['array']=[hp];records[h]['scalar'][9]=6;reset()
assert grade(c)['score']==9
assert grade(hp)['ovr']==80
checks.append('HS commitment and published HS entry produce the same projected rating and role')
# Native fine groups share both competitors and slots. Unrelated broad groups do not.
records[arr]['array']=[];records[committed]['array']=[];reset()
for pos in [5,9]:records[ideal_entries[pos]]['scalar'].update({2:3,4:1})
ot=player('LT candidate',75,pos=5);rt=player('RT leader',90,pos=9);guard=player('unrelated LG',99,pos=6)
records[roster]['array']=[ot,rt,guard]
g=grade(ot);assert g['starters']==2 and g['useful']==4 and g['peers']==1 and g['score']==12
records[ideal_entries[6]]['scalar'].update({2:3,4:1})
assert grade(ot)['starters']==2
checks.append('OT pairing uses LT+RT for both jobs and competition; LG neither steals a slot nor blocks OT')
# LS has a real independent group and a zero-job result, never a fabricated starter.
ls=player('LS',75,pos=21);records[roster]['array']=[ls]
assert grade(ls)['grade']==12 and grade(ls)['starters']==0
records[ideal_entries[21]]['scalar'].update({2:2,4:1})
assert grade(ls)['grade']==0 and grade(ls)['useful']==2
checks.append('LS zero-ideal yields F; a configured LS job yields A+ with correct useful depth')
# Position/OVR/config edits are reread without cache invalidation.
records[roster]['array']=[ot,rt]
records[rt]['scalar'][3]=5;records[ot]['scalar'][191]=50
assert grade(ot)['ovr']==52 and grade(ot)['peers']==1
records[ideal_entries[9]]['scalar'][4]=0
assert grade(ot)['starters']==1 and grade(ot)['score']<12
checks.append('Manual player and depth-config changes are read on every grade request')
# Active/preseason incumbents see this season. A new HS prospect sees his first
# incoming season, and his commitment does not consume a current-season job.
records[roster]['array']=[c,senior];records[arr]['array']=[ht];records[h]['scalar'][9]=5
for active_stage in [0,1]:
 records[season]['scalar'][3]=active_stage;reset()
 current=grade(c);entering=grade(hp)
 assert current['ovr']==75 and current['peers']==1 and current['future_peers']==1
 assert current['immediate']==4 and current['following']==9 and current['score']==5
 assert entering['ovr']==80 and entering['peers']==1 and entering['score']==12
 assert grade(senior)['score']==12 and not grade(senior)['has_future']
records[roster]['array']=[ot,rt];records[arr]['array']=[]
checks.append('Active/preseason incumbents use current jobs; HS targets use entering-season jobs and committed HS joins only the incumbent following-year room')
# At the native week-seven class boundary, returning juniors are now seniors.
# The older graduating senior is excluded by confirmed departure identity,
# not by blindly removing every player whose already-advanced class is Senior.
records[season]['scalar'].update({3:2,4:7,8:8})
records[c]['scalar'].update({191:78,242:3})
records[roster]['array']=[c,senior]
records[departures]['array']=[record('old graduating senior',{2:1,1:0},ref={4:senior})]
reset();trained=grade(c)
assert not trained['has_future']
assert trained['ovr']==78 and trained['peers']==0 and trained['score']==12
reset();assert grade(c)==trained
records[c]['scalar'].update({191:75,242:1})
records[season]['scalar'].update({4:5,8:6})
records[departures]['array']=[];records[roster]['array']=[ot,rt]
checks.append('Stable post-training class rollover keeps a returning senior, excludes the confirmed old graduate, uses actual trained rating once, and survives cache reset')
# The stage label precedes the real callbacks. Check the precise native guard
# with strict raw accessor types; no runtime cache or schema remap substitutes.
def checkpoint():return call(exports['DriTestPt28Checkpoint'],franchise,season)
records[season]['scalar'].update({3:2,4:6,8:7})
records[franchise_eval]['ref'].pop(20)
assert checkpoint()==1  # PLV/portal/NSD do not depend on this calendar guard.
records[season]['scalar'][8]=8
assert checkpoint()==0  # Stage changed before CurrentWeek/year advancement.
records[franchise_eval]['ref'][20]=season_manager
records[season]['scalar'][4]=7
records[training_appointment]['scalar'][2]=0
records[scheduler]['scalar'][1]=21596402
assert checkpoint()==0  # Old NSD stop time is insufficient before training.
records[scheduler]['scalar'][1]=21600003
assert checkpoint()==0  # Class advanced, training not started.
records[training_appointment]['scalar'][2]=1
assert checkpoint()==0  # Active write can precede the persisted training time.
records[scheduler]['scalar'][1]=21686402
assert checkpoint()==0  # Training callback completed, still advancing to stop.
records[scheduler]['scalar'][1]=22201202
assert checkpoint()==1
records[stop]['scalar'][4]+=604800
assert checkpoint()==1  # Stop recurrence may already point to next week.
clock_change_on_read=True;clock_read_count=0
assert checkpoint()==0
clock_change_on_read=False
records[appointment_array]['array'].append(training_appointment)
assert checkpoint()==0xffffffff
records[appointment_array]['array'].pop()
records[appointment_array]['array']=[stop]
assert checkpoint()==0xffffffff
records[appointment_array]['array']=[stop,training_appointment]
for field,bad in [(0,1),(1,4),(2,0),(2,8),(8,1)]:
 saved=records[recurrence]['scalar'][field];records[recurrence]['scalar'][field]=bad
 assert checkpoint()==0xffffffff
 records[recurrence]['scalar'][field]=saved
records[appointment_array]['array']=[stop]*513
assert checkpoint()==0xffffffff
records[appointment_array]['array']=[stop,training_appointment]
assert checkpoint()==1
records[season]['scalar'].update({4:5,8:6})
checks.append('Training guard rejects pre-start, pre-class, busy, clock-racing and malformed calendars; accepts only active training at either verified stop representation')
# Missing pre-PLV final departure data is valid; real confirmed departures remove peers.
records[season]['scalar'][3]=1;records[franchise_eval]['ref'].pop(13);reset()
assert grade(ot)['peers']==1
records[season]['scalar'].update({3:2,8:2});records[franchise_eval]['ref'][13]=leaving
records[departures]['array']=[record('confirmed NFL departure',{2:2,1:0},ref={4:rt})];reset()
assert grade(ot)['peers']==0
records[records[departures]['array'][0]]['scalar'][1]=1
assert grade(ot)['peers']==1
records[records[departures]['array'][0]]['scalar'][1]=0
assert grade(ot)['peers']==0
checks.append('No false fault before PLV evaluator exists; confirmed NFL departure excluded and same-week persuasion immediately restored')
# Execute native general-grade routing and both direct recruiting grade call sites
# into the real Test28 assembly hook. Only SDF/Windows services are fixtures.
def native_region(rva,length):
 first=rva&~4095;last=(rva+length+4095)&~4095
 for page in range(first,last,4096):
  if not any(lo<=GAME+page<=hi for lo,hi,_ in uc.mem_regions()):uc.mem_map(GAME+page,4096)
 uc.mem_write(GAME+rva,raw[rva-0x1000:rva-0x1000+length])
native_region(0x8406da0,0x1000);native_region(0x8412da0,0x1000)
native_region(0x8409da0,0x1000);native_region(0x8159d10,0x1000);native_region(0x817ed10,0x1000)
addresses=alloc(24);call(exports['DriTestPt28Addresses'],addresses)
native_pt,hook_pt,wrapper_pt=struct.unpack('<3Q',uc.mem_read(addresses,24))
uc.mem_write(GAME+0x840a8f0,b'\xff\x25\0\0\0\0'+struct.pack('<Q',hook_pt)+b'\x90\x90')
records[season]['scalar'][24]=1;wq(TABLEVT+0xf8,api(native_scalar))
tracking=record('grade tracking');records[team]['ref'][165]=tracking
pref=alloc(8);wq(pref,ot);expected=grade(ot)['grade']
assert call(exports['DriTestEpsGrade'],ctx,team_ref,13,pref)==expected
fakecaller=alloc(0xb0);wq(fakecaller+0xa0,ctx);native_bp=STACK+0xa000
wq(native_bp+0x60,fakecaller);wq(native_bp+0x68,pref);wq(native_bp+0x70,team_ref)
wq(native_bp+0x67,fakecaller);wq(native_bp-0x51,ot);wq(native_bp+0x77,team_ref)
def snippet(start,end):
 uc.reg_write(UC_X86_REG_RBP,native_bp);uc.reg_write(UC_X86_REG_RSP,STACK+0xd0000)
 uc.emu_start(GAME+start,GAME+end,count=2000000)
 assert reg('RIP')==GAME+end and (reg('RAX')&0xffffffff)==expected
# Overlapping native frame offsets require setting each site's frame separately.
wq(native_bp+0x60,fakecaller);wq(native_bp+0x68,pref);wq(native_bp+0x70,team_ref)
snippet(0x817f330,0x817f348)
wq(native_bp+0x67,fakecaller);wq(native_bp-0x51,ot);wq(native_bp+0x77,team_ref)
snippet(0x815a601,0x815a621)
checks.append('Actual native kind-13 dispatcher and both direct recruiting call sites reach the shared PT28 hook')
# Disabled, diagnostic, and pre-league paths execute the original trampoline.
# Substitute only the body after its verified 16-byte original prologue.
uc.mem_write(GAME+0x840a900,bytes.fromhex('b807000000415f415e415c5f5e5b5dc3'))
for wanted in [0,-1]:
 call(exports['DriTestConfigure'],GAME,wanted,7)
 assert call(exports['DriTestPt28Hook'],ctx,team_ref,pref)==7
configure();records[season]['scalar'][24]=0
assert call(exports['DriTestPt28Hook'],ctx,team_ref,pref)==7
records[season]['scalar'][24]=1
assert call(exports['DriTestPt28Hook'],ctx,team_ref,pref)==expected
records[season]['scalar'].update({4:7,8:8})
records[training_appointment]['scalar'][2]=0
assert call(exports['DriTestPt28Hook'],ctx,team_ref,pref)==7
assert call(exports['DriTestQualityFault'])==0
records[training_appointment]['scalar'][2]=1
records[scheduler]['scalar'][1]=21686402
assert call(exports['DriTestPt28Hook'],ctx,team_ref,pref)==7
assert call(exports['DriTestQualityFault'])==0
records[scheduler]['scalar'][1]=22201202
assert call(exports['DriTestPt28Hook'],ctx,team_ref,pref)==grade(ot)['grade']
records[recurrence]['scalar'][2]=8
assert call(exports['DriTestPt28Hook'],ctx,team_ref,pref)==7
assert call(exports['DriTestQualityFault'])==1
records[recurrence]['scalar'][2]=7;configure()
checks.append('Diagnostic, disabled, unstarted and valid training-transient paths use native fallback; malformed active calendar disables behavior; stable active mode uses custom PT')
reset();call(exports['DriTestOwnershipReset'])
assert all(n==1 for n in refs.values()),[(records[r]['name'],n) for r,n in refs.items() if n!=1]
report={'checks':checks,'referenceCountsBalanced':True,'compiled_callback':True,'live_game_verified':False}
(OUTPUT/'pt28_runtime_results.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
