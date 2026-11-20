"""Verify the captured calendar evidence and the compiled PT phase resolver.

This is a serialized/native evidence replay, not a live lifecycle test.
The small original SPEX rows are bundled; no outside save/parser is needed.
"""
from pathlib import Path
import hashlib
import json
import os
import struct

SOURCE = Path(__file__).resolve().parents[1]
EVIDENCE = SOURCE.parent / 'Audit/PT28/calendar-evidence.json'
evidence = json.loads(EVIDENCE.read_text())
scheduler_evidence = json.loads((EVIDENCE.parent/'scheduler-evidence.json').read_text())
checks = []

assert evidence['expressionSHA256'] == 'cfc18507f5c755635660d0ff58db11dd728757a4539ea6d3b38f5fb8076dcde8'
assert evidence['expressionDecompressedSHA256'] == '051770a37b32cf0abc8fb713ce63029f48f8d8d632e2c2cf6a018b83822861fa'

def decode(row):
    data = bytes.fromhex(row['bytes'])
    assert hashlib.sha256(data).hexdigest() == row['sha256']
    assert len(data) == row['end'] - row['start']
    pos = 0
    def u32():
        nonlocal pos
        value = struct.unpack_from('>I', data, pos)[0]
        pos += 4
        return value
    locals_ = [[u32(), u32()] for _ in range(u32())]
    assert locals_ == row['locals']
    ops = []
    for _ in range(u32()):
        begin = pos
        kind = u32()
        assert kind in (0xffffffff, 1, 2, 3, 4, 6, 7)
        size = 0 if kind == 0xffffffff else 1 if kind == 1 else 4
        value = int.from_bytes(data[pos:pos+size], 'big') if size else None
        pos += size
        opcode, operand = struct.unpack_from('>HH', data, pos)
        pos += 4
        ops.append(dict(offset=begin+row['start'], type=kind, value=value,
                        opcode=opcode, operand=operand))
    assert pos == len(data)
    assert ops == row['ops']
    return [(o['opcode'], o['operand'], o['value']) for o in ops]

rows = {(r['table'], r['row']): decode(r) for r in evidence['rows']}
known = rows[378, 0]
proof = evidence['sourceKnownOpcodeProof']
assert 'if (contractStatus == PlayerContractStatus.Drafted)' in proof['source']
assert 'else if (contractStatus == PlayerContractStatus.Signed)' in proof['source']
assert known[3:8] == [(6,3,None),(31,725,None),(27,0,None),(67,0,None),(75,0,10)]
assert known[17:22] == [(6,3,None),(31,725,None),(27,3,None),(67,0,None),(75,0,18)]
assert 7 + known[7][2] == 17
assert 21 + known[21][2] == len(known)
assert known[16] == (73,0,23) and 16 + known[16][2] == len(known)
checks.append('Source-known Drafted/Signed branches establish equality67 and false-branch75 semantics')

annual = rows[86,257]
assert annual[:7] == [(9,0,0),(20,3,None),(20,25,None),(17,6,None),(1,0,7),(67,0,None),(75,0,15)]
assert annual[7:14] == [(9,0,0),(20,4,None),(20,3,None),(9,0,0),(20,4,None),(1,0,0),(80,0,None)]
assert annual[14:21] == [(9,0,0),(20,4,None),(20,2,None),(9,0,0),(20,4,None),(1,0,0),(80,0,None)]
assert 6 + annual[6][2] == 21
def binding(owner, field):
    return next(x for x in evidence['ownerBindings'][owner] if x['i']==field)
assert binding('RecruitingOffSeasonWeekStartReaction',1)['value'] == (86<<17)|257
assert binding('RecruitingOffSeasonWeekStartReaction',3)['name'] == 'SeasonManager'
assert binding('RecruitingOffSeasonWeekStartReaction',4)['name'] == 'TeamManager'
assert binding('SeasonManager',25)['name'] == 'SeasonInfo'
assert binding('TeamManager',3)['value'] == 0xffffffff
assert binding('TeamManager',2)['value'] == 0xffffffff
methods = {x['index']:x['name'] for x in evidence['sourceMethodMaps']['TeamManager']}
assert methods[3] == 'AdvanceSchoolYearForAllTeams'
assert methods[2] == 'AdvanceRedshirtStatusForAllTeams'
assert evidence['sourceSchemaSeasonInfoOrdinals']['CurrentWeek'] == 6
assert evidence['nativeSeasonInfoOrdinals']['CurrentWeek'] == 4
checks.append('Week7 equality invokes school-year then redshirt advancement; script and native field ordinals are distinct and mapped')

training = rows[86,375]
assert training == [(9,0,0),(20,3,None),(0,0,1),(42,12,None),(9,0,0),(20,3,None),(0,0,0),(42,11,None),(9,0,0),(20,3,None),(20,4,None),(9,0,0),(20,3,None),(1,0,0),(80,0,None)]
assert binding('TrainingResultsStartReaction',3)['name'] == 'PlayerProgressionProgressTeamsTransaction'
nsd = evidence['nsdSeasonInfo']
assert (nsd['CurrentStage'],nsd['CurrentWeek'],nsd['CurrentOffseasonStage']) == ('OffSeason',6,7)
appointments = {x['Name']:x for x in evidence['appointments']}
assert evidence['scheduler']['CurrentTime'] < appointments['Offseason Week']['StartOccurrenceTime'] < appointments['Training Results']['StartOccurrenceTime'] < appointments['Advance Week']['StartOccurrenceTime']
checks.append('NSD week6 precedes week7 year advancement and Training Results progression start before the next stopping appointment')

# The stage label is set before either annual callback. It is not, by itself,
# proof that training has completed inside an in-progress week advance.
week_end = rows[86,200]
assert binding('OffSeasonWeekEndReaction',1)['value'] == (86<<17)|200
assert binding('OffSeasonWeekEndReaction',3)['name'] == 'SeasonManager'
assert week_end[17:21] == [(9,0,0),(20,3,None),(20,25,None),(17,3,None)]
assert week_end[24:33] == [(68,0,None),(75,0,8),(9,0,0),(20,3,None),(20,25,None),(22,1,None),(1,0,1),(58,0,None),(43,3,None)]
assert appointments['Offseason Week']['EndOccurrenceTime'] < appointments['Offseason Week']['StartOccurrenceTime']
checks.append('Stage8 is assigned before year advance and training; it cannot alone certify a stable checkpoint')

rule = scheduler_evidence['configuredAdvanceWeek']
assert (rule['pattern'],rule['unitType'],rule['units'],rule['termType']) == (0,3,7,0)
assert rule['derivedInterval'] == rule['units'] * rule['nativeUnitSeconds']
assert rule['nextOccurrenceTime']-rule['nsdCurrentTime'] == rule['derivedInterval']
assert scheduler_evidence['maps']['Scheduler'] == {'Epoch':0,'CurrentTime':1,'Appointments':2}
assert scheduler_evidence['maps']['Appointment']['IsActive'] == 2
assert scheduler_evidence['maps']['Appointment']['StartEvent'] == 5
assert scheduler_evidence['maps']['Appointment']['RecurrenceRule'] == 9
for saved in scheduler_evidence['rawSavedTables'].values():
    assert hashlib.sha256(bytes.fromhex(saved['headerAndRow0'])).hexdigest() == saved['sha256']
blocking_record=scheduler_evidence['trainingBlockingRecord']
blocking_bytes=bytes.fromhex(blocking_record['headerAndRow0'])
assert hashlib.sha256(blocking_bytes).hexdigest()==blocking_record['sha256']
blocking_header=blocking_record['header']
blocking_bit=struct.unpack_from('>I',blocking_bytes,blocking_header['offsetStart']+8*4)[0]
assert blocking_bit==blocking_record['bitOffset']==299
blocking_word=struct.unpack_from('>I',blocking_bytes,blocking_header['table1StartIndex']+(blocking_bit//32)*4)[0]
assert (blocking_word>>(blocking_bit%32))&1 == 1
root_table=json.loads((EVIDENCE.parent/'training-transaction-array-owner.json').read_text())[0]
root_header=root_table['header'];root_bytes=bytes.fromhex(root_table['data'])
root_field1=struct.unpack_from('>I',root_bytes,root_header['offsetStart']+4)[0]
assert struct.unpack_from('>I',root_bytes,root_header['table1StartIndex']+root_field1//8)[0]==5441<<17
transactions=json.loads((EVIDENCE.parent/'training-runtime-transaction-tables.json').read_text())['5441']
transaction_bytes=bytes.fromhex(transactions['data']);transaction_start=transactions['header']['table1StartIndex']
assert struct.unpack_from('>I',transaction_bytes,transaction_start-4)[0]==22
assert struct.unpack_from('>I',transaction_bytes,transaction_start+12*4)[0]==4901<<17
checks.append('Stopping interval derives from this appointment’s verified day-based recurrence configuration')

# Use the same compiled callback fixture as production adapter tests.
fixture = (SOURCE/'Tests/runtime_fixture_test.py').read_text()
exec(compile(fixture.split('A,TA=recruit')[0], 'runtime_fixture_test.py', 'exec'))
call(exports['DriTestConfigure'],GAME,1,7)
assert 'DriTestPt28Timing' in exports
out = alloc(12)
cases = 0
for stage in range(3):
    for week in range(61):
        for offseason in range(16):
            for hs in (0,1):
                records[season]['scalar'].update({3:stage,4:week,8:offseason})
                assert call(exports['DriTestPt28Timing'],season,hs,out) == 1
                got = struct.unpack('<3i',uc.mem_read(out,12))
                expected = (int(week<7),int(offseason<8),0) if stage==2 else ((1,1,0) if hs else (0,0,1))
                assert got == expected, (stage,week,offseason,hs,got,expected)
                cases += 1
for stage,week,offseason in [(-1,6,7),(3,6,7),(2,-1,7),(2,61,7),(2,6,-1),(2,6,16)]:
    records[season]['scalar'].update({3:stage,4:week,8:offseason})
    assert call(exports['DriTestPt28Timing'],season,0,out) == 0
checks.append('Compiled phase resolver matches every valid stage/week/offseason/HS combination and rejects invalid ranges')

# Replay the captured unit conversion and clock accessor. These use raw dynamic
# indices; applying the native-enum remapper to Scheduler fields would be wrong.
for page in (0x37ee800,0x37f0000):
    uc.mem_map(GAME+page,0x1000)
    uc.mem_write(GAME+page,raw[page-0x1000:page])
unit_out = alloc(4)
for units in (0,1,7,14):
    assert call(GAME+0x37ee960,unit_out,3,units,0) == unit_out
    assert ri(unit_out) == units*86400
clock = record('native Scheduler clock',{1:rule['nsdCurrentTime']})
clock_vtable=alloc(0x200)
uc.mem_write(clock_vtable,bytes(uc.mem_read(TABLEVT,0x200)))
wq(rq(clock+0x10),clock_vtable)
clock_reads=[]
def clock_scalar():
    table,row,field,_=args()
    assert field==1
    clock_reads.append(field)
    return records[tables[table][row]]['scalar'][field]
wq(clock_vtable+0x100,api(clock_scalar))
wi(clock+0x28,-1)
assert call(GAME+0x37f3390,clock)==rule['nsdCurrentTime']
assert clock_reads==[1] and ri(clock+0x28)==rule['nsdCurrentTime']
records[clock]['scalar'][1]=rule['nextOccurrenceTime']
assert call(GAME+0x37f3390,clock)==rule['nsdCurrentTime'] and clock_reads==[1]
wi(clock+0x28,rule['nextOccurrenceTime'])
assert call(GAME+0x37f3390,clock)==rule['nextOccurrenceTime']
checks.append('Captured native day conversion yields the configured interval; raw Scheduler clock access and cached-versus-persisted distinction replay correctly')

# Execute the actual SchedulerAdvance function. Only appointment selection,
# event dispatch and SDF storage are mocked. The captured control flow must
# dispatch one appointment and persist its timestamp before the next call.
uc.mem_write(GAME+0x37f3800,raw[0x37f0000:0x37f3800])
uc.mem_map(GAME+0x8055000,0x1000)
api(lambda:0,GAME+0x8059580)
appointment=record('next due native appointment',{4:0,7:-1})
appointment_vtable=alloc(0x200)
uc.mem_write(appointment_vtable,bytes(uc.mem_read(TABLEVT,0x200)))
wq(rq(appointment+0x10),appointment_vtable)
def raw_scalar():
    table,row,field,_=args()
    return records[tables[table][row]]['scalar'][field]
def raw_setter():
    table,row,field,value=args()
    records[tables[table][row]]['scalar'][field]=value
    return 0
wq(appointment_vtable+0x100,api(raw_scalar))
wq(clock_vtable+0x100,api(raw_scalar))
wq(clock_vtable+0x148,api(raw_setter))
def next_due():
    _,out_ref,_,_=args()
    incref(appointment)
    wq(out_ref,appointment)
    return out_ref
api(next_due,GAME+0x37f3920)
training_active=False
native_dispatches=[]
next_stop=rule['nextOccurrenceTime']
def stable_stop():
    delta=next_stop-records[clock]['scalar'][1]
    return training_active and delta in (0,rule['derivedInterval'])
def dispatch_event():
    global training_active
    current,out_ref,appointment_ref,_=args()
    assert current==clock and rq(appointment_ref)==appointment
    before=records[clock]['scalar'][1]
    current_event=ri(clock+0x28)
    assert before<current_event
    assert not stable_stop()
    if event_name=='training_start':
        training_active=True
        # Even this exact active-write/pre-persist interval is off the stop
        # lattice: earlier week-end/start appointments have already persisted.
        assert before>=appointments['Offseason Week']['StartOccurrenceTime']
        assert not stable_stop()
    native_dispatches.append(dict(event=event_name,persisted_before=before,live_during=current_event))
    wq(out_ref,0)
    return out_ref
api(dispatch_event,GAME+0x37ef340)
records[clock]['scalar'][1]=rule['nsdCurrentTime']
wi(clock+0x28,-1)
advanced=alloc(4)
for event_name,event_time in [('week_end',appointments['Offseason Week']['EndOccurrenceTime']),('week_start',appointments['Offseason Week']['StartOccurrenceTime']),('training_start',appointments['Training Results']['StartOccurrenceTime']),('advance_stop',next_stop)]:
    previous=records[clock]['scalar'][1]
    records[appointment]['scalar'][4]=event_time
    count_before=len(native_dispatches)
    assert call(GAME+0x37f4540,clock,event_time-previous,advanced)==event_time
    assert len(native_dispatches)==count_before+1
    assert records[clock]['scalar'][1]==event_time and ri(clock+0x28)==event_time
    assert stable_stop()==(event_name=='advance_stop')
next_stop+=rule['derivedInterval']
assert stable_stop() and refs[appointment]==1
checks.append('Captured SchedulerAdvance dispatches exactly one event then persists time; the IsActive-before-persist window cannot admit an old NSD stopping timestamp')

# The framework's actual transaction pass clamps its proposed calendar target
# to CurrentTime while a blocking training transaction has not finished.
uc.mem_map(GAME+0x3807800,0x1000)
uc.mem_write(GAME+0x3807800,raw[0x3806800:0x3807800])
uc.mem_map(GAME+0xfb49000,0x1000)  # captured profiler-enable byte; disabled
driver=alloc(0x100)
runtime_owner=alloc(0x20)
runtime_vt=alloc(0x80)
runtime_state=alloc(0x500)
wq(driver+8,runtime_owner);wq(runtime_owner,runtime_vt)
wq(runtime_vt+0x10,api(lambda:runtime_state))
transaction=alloc(0x30)
transaction_vt=alloc(0x90)
transaction_ref=alloc(8)
wq(transaction,transaction_vt);wq(transaction_ref,transaction)
transaction_done=False
transaction_blocking=True
process_count=0
def process_transaction():
    global process_count
    process_count+=1
    return 0
def time_scale():
    uc.reg_write(UC_X86_REG_XMM0,0)
    return 0
wq(transaction_vt+0x38,api(process_transaction))
wq(transaction_vt+0x58,api(lambda:int(transaction_done)))
wq(transaction_vt+0x68,api(lambda:int(transaction_blocking)))
wq(transaction_vt+0x78,api(time_scale))
current=appointments['Training Results']['StartOccurrenceTime']
future=rule['nextOccurrenceTime']
assert call(GAME+0x38084d0,driver,transaction_ref,current,future)==current
transaction_done=True
assert call(GAME+0x38084d0,driver,transaction_ref,current,future)==future
transaction_done=False;transaction_blocking=False
assert call(GAME+0x38084d0,driver,transaction_ref,current,future)==future
assert process_count==3
checks.append('Captured framework transaction pass freezes calendar time for unfinished blocking training and releases it only after completion or for nonblocking work')

# Check each packaged native snippet against the same verified capture used by
# the native-runtime regression suite, independently of its stored digest.
report = json.loads((CAPTURE/'report.json').read_text(encoding='utf-8-sig'))
assert report['ExecutableSha256'] == evidence['nativeCapture']['ExecutableSha256']
extra_native=[]
for filename in ('training-clock-barrier-native.json','training-transaction-native.json'):
    document=json.loads((EVIDENCE.parent/filename).read_text())
    extra_native.extend({'rva':int(r['rva'],16),'bytes':r['hex'],'sha256':r['sha256']} for r in document['ranges'])
for snippet in evidence['nativeSignatures']+scheduler_evidence['nativeSignatures']+extra_native:
    data = bytes.fromhex(snippet['bytes'])
    assert hashlib.sha256(data).hexdigest() == snippet['sha256']
    sec = next(x for x in report['Sections'] if x.get('Selected') and x['Rva']<=snippet['rva']<x['Rva']+(CAPTURE/x['File']).stat().st_size)
    with (CAPTURE/sec['File']).open('rb') as stream:
        stream.seek(snippet['rva']-sec['Rva'])
        assert stream.read(len(data)) == data
checks.append('Native action registration, year guards and field descriptors match the verified captured executable')
result = {'checks':checks,'compiled_policy_cases':cases,'native_scheduler_dispatches':native_dispatches,'static_native_evidence_verified':True,'live_game_verified':False}
(OUTPUT/'pt28_calendar_results.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result))
