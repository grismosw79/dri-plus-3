"""Compiled Test28 router plus original native required-grade evaluator.
Tests Player/HS/JUCO/transfer discrimination, OVR curves and coach preservation.
No game process is started and no save is modified.
"""
from pathlib import Path
source=(Path(__file__).parent/'eps_runtime_test.py').read_text()
exec(compile(source.split('checks=[]')[0],'eps_runtime_test.py','exec'))
# Run the captured transfer discriminator as well as the captured evaluator.
handlers.pop(GAME+0x0862db70)
uc.mem_write(GAME+0x0862db70,raw[0x0862db70-0x1000:0x0862db70-0x1000+0x40])

def configure(m):call(exports['DriTestConfigure'],GAME,m,7)
def required(ref):return call(exports['DriTestEpsRequired'],eval_ptr,inforef,ref,teamref)
def original(ref):return call(native[2],eval_ptr,inforef,ref,teamref)
def curve(points,inactive=-100):
    values={i:inactive for i in range(13)}
    for threshold,grade in points:values[grade]=threshold
    return values

policies={
  2:[(0,10),(70,9),(75,7),(80,5),(85,4),(90,2)],
  4:[(0,10),(70,9),(75,7),(80,5),(85,4),(90,2)],
  7:[(0,8),(80,4),(85,2)],
  9:[(40,12),(45,11),(50,10),(55,9),(60,8),(65,7),(70,6),(75,5),(80,4),(85,3),(90,2),(95,1)],
 10:[(0,10),(70,9),(75,7),(82,5),(90,2)]
}
checks=[];cases=0;native_midpoint_differences=0
for motivation,points in policies.items():
    records[info]['scalar'][3]=motivation
    records[oldcurve]['scalar']=curve(points,-1 if motivation==9 else -100)
    for allowance in [0,1,2,4]:
        for ovr in range(100):
            records[player]['scalar'][191]=ovr
            expected=max([(t,g) for t,g in points if t<=ovr],default=(-1,12))[1]+allowance
            before=original(playerref)
            assert required(playerref)==expected,(motivation,ovr,allowance,required(playerref),expected,before)
            native_midpoint_differences+=before!=expected;cases+=1
            # Every OVR and coach allowance is evaluated through both schemas.
            records[recruit]['scalar'][2]=5;records[player]['scalar'][231]=3
            assert required(recruitref)==expected;cases+=1
        # Independently cross every class/star at every boundary and neighbor.
        boundary_ovrs=sorted({x for t,_ in points for x in [t-1,t,t+1] if 0<=x<100}|{0,99})
        for ovr in boundary_ovrs:
            records[player]['scalar'][191]=ovr
            expected=max([(t,g) for t,g in points if t<=ovr],default=(-1,12))[1]+allowance
            for cls in range(8):
                records[recruit]['scalar'][2]=cls
                for stars in range(5):
                    records[player]['scalar'][231]=stars
                    before=original(recruitref)
                    after=required(recruitref)
                    assert after==(expected if cls>=4 else before),(motivation,ovr,cls,stars,allowance,after,expected,before)
                    cases+=1
    checks.append(f'Motivation {motivation}: all OVRs use explicit floor bands; every boundary/class/star preserves native coach allowance; HS/JUCO unchanged')
assert native_midpoint_differences>0
checks.append(f'{native_midpoint_differences} native Player midpoint/sentinel results replaced by the agreed lower-bound bands')

# Requirements use current OVR, independent of transfer class year/stored stars.
records[info]['scalar'][3]=7;records[oldcurve]['scalar']=curve(policies[7])
records[player]['scalar'][191]=84
for cls in range(4,8):
    records[recruit]['scalar'][2]=cls
    for star in range(5):
        records[player]['scalar'][231]=star
        assert required(recruitref)==4+allowance
checks.append('84 OVR Conference Prestige remains B for every transfer class/star representation')
for star in [5,14,15,-1]:
    records[player]['scalar'][231]=star
    assert required(playerref)==4+allowance
records[player]['scalar'][231]=4
checks.append('Roster Player requirements ignore unset/irrelevant stored recruiting stars')

# Other categories keep their native requirement calculation (Style has its own EPS rule).
records[recruit]['scalar'][2]=6
for motivation in [0,1,3,5,6,11,12,13,15]:
    records[info]['scalar'][3]=motivation
    assert required(recruitref)==original(recruitref)
checks.append('Unchanged motivations preserve their native result')

records[info]['scalar'][3]=7
for wanted in [0,-1]:
    configure(wanted)
    assert required(recruitref)==original(recruitref)
configure(1)
checks.append('Diagnostic/disabled modes preserve native behavior')
call(exports['DriTestEpsReset'])
assert all(v==1 for v in refs.values()),[(records[r]['name'],v) for r,v in refs.items() if v!=1]
assert call(exports['DriTestQualityFault'])==0
checks.append('No leaked record references or runtime fault')
# A DLL/FTC mismatch must disable custom behavior without rewriting the row.
records[info]['scalar'][3]=7
records[oldcurve]['scalar']=curve(policies[7])
records[oldcurve]['scalar'][4]=79
wrong=dict(records[oldcurve]['scalar'])
before=original(playerref)
assert required(playerref)==before
assert call(exports['DriTestQualityFault'])==1
assert records[oldcurve]['scalar']==wrong
assert all(v==1 for v in refs.values())
configure(1)
records[oldcurve]['scalar']=curve(policies[7])
checks.append('Stale FTC curve disables custom behavior, preserves native fallback and leaves tuning unchanged')
(OUTPUT/'db28_requirements_runtime_results.json').write_text(json.dumps({'checks':checks,'nativeRequirementCases':cases,'nativeMidpointDifferences':native_midpoint_differences,'live_game_verified':False},indent=2))
print(json.dumps({'checks':checks,'nativeRequirementCases':cases},indent=2))
