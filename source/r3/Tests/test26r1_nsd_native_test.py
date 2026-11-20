"""Execute captured native lookup inside the compiled NSD finalization callback.

This deliberately does not mock the 0x081689e0 lookup ABI. Native HS signing,
record storage and Windows services remain fixtures; this is not a live season.
Run against the released Test26 fixture as a negative control before shipping.
"""
exec(compile((__import__('pathlib').Path(__file__).parent/'recovery_runtime_test.py').read_text(),'recovery_runtime_test.py','exec'))

def fill_exhausted(tid):
 # Inherited players no longer spend current general places. Add two real
 # published HS overages elsewhere so the fallback still tests reason 8.
 fill(tid,(94,92,91,90,89,88))
 roster_array=records[allteams[tid]]['ref'][239]
 published=records[allteams[tid]]['ref'][33]
 for pos in (5,9):
  ideal=records[ideal_entries[pos]]['scalar'][2]
  records[roster_array]['array'] += [pp('baseline '+str(tid)+'/'+str(pos)+'/'+str(i),70,pos=pos,tid=tid) for i in range(ideal)]
  acquired=pp('secured HS '+str(tid)+'/'+str(pos),72,year=0,pos=pos,tid=255)
  records[published]['array'].append(acquired)

checks=[]
records[season]['scalar'].update({4:6,5:6})
game_api(0x0816b130,lambda:1)
reset_recovery()
a,ap=rr(57020,schools=((10,20),(100,50)),offers=(10,100),portal=False)
b,bp2=rr(57021,schools=((5,10),(10,20),(100,50)),offers=(5,10,100),portal=False)
fill_exhausted(100)
nf=STACK+0x70000;nb=STACK+0x74000;visited=[];lookups=[];native_writes=[]
wq(nf+2*8,context);wq(nf+10*8,nb);uc.mem_write(context+0x1a0,b'\1')
table=nb-0x78;buckets=alloc(139*8);nodes={}
wq(table+8,buckets);wi(table+16,139);wi(table+20,138)
for tid in range(138):
 node=alloc(48);nodes[tid]=node;wi(node,tid);wq(buckets+tid*8,node)
map_game(0x081689e0,0x250)
uc.mem_write(GAME+0x081689e0,raw[0x081689e0-0x1000:0x08168c2e-0x1000])
handlers.pop(GAME+0x081689e0,None)
def capture_lookup(u,address,size,data):
 if address==GAME+0x081689e0:
  mp,out,unused,tidref=args();tid=ri(tidref)
  assert mp==table and unused==0 and rq(reg('RSP')+0x28)==tid
  lookups.append({'tid':tid,'out':out})
def capture_write(u,access,address,size,value,data):
 if GAME+0x081689e0<=reg('RIP')<GAME+0x08168c2e and lookups:
  out=lookups[-1]['out']
  if out<=address<out+24:native_writes.append((address-out,size))
uc.hook_add(unicorn.UC_HOOK_CODE,capture_lookup)
uc.hook_add(unicorn.UC_HOOK_MEM_WRITE,capture_write)
def native_hs_pass():
 ev,tr,vector,setmap=args();destination=rq(tr)
 assert destination in records,('native lookup overwrote Team reference',hex(destination),lookups,native_writes)
 tid=records[destination]['scalar'][383]
 assert ev==context and vector==nodes[tid]+8 and setmap==nb+0x18
 assert rq(reg('RSP')+0x28)==nb-0x18 and rq(reg('RSP')+0x30)==0
 result=lookups[-1]
 assert rq(result['out'])==nodes[tid] and rq(result['out']+8)==buckets+tid*8
 assert bytes(uc.mem_read(result['out']+16,1))==b'\0'
 visited.append(tid)
 if tid==10:records[a]['scalar'][9]=6;fill_exhausted(10)
 elif tid==5:records[b]['scalar'][9]=6
 else:raise AssertionError(tid)
 return 0
class_refresh=[]
game_api(0x081bb580,lambda:class_refresh.append(args()[0]) or 0)
game_api(0x081b40f0,native_hs_pass)
call(hooks[24],nf)
assert visited==[10,5] and class_refresh==[context],(visited,class_refresh)
assert len(lookups)==2 and (16,1) in native_writes and any(o==0 and n>=8 for o,n in native_writes),native_writes
assert records[a]['scalar'][9]==records[b]['scalar'][9]==6
call(hooks[24],nf);assert visited==[10,5] and class_refresh==[context]
uc.mem_write(context+0x1a0,b'\0');call(hooks[24],nf);assert visited==[10,5]
# Check both lookup result paths against guard bytes. The production caller's
# map normally already contains all teams; insertion still has the same ABI.
def capacity_check():
 _,out,capacity,count=args();assert capacity==139 and count==137
 uc.mem_write(out,b'\0');return out
game_api(0x019e9b60,capacity_check)
game_api(0x019ea660,lambda:alloc(reg('RDX')))
abi_cases=[]
for tid,insert in [(0,False),(5,False),(10,False),(137,False),(100,True)]:
 if insert:wq(buckets+tid*8,0);wi(table+20,137)
 guard=alloc(56);uc.mem_write(guard,b'\xa5'*56);out=guard+16;tidref=alloc(8);wi(tidref,tid)
 wq(STACK+0xf0000+0x20,tid)
 assert call(GAME+0x081689e0,table,out,0,tidref)==out
 assert rq(out)==rq(buckets+tid*8) and rq(out+8)==buckets+tid*8
 assert bytes(uc.mem_read(out+16,1))==bytes([int(insert)])
 assert bytes(uc.mem_read(guard,16))==b'\xa5'*16
 assert bytes(uc.mem_read(out+17,23))==b'\xa5'*23
 abi_cases.append({'team':tid,'inserted':insert,'guards_intact':True})
call(exports['DriTestOwnershipReset'])
assert all(v==1 for v in refs.values()),[(records[k]['name'],v)for k,v in refs.items()if v!=1]
assert call(exports['DriTestQualityFault'])==0
checks.append('Captured native lookup writes both pointers and inserted flag inside compiled callback; Team reference survives; earlier-school fallback signs and class statistics refresh once')
(OUTPUT/'test26r1_nsd_native_results.json').write_text(json.dumps({'checks':checks,'native_lookup_write_spans':native_writes,'visited_teams':visited,'native_abi_cases':abi_cases,'live_game_verified':False},indent=2)+'\n')
print(json.dumps(checks,indent=2))
