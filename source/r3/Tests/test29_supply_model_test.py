#!/usr/bin/env python3
"""Multi-cohort supply sensitivity; analytically modeled, never live outcomes."""
import hashlib, json, math, os, pathlib
ROOT=pathlib.Path(__file__).resolve().parents[4]
AUDIT=ROOT/'Runtime/Portal/Audit/Test29'
manifest=json.loads((AUDIT/'generator_changes.json').read_text())
evidence=json.loads((AUDIT/'Inputs/punter-cohort-evidence.json').read_text())
assert evidence['user_school']==67 and evidence['NSD_old_player_years']=={'Freshman':57,'Junior':60,'Sophomore':51}
assert evidence['NSD_projected_CPU_P']==135+33+65==233

def binomial_below(n,p,threshold):
    # Exact iid binomial CDF recurrence, stable for this narrowly scoped model.
    term=math.exp(n*math.log1p(-p));answer=term
    for k in range(1,threshold):
        term*=((n-k+1)/k)*(p/(1-p));answer+=term
    return min(1.,answer)

routes=[]
for r in manifest['routes']:
    src=ROOT/r['baseline_path'];dst=ROOT/r['path']
    assert hashlib.sha256(src.read_bytes()).hexdigest()==r['before_sha256']
    # Names Test01 changes only naming tables. Exact approved overlay identity
    # retains the original R4 generator provenance and generation model.
    identity=json.loads((ROOT/'Runtime/Portal/Audit/TU1029/ftc_identity.json').read_text())['files'][r['path']]
    assert identity['R4_sha256']==r['after_sha256']
    assert hashlib.sha256(dst.read_bytes()).hexdigest()==identity['sha256']
    b,a=r['before_model'],r['after_model']
    assert abs(sum(a['positions'].values())-3200)<1e-8
    assert abs(a['positions']['P']-95)<.15
    for pos in b['positions']:
        if pos not in ['P','WR','CB']:
            assert max(abs(x-y) for x,y in zip(a['joint'][pos],b['joint'][pos]))<1e-9
    assert all(abs(x-y)<.01 for x,y in zip(a['stars'],b['stars']))
    assert a['joint']['P'][0]==a['joint']['P'][3]==a['joint']['P'][4]==0
    for pos in ['WR','CB']:
        assert max(abs(a['joint'][pos][i]-b['joint'][pos][i]) for i in [3,4])<.003
    bm,am=b['positions']['P'],a['positions']['P']
    rows=[]
    # Initial retained inventory is measured. Later old cohorts age out under
    # a four-year, no-new-attrition sensitivity scenario. New-cohort counts are
    # potential supply, not forced signings or native roster occupants.
    for years,old in [(1,168),(2,108),(3,57),(4,0),(5,0),(8,0)]:
        active=min(years,4);need=274-old
        row={'year':years,'old_roster_punters_available_assumption':old,'active_new_cohorts':active,
             'baseline_expected_available':old+active*bm,'test29_expected_available':old+active*am,
             'baseline_probability_supply_below274':binomial_below(3200*active,bm/3200,need),
             'test29_probability_supply_below274':binomial_below(3200*active,am/3200,need)}
        assert row['test29_probability_supply_below274']<row['baseline_probability_supply_below274']
        rows.append(row)
    sensitivity=[]
    for years in [4,5]:
        for usable in [1.,.9,.8,.7]:
            sensitivity.append({'roster_lifetime_years':years,'assumed_usable_fraction':usable,
             'baseline_expected_usable_supply':years*bm*usable,'test29_expected_usable_supply':years*am*usable,
             'baseline_probability_below274':binomial_below(3200*years,bm/3200*usable,274),
             'test29_probability_below274':binomial_below(3200*years,am/3200*usable,274)})
    routes.append({'route':r['route'],'baseline_expected_P':bm,'test29_expected_P':am,
     'steady_four_year_annual_replacement':274/4,
     'annual_probability_below69_baseline':binomial_below(3200,bm/3200,69),
     'annual_probability_below69_test29':binomial_below(3200,am/3200,69),
     'minimum_usable_fraction_for_four_year_balance_baseline':274/(4*bm),
     'minimum_usable_fraction_for_four_year_balance_test29':274/(4*am),
     'cohorts':rows,'realization_sensitivity':sensitivity})

result={'release':'Test29','status':'PASS_MODELED_SUPPLY','live_game_verified':False,
 'model':'Independent fixed3200 weighted draws; recipe expectations reparsed from delivered bytes. Binomial CDFs are modeled supply only.',
 'initial_deficit':{'measured_W0_CPU_P':199,'ideal_seats':274,'initial_deficit':75,'measured_NSD_CPU_P':233,'measured_remaining_net_deficit_even_if_both_unsigned_transfers_sign':39},
 'decision':'P target75→95 supports a larger competitive pool and reduces recurring replacement scarcity; a permanent+40 was not inferred from the one-year catch-up deficit. WR/CB nonelite supply funds the change. C/LG/ROLB generation remains at its accepted baseline.',
 'routes':routes,
 'limitations':['These are independent probability models, not generated game cohorts or live signing results.','The observed R14 cohort is one sample; its currently generated recruits are unchanged.','An initial deficit can persist into the first Test29 cohort. Two-cohort supply sufficiency is not guaranteed school-level allocation.','Actual eligibility, offers, school geography, redshirts, transfers, JUCO remaining years, roster limits and recruit choices can reduce useful realization.','No model assigns a recruit, bypasses a budget or forces the available pool to sign. More generated P are opportunities; room policies must avoid stacking.','The five routes are alternative asset layouts, not five independent live tests.']}
out=pathlib.Path(os.environ.get('DRI_TEST_OUTPUT_DIR',str(AUDIT)))/'supply-cohort-model.json'
out.parent.mkdir(parents=True,exist_ok=True);out.write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({'status':result['status'],'routes':len(routes),'initial_deficit':result['initial_deficit'],'annual_under69':[{k:r[k] for k in ['route','annual_probability_below69_baseline','annual_probability_below69_test29']} for r in routes]}))
