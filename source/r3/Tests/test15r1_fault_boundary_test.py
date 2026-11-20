"""A processing failure must never become a recruiting policy rejection.

Execute the compiled Windows callbacks against fixture SDF records and the
captured native board-removal function. The original Test15 DLL is an explicit
negative control. These are offline callback regressions, not a live save run.
"""
exec(compile((__import__('pathlib').Path(__file__).parent/'test15_acceptance_test.py').read_text(),
             'test15_acceptance_test.py','exec'))
import copy,subprocess

fault_checks=[]
fault_cases=[]
owner_out=alloc(4)

def business_state():
    return copy.deepcopy(records)

def assert_balanced():
    call(exports['DriTestOwnershipReset'])
    assert all(v==1 for v in refs.values()),[(records[r]['name'],v) for r,v in refs.items() if v!=1]

def assert_no_owner(rec):
    wi(owner_out,-1)
    assert call(exports['DriTestPendingOwner'],rec,records[rec]['ref'][5],owner_out)==0
    assert ri(owner_out)==0xffffffff

def bad_returner():
    # Year=4 is malformed required eligibility data in both builds. Unlike
    # unset prospect stars on a returner, it must still disable processing.
    p=player('Fault boundary malformed eligibility',18,ovr=76,year=4)
    records[roster]['array']=[p]
    return p

def portal_start():
    wq(frame+3*8,team_ref);wq(frame+10*8,bp)
    wq(bp+0x28,team_ref);wq(bp+0x58,season)
    wq(sp+0x58,context);uc.mem_write(sp+0x42,b'\1')
    call(start,frame)

def board_failure(negative=False):
    reset();first,_=prospect();second,_=prospect(pos=3)
    bad_returner()
    before=business_state();board_before=list(records[arr]['array'])
    event_at=len(native_events)
    # Start uses this saved frame; set it before taking the frame snapshot.
    wq(frame+3*8,team_ref);wq(frame+10*8,bp)
    wq(bp+0x28,team_ref);wq(bp+0x58,season)
    wq(sp+0x58,context);uc.mem_write(sp+0x42,b'\1')
    frame_before=bytes(uc.mem_read(frame,128))
    call(start,frame)
    assert call(exports['DriTestQualityFault'])==1
    assert bytes(uc.mem_read(frame,128))==frame_before
    if negative:
        assert records[arr]['array']==board_before[1:], 'Original Test15 did not reproduce the first-target removal'
        assert native_events[event_at:], 'Expected captured native removal was not executed'
        # Native removal also updates offer accounting; reproducing that
        # mutation is the negative control, not an expectation of safety.
    else:
        assert records==before, 'A failed helper mutated the native board/offer/player data'
        assert native_events[event_at:]==[], 'A failed helper entered native board removal'
        assert_no_owner(first);assert_no_owner(second)
    assert_balanced()
    return {'fault':True,'native_board_removed':negative,'first_candidate':ri(first+0x18),
            'remaining_board_size':len(records[arr]['array'])}

if os.environ.get('DRI_TEST15_FAULT_NEGATIVE')=='1':
    negative_result=board_failure(negative=True)
    (OUTPUT/'test15r1_fault_boundary_negative_results.json').write_text(json.dumps(negative_result,indent=2)+'\n')
    print('NEGATIVE CONTROL: original Test15 removes the first target after a real decision failure')
    raise SystemExit(0)

baseline=os.environ.get('DRI_TEST15_BASELINE_DLL')
assert baseline and Path(baseline).is_file(), 'Supply the original Test15 fixture DLL as DRI_TEST15_BASELINE_DLL'
negative_env=dict(os.environ,DRI_TEST15_FAULT_NEGATIVE='1',DRI_TEST_OUTPUT_DIR=str(Path(baseline).parent))
negative_run=subprocess.run([sys.executable,str(Path(__file__).resolve())],env=negative_env,
                            capture_output=True,text=True)
(OUTPUT/'test15r1_fault_boundary_negative.log').write_text(negative_run.stdout+negative_run.stderr)
assert negative_run.returncode==0, negative_run.stdout[-2000:]+negative_run.stderr[-2000:]
negative_result=json.loads((Path(baseline).parent/'test15r1_fault_boundary_negative_results.json').read_text())
assert negative_result['native_board_removed'] is True
fixed_result=board_failure()
fault_checks.append('The original compiled Test15 callback reproduces the first-target removal after a real helper fault; the repaired callback preserves the entire board and never enters native removal')

def prepare_accept_frame(route,rec):
    # Bulk is a native unaligned frame: RBP=RSP+0x79. Reproducing that
    # relationship avoids aliasing the recruit pointer with saved registers.
    use_bp=accept_sp+(0x79 if route=='bulk' else 0x200)
    uc.mem_write(accept_frame,b'\0'*128)
    wq(accept_frame+10*8,use_bp)
    wq(accept_frame,team);wq(accept_frame+11*8,team)
    wq(accept_frame+3*8,team);wq(accept_frame+2*8,context)
    if route in ['hs_weekly_hard','hs_weekly_signed','portal_weekly']:
        wq(accept_sp+0x38,transition);wq(use_bp+0x68,rec);wq(use_bp+0x60,context)
    elif route=='instant':
        wq(use_bp-0x41,rec);wq(use_bp+0x77,destination_ref);wq(use_bp+0x67,context)
    elif route=='bulk':wq(use_bp-0x19,rec)
    else:
        wq(accept_sp+0x30,rec);wq(use_bp+(0x57 if route=='board' else 0x40),context)
    records[transition]['scalar'][1]=6 if route=='hs_weekly_signed' else 5
    wq(accept_frame+14*8,0x12340001)

def invoke_accept(route):
    if route=='portal_weekly':
        call(weekly_cb,accept_frame)
        return int(not (rq(accept_frame+14*8)&255))
    return call({'hs_weekly_hard':hs_cb,'hs_weekly_signed':hs_cb,'instant':instant_cb,
                 'board':sign_cb,'deadline':deadline_cb,'bulk':bulk_guard_cb}[route],accept_frame)

for portal,routes in [(False,hs_routes+(['bulk'] if test26 else [])),(True,portal_routes+(['bulk'] if test26 else []))]:
    for route in routes:
        for failure in ['required_roster_data','candidate_rating']:
            reset();rec,p=prospect(portal=portal)
            if failure=='required_roster_data':bad_returner()
            else:records[p]['scalar'][191]=100
            prepare_accept_frame(route,rec)
            before=business_state();frame_before=bytes(uc.mem_read(accept_frame,128))
            event_at=len(native_events)
            assert invoke_accept(route)==0,(portal,route,failure,'fault became a native veto')
            assert call(exports['DriTestQualityFault'])==1,(portal,route,failure,'fault not injected')
            assert records==before,(portal,route,failure,'native records changed')
            assert bytes(uc.mem_read(accept_frame,128))==frame_before,(portal,route,failure,'frame changed')
            assert native_events[event_at:]==[]
            assert_no_owner(rec)
            # Disabled means subsequent hooks also leave the native attempt
            # alone, including the separate bulk ownership observer.
            assert invoke_accept(route)==0
            call(bulk_owner_cb,accept_frame)
            assert_no_owner(rec);assert records==before
            assert_balanced()
            fault_cases.append({'portal':portal,'route':route,'failure':failure})
fault_checks.append('All six HS and five portal acceptance paths pass native control through after invalid required roster data or candidate rating; stages, offers, players, frames and pending ownership remain unchanged')

# Malformed active-offer data is a processing error, not an absent offer.
for route in ['portal_weekly','instant','bulk']:
    reset();rec,p=prospect(portal=True)
    records[arr]['array'].append(record('Duplicate active offer',scalar={12:3},ref={10:rec}))
    prepare_accept_frame(route,rec)
    before=business_state();frame_before=bytes(uc.mem_read(accept_frame,128))
    assert invoke_accept(route)==0,(route,'offer decode error became veto')
    assert call(exports['DriTestQualityFault'])==1
    assert records==before and bytes(uc.mem_read(accept_frame,128))==frame_before
    assert_no_owner(rec);assert_balanced()
    fault_cases.append({'portal':True,'route':route,'failure':'duplicate_offer'})
fault_checks.append('Weekly, instant and bulk active-offer decode failures preserve the native condition instead of becoming an offerless-policy rejection')

select_frame=STACK+0x42000;select_sp=select_frame+128;select_bp=select_sp+0x200
select_rec_ref=alloc(8);fallback_teams=alloc(8);wq(fallback_teams,team)
for portal in [False,True]:
    for route in ['select','fallback','admit']:
        reset();rec,p=prospect(portal=portal)
        if route=='admit':
            portal_start()  # Establish a valid session before injecting fault.
            assert call(exports['DriTestQualityFault'])==0
            wq(candidate,rec);wq(frame+11*8,candidate)
            used_frame=frame
        else:
            uc.mem_write(select_frame,b'\0'*128)
            wq(select_frame+10*8,select_bp);wq(select_frame+14*8,0x12340001)
            if route=='select':
                wq(select_rec_ref,rec);wq(select_bp+0x67,context)
                wq(select_bp+0x77,select_rec_ref);wq(select_bp-0x39,team)
            else:
                wq(select_frame+13*8,context);wq(select_frame+7*8,0)
                wq(select_sp+0x108,0);call(hooks[12],select_frame)
                wq(select_bp-0x69,fallback_teams);wq(select_bp-0x21,rec)
            used_frame=select_frame
        bad_returner()
        before=business_state();frame_before=bytes(uc.mem_read(used_frame,128));event_at=len(native_events)
        result=call({'select':hooks[11],'fallback':hooks[13],'admit':admit}[route],used_frame)
        if route!='select':assert result==0,(portal,route,'fault became veto')
        assert call(exports['DriTestQualityFault'])==1,(portal,route,'missing fault')
        assert records==before and bytes(uc.mem_read(used_frame,128))==frame_before
        assert native_events[event_at:]==[];assert_no_owner(rec);assert_balanced()
        fault_cases.append({'portal':portal,'route':route,'failure':'required_roster_data'})
fault_checks.append('Select, fallback and admission callbacks preserve native conditions and saved frames when the shared HS or portal decision fails')

# Valid policy decisions must still reject genuinely prohibited recruiting.
reset();first,_=prospect(stars=4,stage=5);second,_=prospect(stars=4)
board_before=list(records[arr]['array']);event_at=len(native_events)
portal_start()
assert call(exports['DriTestQualityFault'])==0
assert records[arr]['array']==board_before[:1] and native_events[event_at:]
assert_balanced()
for route in ['portal_weekly','instant','bulk']:
    reset();rec,p=prospect(portal=True)
    records[arr]['array']=[]  # A well-formed absence is a genuine policy veto.
    prepare_accept_frame(route,rec)
    assert invoke_accept(route)!=0,(route,'legitimate no-offer veto disappeared')
    assert call(exports['DriTestQualityFault'])==0
    assert_no_owner(rec);assert_balanced()
fault_checks.append('Genuine same-position five-star board removal and portal no-active-offer vetoes remain enforced without a processing fault')

result={'checks':fault_checks,'cases':fault_cases,'negative_control':negative_result,
        'repaired_board_result':fixed_result,'live_game_verified':False,
        'method':'compiled original and repaired Windows callbacks; injected malformed required input; captured native board removal; fixture SDF and Windows services'}
(OUTPUT/'test15r1_fault_boundary_results.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
