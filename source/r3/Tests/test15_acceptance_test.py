"""Test15 acceptance regressions against the compiled Windows callbacks.

The captured native Signed-state writer runs against fixture SDF storage.
HardCommitted publication is an explicit fixture state change. These tests
verify sequential decisions within one advance, not live game outcomes.
"""
exec(compile((__import__('pathlib').Path(__file__).parent/'runtime_fixture_test.py').read_text(),
             'runtime_fixture_test.py','exec'))

star_cb,weekly_cb,instant_cb,sign_cb,deadline_cb=hooks[3:8]
hs_cb,bulk_owner_cb,bulk_guard_cb=hooks[16:19]
checks=[]
franchise=record('Test15 acceptance franchise')
context_record=record('Test15 acceptance context',ref={6:franchise})
context=alloc(0x300)
wq(context+0x10,rq(context_record+0x10));wi(context+0x18,ri(context_record+0x18))
wq(context+0x30,0xabcdef);wq(context+0xc8,manager);wq(context+0xd0,tuning)
wq(context+0xf8,fixture_hs_scorer)  # R8 native scoring service in this independent evaluator.
hslist=record('Test15 acceptance HS class',array=[])
tplist=record('Test15 acceptance portal class',array=[])
committed=record('Test15 acceptance published commits',array=[])
departures=record('Test15 acceptance departures',array=[])
leaving=record('Test15 acceptance leaving evaluator',ref={7:departures})
franchise_eval=record('Test15 acceptance franchise evaluator',ref={13:leaving})
dummy_teams=[]
for tid in range(138):
    empty_board=record('Test15 empty board '+str(tid),
                       ref={0:record('Test15 empty targets '+str(tid),array=[])})
    dummy_teams.append(record('Test15 empty team '+str(tid),{383:tid,384:5},ref={
        33:record('Test15 empty commits '+str(tid),array=[]),224:empty_board,
        239:record('Test15 empty roster '+str(tid),array=[]),64:config}))
teams=record('Test15 acceptance teams',array=dummy_teams.copy())
records[teams]['array'][TEST_TEAM_ID]=team
records[franchise]['ref'].update({9:franchise_eval,39:hslist,44:season,45:teams,46:tplist})
records[team]['ref'][33]=committed

accept_frame=STACK+0x38000;accept_sp=accept_frame+128;accept_bp=accept_sp+0x200
wq(accept_frame+10*8,accept_bp)
destination_ref=alloc(8);wq(destination_ref,team)
transition=record('Test15 acceptance transition',{1:5})
quality_out=alloc(4096)  # Prefix is stable; policy metadata can extend the tail.
snapshot_out=alloc(157*4)
next_rid=30000

# Use the same captured native state writer as the ownership regressions.
map_game(0x081b5c6a,0x50)
uc.mem_write(GAME+0x081b5c6a,raw[0x081b5c6a-0x1000:0x081b5ca2-0x1000])
game_api(0x081b5ca2,lambda:0)
native_signed_bp=STACK+0x3b000
def native_signed(rec):
    wq(native_signed_bp-0x70,0xabcdef);wq(native_signed_bp-0x78,rec)
    uc.reg_write(UC_X86_REG_RBP,native_signed_bp)
    call(GAME+0x081b5c6a)
    assert records[rec]['scalar'][9]==6

def reset():
    call(exports['DriTestOwnershipReset']);configure(1)
    records[team]['ref'].pop(407,None)
    records[team]['scalar'].update({383:TEST_TEAM_ID,384:8})
    records[season]['scalar'].update({2:2026,4:4,5:1})
    records[tuning]['scalar'][39]=0
    for entry,ideal in zip(ideal_entries,ideals):
        records[entry]['scalar'].update({0:0,2:ideal,4:1})
    for a in [roster,arr,committed,hslist,tplist,departures]:records[a]['array']=[]

def player(name,pos,ovr=65,stars=1,year=1):
    return record(name,{3:pos,191:ovr,231:stars,242:year,261:TEST_TEAM_ID})

def prospect(pos=9,ovr=75,stars=2,portal=False,stage=4):
    global next_rid
    next_rid+=1
    rec,target=recruit(next_rid,pos,1,250,stage=stage)
    p=records[rec]['ref'][5]
    records[p]['scalar'].update({191:ovr,231:stars,242:1 if portal else 0,261:255})
    records[rec]['scalar'].update({2:5 if portal else 0,14:1})
    records[tplist if portal else hslist]['array'].append(rec)
    records[arr]['array'].append(target)
    return rec,p

def room(pos):
    assert call(exports['DriTestQualityRoom'],team,context,pos,quality_out)==1
    return struct.unpack('<6i',uc.mem_read(quality_out,24))

def snapshot():
    assert call(exports['DriTestHsSnapshot'],team,context,snapshot_out)==1
    return struct.unpack('<157i',uc.mem_read(snapshot_out,157*4))

def attempt(route,rec):
    """Return the callback veto without allowing native code to publish it."""
    wq(accept_frame,team);wq(accept_frame+11*8,team)
    wq(accept_frame+3*8,team);wq(accept_frame+2*8,context)
    # These are different native stack frames; several verified offsets
    # overlap, so populate only the frame belonging to this callback.
    if route in ['hs_weekly_hard','hs_weekly_signed','portal_weekly']:
        wq(accept_sp+0x38,transition);wq(accept_bp+0x68,rec);wq(accept_bp+0x60,context)
    elif route=='instant':
        wq(accept_bp-0x41,rec);wq(accept_bp+0x77,destination_ref);wq(accept_bp+0x67,context)
    elif route=='bulk':wq(accept_bp-0x19,rec)
    else:
        wq(accept_sp+0x30,rec)
        wq(accept_bp+(0x57 if route=='board' else 0x40),context)
    records[transition]['scalar'][1]=6 if route=='hs_weekly_signed' else 5
    wq(accept_frame+14*8,1)
    original_frame=bytes(uc.mem_read(accept_frame,128))
    original_recruit=dict(records[rec]['scalar'])
    p=records[rec]['ref'][5];original_player=dict(records[p]['scalar'])
    if route=='portal_weekly':
        call(weekly_cb,accept_frame);result=int(not (rq(accept_frame+14*8)&255))
        expected_frame=bytearray(original_frame)
        if result:expected_frame[14*8]=0
        assert bytes(uc.mem_read(accept_frame,128))==bytes(expected_frame)
    else:
        cb={'hs_weekly_hard':hs_cb,'hs_weekly_signed':hs_cb,'instant':instant_cb,
            'board':sign_cb,'deadline':deadline_cb,'bulk':bulk_guard_cb}[route]
        result=call(cb,accept_frame)
        assert bytes(uc.mem_read(accept_frame,128))==original_frame,route
    assert records[rec]['scalar']==original_recruit,(route,'callback changed stage')
    assert records[p]['scalar']==original_player,(route,'callback changed player')
    assert call(exports['DriTestQualityFault'])==0,(route,'fixture fault')
    return result

def accept(route,rec):
    """Publish only after permission, matching the native sequential advance."""
    assert attempt(route,rec)==0,(route,'unexpected veto')
    if route=='bulk':call(bulk_owner_cb,accept_frame)
    if route in ['hs_weekly_hard','portal_weekly','instant']:
        records[rec]['scalar'][9]=5
    else:native_signed(rec)

test26=any(tag in (OUTPUT/'retention_fixture.dll').read_bytes() for tag in [b'DRI_PORTAL_TEST26',b'DRI_PORTAL_TEST27',b'DRI_PORTAL_TEST28'])
hs_routes=['hs_weekly_hard','hs_weekly_signed','instant','board','deadline']+([] if test26 else ['bulk'])
portal_routes=['portal_weekly','instant','board','deadline']+([] if test26 else ['bulk'])
if test26:
    for portal in [False,True]:
        reset();soft,_=prospect(portal=portal)
        assert attempt('bulk',soft)==11 and room(9)[3]==0
    checks.append('Test26 soft choices defer from bulk to final offered-school selection without consuming a roster place')

# A permitted native attempt may still fail to commit. Permission is not a
# reservation. Once native state changes, the very next candidate sees it.
for route in hs_routes:
    reset()
    first,fp=prospect(stars=4);second,sp15=prospect(stars=4)
    assert attempt(route,first)==0
    assert room(9)[3]==0
    assert snapshot()[110+9]==0
    assert attempt(route,second)==0,(route,'failed attempt reserved a five-star place')
    assert room(9)[3]==0
    accept(route,first)
    assert room(9)[3]==1 and fp not in records[committed]['array']
    assert snapshot()[110+9]==1
    assert attempt(route,second)!=0,(route,'second five-star admitted in same advance')
    records[committed]['array']=[fp]
    assert room(9)[3]==1,(route,'publication duplicated accepted player')
checks.append('Unsecured HS acceptance routes: pending permission consumes nothing; native hard/signed state immediately blocks a second same-position five-star; published ownership deduplicates')

# Distinct real vacancies permit multiple four-stars after the annual quota;
# the next acceptance is closed once those actual openings have been filled.
for route in hs_routes:
    reset();records[roster]['array']=[player('returning RT',9)]
    first,_=prospect(stars=3);second,_=prospect(stars=3);third,_=prospect(stars=3)
    accept(route,first);assert room(9)[3]==2
    accept(route,second);assert room(9)[3]==3
    assert attempt(route,third)!=0,(route,'third four-star has no distinct opening')
checks.append('Unsecured HS acceptance routes: two distinct RT vacancies can fill with four-stars despite normal annual limit one; a third is blocked when the room reaches ideal')

# Existing owned hard commitments must not reject their own finalization.
# They already occupy the projected room and five-star/85+ QB counts.
for route in hs_routes+(['bulk'] if test26 else []):
    reset();first,_=prospect(pos=0,ovr=88,stars=4,stage=5)
    assert room(0)[3]==1
    assert attempt(route,first)==0,(route,'owned hard commitment self-blocked')
checks.append('Owned five-star 88 OVR HS hard commitments finalize through every acceptance callback without self-blocking')
for route in portal_routes+(['bulk'] if test26 else []):
    reset();first,_=prospect(pos=0,ovr=88,stars=4,portal=True,stage=5)
    assert room(0)[3]==1
    assert attempt(route,first)==0,(route,'owned portal hard commitment self-blocked')
checks.append('Offer-backed owned 88 OVR portal hard commitments finalize through every portal acceptance callback without self-blocking')

# Both directions cross class boundaries: a secured HS QB blocks a new elite
# portal QB, and a secured portal QB blocks a new elite HS QB.
for route in portal_routes:
    reset();records[tuning]['scalar'][39]=2
    first,_=prospect(pos=0,ovr=85,stars=2)
    accept('board',first)
    second,_=prospect(pos=0,ovr=83,stars=3,portal=True)
    assert attempt(route,second)!=0,(route,'HS 85 QB failed to block projected portal 85 QB')
    records[team]['ref'][407]=record('Test15 portal user')
    assert attempt(route,second)==0,(route,'new CPU QB rule affected user')
for route in hs_routes:
    reset();records[tuning]['scalar'][39]=2
    first,_=prospect(pos=0,ovr=83,stars=3,portal=True)
    accept('instant',first)
    second,_=prospect(pos=0,ovr=85,stars=2)
    assert attempt(route,second)!=0,(route,'projected portal 85 QB failed to block HS 85 QB')
    records[team]['ref'][407]=record('Test15 HS user')
    assert attempt(route,second)==0,(route,'new CPU QB rule affected user')
checks.append('Every HS/portal acceptance path honors the shared 85+ projected QB guard across recruit classes and bypasses the new restriction for user teams')

# Same portal-only sequence, including the exact native Signed writer, closes
# the second elite QB opportunity before Team.CommittedPlayers publication.
for route in portal_routes:
    reset();records[tuning]['scalar'][39]=2
    first,_=prospect(pos=0,ovr=83,stars=3,portal=True)
    second,_=prospect(pos=0,ovr=83,stars=3,portal=True)
    assert attempt(route,first)==0 and attempt(route,second)==0
    assert room(0)[3]==0
    accept(route,first)
    assert room(0)[3]==1
    assert attempt(route,second)!=0,(route,'second projected elite portal QB admitted')
checks.append('Unsecured portal acceptance routes immediately count the first secured projected 85 QB; an uncompleted permission neither reserves nor blocks a roster place')

# Elite-bunching restrictions are CPU planning policy, not user restrictions.
for route in hs_routes:
    reset();first,_=prospect(stars=4,stage=5);second,_=prospect(stars=4)
    assert attempt(route,second)!=0
    records[team]['ref'][407]=record('Test15 five-star user')
    assert attempt(route,second)==0,(route,'five-star CPU policy affected user')
    # Deliberately omit the CPU-only projection source: a true early user
    # bypass must not inspect it or disable otherwise healthy behavior.
    saved_offense=records[config]['ref'].pop(5)
    assert attempt(route,second)==0,(route,'user bypass inspected CPU projection')
    records[config]['ref'][5]=saved_offense
checks.append('All HS callbacks bypass the new same-position five-star restriction for user recruiting')

# No invented total-85 ceiling: a genuine RT vacancy remains legal while
# another position already carries surplus players. Final roster cuts are
# outside this package. The accepted incoming recruit raises the total to 87.
for route,is_portal in [(route,False) for route in hs_routes]+[(route,True) for route in portal_routes]:
    reset()
    returners=[player('Test15 above85 '+str(pos)+'/'+str(j),pos)
               for pos in range(21) if pos!=9 for j in range(4)]
    returners += [player('Test15 surplus WR '+str(j),3) for j in range(6)]
    assert len(returners)==86
    records[roster]['array']=returners
    first,_=prospect(pos=9,ovr=75,stars=2,portal=is_portal)
    accept(route,first)
    assert room(9)[3]==1 and len(records[roster]['array'])==86
    assert snapshot()[155]==87,(route,'accepted total should exceed85')
checks.append('An existing projected total above85 does not block an authentic vacant-position signing through any HS or portal acceptance path')

call(exports['DriTestOwnershipReset'])
assert call(exports['DriTestQualityFault'])==0
assert all(v==1 for v in refs.values()),[(records[r]['name'],v) for r,v in refs.items() if v!=1]
checks.append('Callbacks preserve native stage/player data and saved register frames; all fixture record references balance')
result={'checks':checks,'method':'compiled Windows callbacks with mocked SDF/Windows services and captured native Signed writer',
        'live_game_verified':False,'hs_acceptance_routes':hs_routes,'portal_acceptance_routes':portal_routes}
(OUTPUT/'test15_acceptance_results.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
