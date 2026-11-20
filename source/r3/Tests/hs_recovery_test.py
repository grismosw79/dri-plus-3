"""Test15 earned HS/JUCO reopening through compiled callbacks/native lockout.
SDF storage, native dealbreaker verdicts and Windows services are fixtures.
This verifies policy mechanics, not a live-game recruiting outcome.
"""
exec(compile((__import__('pathlib').Path(__file__).parent/'late_recovery_test.py').read_text(),'late_recovery_test.py','exec'))
hschecks=[]

def hsreset():
 reset_recovery();denied.clear();native_gate_calls.clear()
 records[season]['scalar'].update({4:3,5:6})
 # A realistic all-position scheme total for the shared HS snapshot.
 ideals=[4,6,1,10,5,4,4,4,4,4,5,5,4,4,6,4,4,4,2,2,2,0]
 for p,v in enumerate(ideals):records[ideal_entries[p]]['scalar'].update({2:v,0:0,4:1})
 records[tuning]['scalar'][39]=0

def hsrec(rid,tid=96,points=23,stage=3,pos=0,ovr=78,offers=None,school_class=0):
 offers=(tid,) if offers is None else offers
 r,p=rr(rid,ovr=ovr,stage=stage,schools=((10,99),(11,97),(12,50)),offers=offers,portal=False)
 records[r]['scalar'][2]=school_class
 records[p]['scalar'].update({3:pos,231:2})
 for dest in offers:
  for t in records[board_targets(allteams[dest])]['array']:
   if records[t]['ref'].get(10)==r:records[t]['scalar'][8]=points
 return r,p

def preboard(tid):
 global next_heap
 before=next_heap;call(exports['DriTestHsRecoverTeam'],allteams[tid],context);after=next_heap
 if after>before:uc.mem_write(before,bytes(after-before))
 next_heap=before
 assert call(exports['DriTestQualityFault'])==0

def snapshot(r):
 return (values(r),dict(records[r]['scalar']),[(tid,dict(records[t]['scalar'])) for tid in range(138) for t in records[board_targets(allteams[tid])]['array'] if records[t]['ref'].get(10)==r])

# The original portal-market recovery must use the same projected acceptance
# policy as fresh signing callbacks, including raw-OVR exceptions and flex.
hsreset();records[tuning]['scalar'][39]=5
r,p=rr(6050,ovr=80,offers=(10,13));fill(10,ratings=(80,))
assert run_recovery()==0 and records[r]['scalar'][9]==0
assert values(r)[0]==(13,37) and not offer_status(r,10) and offer_status(r,13)
hschecks.append('Portal recovery rejects a second projected 85+ QB even when both current ratings are 80, reopening the actual remaining vacancy pursuit')
for current in (79,80):
 hsreset();records[tuning]['scalar'][39]=2;records[allteams[10]]['scalar'][384]=0
 r,p=rr(6051+current,ovr=current,offers=(10,13));records[p]['scalar'][3]=1
 records[records[allteams[10]]['ref'][33]]['array']=[pp('secured HS HB '+str(i),ovr,year=0,pos=1,tid=10) for i,ovr in enumerate((90,78,77,76,70,60))]
 assert run_recovery()==0
 assert records[r]['scalar'][9]==0 and values(r)[0]==(13,37) and not offer_status(r,10)
hschecks.append('Adequate full HB roles close for both 79 and 80 candidates; raw 80 no longer creates an extra rotation exception')
for ideal_total in (84,85):
 hsreset()
 for pos,e in enumerate(ideal_entries):records[e]['scalar'][2]=4 if pos<21 else 0
 if ideal_total==85:records[ideal_entries[3]]['scalar'][2]=5
 r,p=rr(6140+ideal_total,ovr=65,offers=(10,13));records[p]['scalar'][3]=1
 records[records[allteams[10]]['ref'][239]]['array']=[pp('flex HB '+str(i),ovr,pos=1,tid=10) for i,ovr in enumerate((95,90,85,60))]
 assert run_recovery()==0
 if ideal_total==84:assert records[r]['scalar'][9]==3 and offer_status(r,10)
 else:assert records[r]['scalar'][9]==0 and values(r)[0]==(13,37) and not offer_status(r,10)
hschecks.append('Portal market recovery preserves a useful full-room offer when the complete scheme has a flexible place, and removes the same blocked offer when no flexible place exists')

# Shared budget also binds market recovery's cached room path across positions.
hsreset();records[allteams[10]]['scalar'][384]=10
for pos,count in [(1,6),(6,4),(5,6)]:
 records[records[allteams[10]]['ref'][239]]['array'] += [pp('budget room '+str(pos)+'/'+str(i),60,pos=pos,tid=10) for i in range(count)]
r1,p1=rr(6190,ovr=83,offers=(10,13));records[p1]['scalar'][3]=1
r2,p2=rr(6191,ovr=83,offers=(10,13));records[p2]['scalar'][3]=6
assert run_recovery()==0
for rec in [r1,r2]:
 assert records[rec]['scalar'][9]==3 and offer_status(rec,10) and offer_status(rec,13)
hschecks.append('Inherited cross-position overages do not consume new portal exceptions in recovery')
# Four other meaningful full-room arrivals use 2 general + 2 emergency slots;
# the unspent RB-only slot cannot admit an LG.
for i,pos in enumerate([7,8,9,10]):
 ideal=records[ideal_entries[pos]]['scalar'][2]
 records[records[allteams[10]]['ref'][239]]['array'] += [pp('secured budget base',60,pos=pos,tid=10) for _ in range(ideal)]
 secured,sp=rr(6192+i,ovr=80,stage=5,schools=((10,200),),offers=(10,));records[sp]['scalar'][3]=pos
assert run_recovery()==0
assert records[r2]['scalar'][9]==0 and not offer_status(r2,10) and offer_status(r2,13)
assert records[r1]['scalar'][9]==3 and offer_status(r1,10) # RB slot remains available.
hschecks.append('Cached recovery enforces exhausted general/emergency slots across positions while preserving the dedicated RB allowance')

hsreset();r,p=hsrec(6100)
assert locked(r,96)==1
before_offer=records[r]['scalar'][14];before_score=records[r]['scalar'][3]
preboard(96)
assert values(r)[0]==(96,23) and records[r]['scalar'][9]==0 and locked(r,96)==0
assert records[r]['scalar'][14]==before_offer and records[r]['scalar'][3]==before_score and offer_status(r,96)
assert not native_events and records[records[allteams[96]]['ref'][33]]['array']==[]
prior=snapshot(r);preboard(96);late(r);assert snapshot(r)==prior
hschecks.append('Preboard CPU vacancy recovery exposes an existing 23-point offer before native action; no offer, points, ownership or commitment threshold is created; repeat is inert')

# HS and JUCO, multiple positions, and academies can use native legal offers.
for cls,pos,tid in [(0,1,96),(1,3,96),(2,10,96),(3,14,96),(0,19,0),(0,20,7),(0,0,55)]:
 hsreset();r,p=hsrec(6110+pos+cls,tid=tid,pos=pos,school_class=cls)
 late(r)
 assert records[r]['scalar'][9]==0 and values(r)[0]==(tid,23) and offer_status(r,tid)
hschecks.append('HS/JUCO class enums and HB/WR/DT/CB/K/P/QB vacancy routes work; academy exclusion remains portal-only')

# Ordinary annual allowance can keep a finalist viable at its position ideal.
for full in (False,True):
 hsreset();r,p=hsrec(6150+int(full),offers=(10,96))
 if full:fill(10,ratings=(80,78,74,70))
 prior=snapshot(r);preboard(96);late(r);assert snapshot(r)==prior
hschecks.append('Viable finalist is preserved even when its route is ordinary annual developmental recruiting rather than a vacancy exception')

# Users retain the original race whether listed or outside the finalist set.
for user_tid in (10,13):
 hsreset();r,p=hsrec(6160+user_tid,offers=(user_tid,96))
 records[allteams[user_tid]]['ref'][407]=record('HS user')
 prior=snapshot(r);preboard(96);late(r);assert snapshot(r)==prior
hsreset();r,p=hsrec(6175);records[allteams[96]]['ref'][407]=record('HS recruiting user')
prior=snapshot(r);preboard(96);assert snapshot(r)==prior
hschecks.append('Any existing user offer protects its HS race; user preboard callback is inert')

# Failed rescue attempts preserve paid activities and the original shortlist.
for case in ('full','no_offer','no_points','dealbreaker'):
 hsreset();r,p=hsrec(6180+len(case),points=0 if case=='no_points' else 23)
 if case=='full':fill(96,ratings=(80,78,74,70))
 if case=='no_offer':
  records[r]['scalar'][14]=0
  for t in records[board_targets(allteams[96])]['array']:records[t]['scalar'][12]=1
 if case=='dealbreaker':denied.add(96)
 prior=snapshot(r);preboard(96);late(r);assert snapshot(r)==prior
hschecks.append('No projected vacancy, no scholarship, zero earned influence and native dealbreaker rejection all preserve the original race without forced placement')

# Class protections bind recovery despite other remaining position vacancies.
for case in ('five_star','elite_qb'):
 hsreset();r,p=hsrec(6200+len(case),ovr=86 if case=='elite_qb' else 78)
 if case=='elite_qb':fill(96,ratings=(88,))
 else:
  records[p]['scalar'][231]=4
  secured,sp=hsrec(6220,tid=96,stage=5);records[sp]['scalar'][231]=4
  top=records[secured]['ref'][13];records[records[top]['array'][0]]['scalar'][0]=96
 prior=snapshot(r);preboard(96);late(r);assert snapshot(r)==prior
hschecks.append('Five-star class bunching and 85+ QB stacking safeguards also block recovery')

# Cache is callback-local, so later native signings close previous vacancies.
hsreset();records[ideal_entries[0]]['scalar'][2]=1
r,p=hsrec(6230);preboard(96);assert records[r]['scalar'][9]==0
second,sp=hsrec(6231);fill(96,ratings=(78,));prior=snapshot(second)
preboard(96);late(second);assert snapshot(second)==prior
hschecks.append('New callback recomputes vacancy after earlier signings rather than keeping a stale open roster slot')

hsreset();r,p=hsrec(6235)
records[records[allteams[96]]['ref'][239]]['array']=[pp('unbalanced roster '+str(i),65,pos=1,tid=96) for i in range(85)]
late(r);assert records[r]['scalar'][9]==0 and values(r)[0]==(96,23)
hschecks.append('An existing total of 85 does not suppress a genuine vacancy at another position; no new total-roster hard cap is introduced')

# Timing and exact native callsite isolation.
for typ,week in ((6,1),(6,7),(2,8),(3,0),(5,0)):
 hsreset();r,p=hsrec(6240+typ+week);records[season]['scalar'].update({4:week,5:typ})
 prior=snapshot(r);preboard(96);late(r);assert snapshot(r)==prior
for stage in (0,5,6):
 hsreset();r,p=hsrec(6260+stage,stage=stage);prior=snapshot(r)
 preboard(96);late(r);assert snapshot(r)==prior
hsreset();r,p=hsrec(6270);records[season]['scalar'].update({4:6,5:6})
ff=STACK+0x18000;fsp=ff+128;rrp=alloc(8);wq(rrp,r)
wq(ff+13*8,context);wq(ff+7*8,rrp);wq(ff+10*8,STACK+0x1a000)
wq(fsp+0x108,GAME+0x08103d10);call(hooks[12],ff);assert records[r]['scalar'][9]==3
out26=alloc(8);wq(ff+12*8,out26);before26=values(r)
wq(fsp+0x108,GAME+0x081b4602);assert call(hooks[12],ff)==1
assert rq(out26)==allteams[96] and records[r]['scalar'][9]==3 and values(r)==before26
decref(rq(out26));wq(out26,0)
hschecks.append('Only native legal recruiting windows and unsigned closed races recover; verified HS NSD fallback return works while unrelated callers remain inert')

# Diagnostic/disabled modes and unknown recruit classes never change a race.
for wanted in (0,-1):
 hsreset();r,p=hsrec(6280-wanted);configure(wanted);prior=snapshot(r)
 preboard(96);late(r);assert snapshot(r)==prior
hsreset();r,p=hsrec(6285,school_class=8);prior=snapshot(r)
preboard(96);late(r);assert snapshot(r)==prior
hschecks.append('Diagnostic/disabled modes and unrecognized recruit classes remain untouched')

# Malformed source records fail before the first shortlist/stage mutation.
for issue in ('duplicate_interest','invalid_points'):
 hsreset();r,p=hsrec(6290+len(issue))
 entries=records[records[r]['ref'][13]]['array']
 if issue=='duplicate_interest':records[entries[1]]['scalar'][0]=10
 else:records[entries[0]]['scalar'][1]=65536
 prior=snapshot(r)
 call(exports['DriTestDeadlineRecovery'],r,context)
 assert call(exports['DriTestQualityFault'])==1 and snapshot(r)==prior
hsreset()
hschecks.append('Duplicate interest identities and out-of-range influence fail closed before any race mutation')

call(exports['DriTestOwnershipReset']);assert all(v==1 for v in refs.values())
assert call(exports['DriTestQualityFault'])==0
hschecks.append('All held SDF references balance and compiled runtime remains fault-free')
(OUTPUT/'hs_recovery_results.json').write_text(json.dumps({'checks':hschecks,'native_gate_calls':native_gate_calls,'mocked':['native dealbreaker predicate','SDF storage','Windows APIs'],'live_game_verified':False},indent=2)+'\n')
print(json.dumps(hschecks,indent=2))
