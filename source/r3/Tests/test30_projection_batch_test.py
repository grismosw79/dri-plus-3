"""Compiled callback-local complete-room projection vs 22 independent reads.
SDF and Windows are controlled fixtures; native commit helpers are captured.
"""
import os,time
os.environ['DRI_TEST_REUSE_NATIVE_HEAP']='1'
exec(compile((__import__('pathlib').Path(__file__).parent/'portal_runtime_test.py').read_text(),'portal_runtime_test.py','exec'))
configure(1);wq(context+0xd0,tuning);records[tuning]['scalar'][39]=2
for a in [departures,roster,hslist,tplist,committed,arr]:records[a]['array']=[]
call(exports['DriTestOwnershipReset'])
records[team]['scalar'][384]=8
players=[];hs=[];transfers=[]
for pos in range(22):
 values=[pp(f'returner {pos}',70+pos%8,pos=pos),pp(f'capped {pos}',98,pos=pos),
         pp(f'graduate {pos}',99,3,pos=pos),pp(f'departed {pos}',88,pos=pos,tid=255),pp(f'NFL {pos}',92,pos=pos)]
 players.append(values[0]);records[roster]['array']+=values
 records[departures]['array'].append(record(f'NFL departure {pos}',{2:1,1:0},ref={4:values[-1]}))
 h,ht,hp=pr(88000+pos,76,pos=pos,stage=5,portal=False);hs.append((h,hp))
 t,tt,tp=pr(89000+pos,80,pos=pos,stage=5);transfers.append((t,tp))
# Real Player records share a table; departure identity validation requires it.
player_table=alloc(0x20);wq(player_table,TABLEVT);tables[player_table]={}
for p in records[roster]['array']+[p for _,p in hs+transfers]:
 wq(p+0x10,player_table);tables[player_table][ri(p+0x18)]=p
# Duplicate identities must merge, not add seats.
records[roster]['array'].append(players[0])
size=call(exports['DriTest30QualitySize'])
assert size==(1584 if 'DriTest31ClassAnnual' in exports else 1580),size
room_format='<'+str(size//4)+'i'
batch=alloc(size*22);single=alloc(size)
array_reads={'HS':0,'TP':0,'roster':0,'published':0,'departures':0}
watched={rq(r+0x10):name for r,name in [(hslist,'HS'),(tplist,'TP'),(roster,'roster'),(committed,'published'),(departures,'departures')]}
ref_address=rq(TABLEVT+0x128);original_ref=handlers[ref_address]
def counting_ref():
 t,_,_,f=args()
 if t in watched and f<0x100000:array_reads[watched[t]]+=1
 return original_ref()
handlers[ref_address]=counting_ref
def compare(which):
 for key in array_reads:array_reads[key]=0
 start=time.monotonic()
 assert call(exports['DriTest30ProjectedRooms' if which else 'DriTest30CurrentRooms'],team,context,batch)==1
 batch_seconds=time.monotonic()-start;batch_reads=dict(array_reads)
 for key in array_reads:array_reads[key]=0
 start=time.monotonic()
 for pos in range(22):
  assert call(exports['DriTestProjectedRoom' if which else 'DriTestQualityRoom'],team,context,pos,single)==1
  assert bytes(uc.mem_read(batch+size*pos,size))==bytes(uc.mem_read(single,size)),(which,pos)
 single_seconds=time.monotonic()-start;single_reads=dict(array_reads)
 for key in batch_reads:assert single_reads[key]==22*batch_reads[key],(key,batch_reads,single_reads)
 return {'kind':'projected' if which else 'current','batch_array_reads':batch_reads,'repeated_array_reads':single_reads,'emulated_batch_seconds':batch_seconds,'emulated_repeated_seconds':single_seconds}
bench=[compare(1),compare(0)]
# Independent expected numeric room: returner +2, cap99, HS unchanged, TP+2.
assert call(exports['DriTest30ProjectedRooms'],team,context,batch)==1
for pos in range(22):
 q=struct.unpack(room_format,uc.mem_read(batch+size*pos,size))
 assert q[0]==pos and q[3]==4 and q[4]==3 and q[5]==82,(pos,q[:6])
 assert list(q[6:10])==sorted([72+pos%8,99,76,82],reverse=True),(pos,q[6:10])
# Same callback API after each mutation must recollect; no week-wide cache.
records[players[1]]['scalar'][191]=85
assert call(exports['DriTest30ProjectedRooms'],team,context,batch)==1
assert struct.unpack(room_format,uc.mem_read(batch+size,size))[6:10]==(99,87,82,76)
for r,p in hs+transfers:records[r]['scalar'][9]=6;records[committed]['array'].append(p)
bench.append(compare(1))
assert call(exports['DriTest30ProjectedRooms'],team,context,batch)==1
assert struct.unpack(room_format,uc.mem_read(batch,size))[3]==4
records[tuning]['scalar'][39]=0
assert call(exports['DriTest30ProjectedRooms'],team,context,batch)==1
assert struct.unpack(room_format,uc.mem_read(batch,size))[6:10]==(98,80,76,70)
# Malformed config fails the complete batch; nothing silently invents a room.
old=records[ideal_entries[0]]['scalar'][2];records[ideal_entries[0]]['scalar'][2]=31
assert call(exports['DriTest30ProjectedRooms'],team,context,batch)==0
records[ideal_entries[0]]['scalar'][2]=old
assert call(exports['DriTest30ProjectedRooms'],team,context,batch)==1
call(exports['DriTestOwnershipReset']);assert all(v==1 for v in refs.values())
assert call(exports['DriTestQualityFault'])==0
result={'status':'PASS','method':'compiled Windows projection functions with captured native ownership helper and controlled SDF/Windows services','checks':['22 current and projected rooms byte-identical to independent per-position collection','independent expected counts and OVR projection across all positions','graduation/old ownership/confirmed departures excluded','duplicate roster and published acquisition merged','same-week rating and publication mutations immediately recollected','changed projection setting immediately applied','malformed config rejected','references balanced'],'workload':{'positions':22,'roster_rows':len(records[roster]['array']),'HS':22,'transfers':22,'departures':22},'benchmarks':bench,'live_game_verified':False,'native_game_timing_measured':False}
(OUTPUT/'test30_projection_batch_results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
