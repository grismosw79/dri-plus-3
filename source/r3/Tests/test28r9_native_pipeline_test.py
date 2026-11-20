"""R9 captured native HS shortlist -> actual compiled reservation/allocator/admit.
Native storage/grade services are modeled; no live-game outcome is claimed.
"""
exec(compile((__import__('pathlib').Path(__file__).parent/'portal_runtime_test.py').read_text(),'portal_runtime_test.py','exec'))
configure(1);call(exports['DriTestOwnershipReset']);call(exports['DriTestHsPlanReset']);call(exports['DriTestHsCoverageReset'])
records[season]['scalar'].update({2:2045,4:1,5:1})
for x in [departures,roster,hslist,tplist,committed,arr]:records[x]['array']=[]
for pos,e in enumerate(ideal_entries):
 records[e]['scalar'].update({2:ideals[pos],4:1})
 records[roster]['array'] += [pp('returner '+str(pos)+' '+str(j),65,pos=pos)for j in range(max(0,ideals[pos]-(pos==0)))]
records[team]['scalar'][384]=5
primary,pt,ppp=pr(949,82,year=0,pos=0,stage=0,portal=False)
backup,bt,bpp=pr(985,81,year=0,pos=0,stage=0,portal=False)
# Labels mirror observed identities, but fixture ratings/grades/resources are controlled.
for r in [primary,backup]:
 records[r]['scalar'][3]=1000
 for e in records[records[r]['ref'][13]]['array']:records[e]['scalar'][1]=0
valid_offer(primary,pt)
offer=record('offer action',{0:3,1:0,2:5,5:1,6:1});social=record('social action',{0:5,1:4,2:5,5:1,6:0})
records[tuning]['ref'][49]=record('actions',array=[offer,social])
records[board]['scalar'].update({1:0,2:0,3:1500})
# Give native scorer synthetic positive scores. Actual captured scoring was
# separately verified; this isolates quota -> pair/admission dependency.
def score_fn():
 cand=rq(reg('RSP')+0x30);wi(cand+40,500 if rq(cand)==primary else 482);return 0
game_api(0x08185840,score_fn)
ctx=alloc(0xc00);uc.mem_write(ctx,bytes(uc.mem_read(context,0x300)))
for pos in range(22):wi(ctx+0x9a4+4*pos,1)
af=STACK+0x23000;asp=af+128;abp=asp+0x200
wq(af+10*8,abp);wq(af+2*8,ctx);wq(af+3*8,team_ref)
wq(abp+0x28,team_ref);wq(abp+0x58,season);wq(abp+0x180,need);wq(asp+0x58,ctx);uc.mem_write(asp+0x42,b'\0')
results=[]
for hook_enabled in [False,True]:
 backup_score=482
 configure(1);call(exports['DriTestOwnershipReset']);call(exports['DriTestHsPlanReset']);call(exports['DriTestHsCoverageReset'])
 records[arr]['array']=[pt];records[pt]['ref'][10]=primary;records[pt]['scalar'][12]=3
 slot=call(exports['DriTestHsRecoveryDestination'],backup,team,ctx,1)
 role=call(exports['DriTestHsRecoveryDestination'],backup,team,ctx,2)
 assert slot==1 and role==2,(slot,role)
 assert call(exports['DriTestHsRecoveryReserve'],backup,team,ctx,slot*4+role)==1
 call(hooks[8],af);call(start,af);assert call(exports['DriTestQualityFault'])==0
 # Execute captured shortlist instructions with the actual compiled R9 hook.
 map_game(0x0817bd10,0x1000);uc.mem_write(GAME+0x0817bd10,raw[0x0817bd10-0x1000:0x0817cd10-0x1000])
 qhook=call(exports['DriTestHsQuotaHook'])
 if hook_enabled:uc.mem_write(GAME+0x0817bfdf,b'\xff\x25\0\0\0\0'+struct.pack('<Q',qhook)+b'\x90'*4)
 uc.ctl_remove_cache(GAME+0x0817bd10,GAME+0x0817cd10)
 isp=STACK+0x60000;ibp=isp+0x800
 iv=alloc(24);ic=alloc(96);ip=alloc(22*72);ii=alloc(64);inn=alloc(88);inref=alloc(8)
 wq(iv,ic);wq(ip,ii);wq(ip+8,ii);wq(ip+16,ii+64);wq(inref,inn);wi(inn,1)
 wq(ibp+0xe0,ip);wq(isp+0x60,ctx);wq(ibp-0x60,team_ref);wq(ibp-0x58,inref);wq(isp+0x58,iv);uc.mem_write(ibp+0x7b0,b'\0')
 def append_index():
  v=reg('RCX');end=rq(v+8);wi(end,ri(reg('RDX')));wq(v+8,end+4);return 0
 game_api(0x081bc470,append_index)
 for idx,val in enumerate([500,482]):
  wi(ic+idx*48+40,val);wq(isp+0x50,idx)
  for rn,v in [('RAX',0),('RSI',0),('R13',ic+idx*48),('RBP',ibp),('RSP',isp)]:uc.reg_write(globals()['UC_X86_REG_'+rn],v)
  uc.emu_start(GAME+0x0817bf83,GAME+0x0817c0d8,count=500000)
  assert reg('RIP')==GAME+0x0817c0d8
 backup_score=ri(ic+48+40);assert backup_score==(482 if hook_enabled else 0)
 entries=[]
 for r,value in [(primary,500),(backup,backup_score)]:
  e=bytearray(48);struct.pack_into('<Q',e,0,r);struct.pack_into('<i',e,40,value);entries.append(bytes(e))
 vec=alloc(24);mem=alloc(192);main=mem+48;canary=b'\xa5'*48
 uc.mem_write(mem,canary+b''.join(entries)+canary);wq(vec,main);wq(vec+8,main+96);wq(vec+16,main+96)
 wq(abp-0x68,vec);wq(asp+0x68,2);wi(abp+0x30,50);wq(asp+0x50,1);wq(asp+0x78,0);wq(abp-0x80,0)
 call(hooks[15],af);assert call(exports['DriTestQualityFault'])==0
 admitted=[]
 for r in [primary,backup]:
  entry=alloc(48);wq(entry,r);wq(af+11*8,entry);admitted.append(call(admit,af)==0)
 assert admitted==[True,backup_score>0],(backup_score,admitted)
 assert sorted(bytes(uc.mem_read(main+i*48,48))for i in range(2))==sorted(entries)
 assert bytes(uc.mem_read(mem,48))==canary and bytes(uc.mem_read(main+96,48))==canary
 assert records[arr]['array']==[pt]
 results.append({'backup_candidate_score':backup_score,'reservation':{'slot':slot,'role':role},'admitted':admitted,'exact_vector_preserved':True,'offers_created_by_probe':False})
# Inspect actual DLL eligibility probe ABI before replaying native gate below.
gate_args=[]
def gate_probe():gate_args.append(args());return 1
game_api(0x0816b130,gate_probe)
call(exports['DriTestHsRecoveryDestination'],backup,team,ctx,0)
assert gate_args and all(x[0]==rq(ctx+0xc8) for x in gate_args)
abi={'DLL_first_argument':'manager','native_HS_call_first_argument':'qword[evaluator+0xc8]','distinct':ctx!=rq(ctx+0xc8),'calls_observed':len(gate_args)}
assert abi['distinct']
# Captured eligibility function: native branches and grade comparison are real;
# record schema recognition and grade services are modeled dependencies.
map_game(0x0816b130,0x500);uc.mem_write(GAME+0x0816b130,raw[0x0816b130-0x1000:0x0816b610-0x1000]);handlers.pop(GAME+0x0816b130,None);uc.ctl_remove_cache(GAME+0x0816b130,GAME+0x0816b610)
native_manager=alloc(0xc00);player_type=0xabc001;recruit_type=0xabc002
wq(native_manager+0x80,player_type);wq(native_manager+0xa0,recruit_type);wq(ctx+0xc8,native_manager)
wi(native_manager+0xb08,0);wq(native_manager+0x180,0x111111);wq(native_manager+0x1b0,0x222222)
records[bpp]['scalar'][233]=0
# Validate assertions based on their actual bool argument, unlike generic fixture.
game_api(0x037f5e40,lambda:reg('RCX')&1)
schema_seen=[]
def schema_match():
 schema_seen.append(hex(reg('RDX')));return int(reg('RDX')==recruit_type)
game_api(0x08059c00,schema_match)
game_api(0x0862db70,lambda:0) # ancillary recruit-wrapper helper; native releases ownership itself
seen=[]
def requirement():
 assert reg('RCX')==0x111111;seen.append('requirement');return 10
def grade_meta():wq(reg('RDX'),0);return reg('RDX')
def grade():assert reg('RCX')==0x222222;seen.append('grade');return current_grade
for addr,fn in [(0x08406fa0,requirement),(0x084332f0,grade_meta),(0x08433370,grade),(0x0817fa40,lambda:0),(0x08187270,lambda:0),(0x007ac0c0,lambda:0)]:game_api(addr,fn)
br=alloc(8);wq(br,backup);tr=alloc(8);wq(tr,team)
status=[]
for receiver,current_grade in [(ctx,5),(native_manager,5),(native_manager,10)]:
 seen.clear();schema_seen.clear();v=call(GAME+0x0816b130,receiver,tr,br,0)
 status.append({'receiver':'evaluator' if receiver==ctx else 'manager','school_grade':current_grade,'requirement':10,'allowed':bool(v),'grade_services_called':list(seen),'schema_arguments':list(schema_seen)})
print('GATE_STATUS',status,flush=True)
assert status[0]['allowed'] and not status[0]['grade_services_called']
assert not status[1]['allowed'] and status[1]['grade_services_called']==['requirement','grade']
assert status[2]['allowed'] and status[2]['grade_services_called']==['requirement','grade']
# Checked production helper must reject and accept through the captured function.
current_grade=5
assert call(exports['DriTestEligibility'],backup,team,ctx)==0
current_grade=10
assert call(exports['DriTestEligibility'],backup,team,ctx)==1
# Null manager and unrecognized schemas fail closed rather than pass eligibility.
wq(ctx+0xc8,0);assert call(exports['DriTestEligibility'],backup,team,ctx)==0
assert call(exports['DriTestQualityFault'])==1
configure(1);wq(ctx+0xc8,native_manager);wq(native_manager+0xa0,0xdeadbeef)
assert call(exports['DriTestEligibility'],backup,team,ctx)==0
assert call(exports['DriTestQualityFault'])==1
call(exports['DriTestOwnershipReset']);assert all(v==1 for v in refs.values()),[(records[r]['name'],v)for r,v in refs.items()if v!=1]
result={'method':__doc__,'allocation_cases':results,'eligibility_ABI':abi,'captured_gate_cases':status,'live_game_verified':False}
(OUTPUT/'test28r9_native_pipeline_results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
