"""Next-season estimate uses one native OVRProjection step, never edits ratings."""
exec(compile((__import__('pathlib').Path(__file__).parent/'portal_runtime_test.py').read_text(),'portal_runtime_test.py','exec'))
configure(1);wq(context+0xd0,tuning);records[tuning]['scalar'][39]=2
for a in [departures,roster,hslist,tplist,committed,arr]:records[a]['array']=[]
records[team]['scalar'][384]=9
returner=pp('returner',74);cap=pp('cap',98);senior=pp('graduate',99,year=3)
records[roster]['array']=[returner,cap,senior]
h,ht,hp=pr(42000,76,stage=5,portal=False)
t,tt,tp=pr(42001,80,stage=5);valid_offer(t,tt)
out=alloc(1584)
assert call(exports['DriTestProjectedRoom'],team,context,0,out)==1
q=struct.unpack('<394i',uc.mem_read(out,1576));assert q[:6]==(0,4,1,4,3,82),q[:6]
assert q[6:10]==(99,82,76,76),q[6:10]
assert call(exports['DriTestQualityRoom'],team,context,0,out)==1
q=struct.unpack('<394i',uc.mem_read(out,1576));assert q[6:10]==(98,80,76,74)
assert [records[p]['scalar'][191] for p in [returner,cap,hp,tp,senior]]==[74,98,76,80,99]
# Publishing the portal and HS acquisition cannot change their projection/double count.
records[t]['scalar'][9]=6;records[h]['scalar'][9]=6;records[committed]['array']=[hp,tp]
assert call(exports['DriTestProjectedRoom'],team,context,0,out)==1
q=struct.unpack('<394i',uc.mem_read(out,1576));assert q[3]==4 and q[6:10]==(99,82,76,76)
# Zero increment is conservative and diagnostics still never edit player OVR.
records[tuning]['scalar'][39]=0
assert call(exports['DriTestProjectedRoom'],team,context,0,out)==1
assert struct.unpack('<394i',uc.mem_read(out,1576))[6:10]==(98,80,76,74)
call(exports['DriTestOwnershipReset']);assert all(v==1 for v in refs.values())
assert call(exports['DriTestQualityFault'])==0
checks=['Returning player +2, incoming transfer +2, incoming HS unchanged, cap 99, graduate excluded',
 'Hard commitment and published acquisition give the same projected roster with no duplicate player',
 'Current-OVR signing policy and saved player ratings remain unchanged',
 'Zero configured increment falls back to current ratings; all references balanced']
(OUTPUT/'projection_runtime_results.json').write_text(json.dumps({'checks':checks,'estimate':'One configured OVRProjection step; actual development is not predicted','live_game_verified':False},indent=2)+'\n')
print(json.dumps(checks,indent=2))
