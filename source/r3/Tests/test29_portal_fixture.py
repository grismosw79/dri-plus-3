"""Definition-only reuse of captured SDF/native services; no legacy assertions
are suppressed or relabeled. Legacy regression tests remain separate checks."""
from pathlib import Path
_tests=Path(__file__).resolve().parent
_base=(_tests/'runtime_fixture_test.py').read_text()
exec(compile(_base.split('\nfill_hs_position(0,ideals[0])\n')[0],str(_tests/'runtime_fixture_test.py'),'exec'))
_portal=(_tests/'portal_runtime_test.py').read_text()
_portal=_portal[_portal.index('star_cb,weekly_cb,instant_cb,sign_cb,deadline_cb='):]
exec(compile(_portal.split('\nkeep=pp(')[0],str(_tests/'portal_runtime_test.py'),'exec'))
configure(1)
records[season]['scalar'].update({2:2026,4:3,5:6})
records[tuning]['scalar'][39]=2
for _a in [departures,roster,hslist,tplist,committed,arr]:records[_a]['array']=[]
for _p,_e in enumerate(ideal_entries):records[_e]['scalar'].update({2:ideals[_p],4:1,0:0})
records[team]['scalar'][384]=7
records[board]['scalar'].update({1:0,2:0,3:800})
