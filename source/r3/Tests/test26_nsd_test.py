"""Test26 compiled final-offer selector with native removal and reference APIs.
Native dealbreaker and HS pass services are fixtures, not a live season.
"""
# The final NSD callback intentionally performs several complete native school
# passes plus R8 matching. Keep a finite instruction budget for that larger
# integration operation; individual callback fixtures retain the 2M default.
__import__('os').environ['DRI_TEST_INSTRUCTION_LIMIT']='10000000'
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

checks=[];denied=set();records[season]['scalar'].update({4:6,5:6});game_api(0x0816b130,lambda:int(records[rq(args()[1])]['scalar'][383] not in denied))
def winner(r,prune=0):
 result=call(exports['DriTestNsdWinner'],r,context,prune)
 if result:decref(result)
 assert call(exports['DriTestQualityFault'])==0
 return result
def points(r,tid,n):
 # Emulated out-of-band edits bypass the native weekly/ownership invalidation.
 if 'DriTestNsdReset' in exports:call(exports['DriTestNsdReset'])
 for t in records[board_targets(allteams[tid])]['array']:
  if records[t]['ref'].get(10)==r:records[t]['scalar'][8]=n
for portal in [False,True]:
 reset_recovery();r,p=rr(56000+portal,score=160,stage=3,schools=((10,19),(11,11)),offers=(10,11,96),portal=portal)
 points(r,96,25);before=values(r)
 assert winner(r)==allteams[96] and values(r)==before and records[r]['scalar'][9]==3
 points(r,96,0);assert winner(r)==allteams[10]
 denied.add(10);assert winner(r)==allteams[11];denied.clear()
 fill_exhausted(10);chosen=winner(r,1);assert chosen==allteams[11] and not offer_status(r,10),(portal,records.get(chosen),offer_status(r,10),values(r))
 assert values(r)==before and records[r]['scalar'][14]==2
 checks.append(('Portal' if portal else 'HS')+': highest active offer wins below threshold and outside shortlist; native eligibility/capacity still apply; invalid offer removed without invented influence')
reset_recovery();r,p=rr(56010,schools=((10,0),(11,0)),offers=(11,10))
assert winner(r)==allteams[10]
records[r]['scalar'][9]=5;assert winner(r)==0
records[r]['scalar'][9]=6;assert winner(r)==0
records[r]['scalar'][9]=2
for m in [0,-1]:configure(m);assert winner(r)==0
configure(1)
checks.append('Zero-influence offers sign; equal influence uses smaller TeamIndex; hard/signed ownership and diagnostic/disabled modes untouched')
reset_recovery();r,p=rr(56011,schools=((0,999),(7,998),(55,997)),offers=(0,7,55))
assert winner(r,1)==0 and records[r]['scalar'][14]==0
checks.append('Portal academy offers excluded; no remaining valid offer means no destination')
# Verify exact NSD caller routing at both native selector sites.
reset_recovery();r,p=rr(56012,schools=((10,100),(11,90)),offers=(10,96));points(r,96,120)
f=STACK+0x68000;bp=f+0x400;rrp=alloc(8);wq(rrp,r);out=alloc(8)
wq(f+10*8,bp);wq(bp+0x67,context);wq(bp+0x77,rrp);wq(bp-0x39,allteams[10])
for caller in [0x081b45cb,0x081b58cf]:
 wq(bp+0x5f,GAME+caller);wq(f+14*8,1);call(hooks[11],f);assert rq(f+14*8)==0
for caller in [0x081b4602,0x081b58fa]:
 wq(f+13*8,context);wq(f+12*8,out);wq(f+7*8,rrp);wq(f+128+0x108,GAME+caller)
 assert call(hooks[12],f)==1 and rq(out)==allteams[96] and rq(f+14*8)==out
 decref(rq(out));wq(out,0)
wq(f+128+0x108,GAME+0x08103d10);assert call(hooks[12],f)==0 and rq(out)==0
checks.append('Verified HS and portal NSD caller frames reject lower listed school and return highest outside-list offer; unrelated caller retains native path')
# Final HS pass closes the earlier-school fallback ordering hole before native
# publication. Native vector lookup and per-team signing are controlled services.
reset_recovery();a,ap=rr(56020,schools=((10,20),(100,50)),offers=(10,100),portal=False)
b,bp2=rr(56021,schools=((5,10),(10,20),(100,50)),offers=(5,10,100),portal=False)
fill_exhausted(100);nf=STACK+0x70000;nb=STACK+0x74000;entry=alloc(64);visited=[]
wq(nf+2*8,context);wq(nf+10*8,nb);uc.mem_write(context+0x1a0,b'\1')
def get_vector():
 mp,out,unused,tidref=args();assert mp==nb-0x78 and unused==0
 assert rq(reg('RSP')+0x28)==ri(tidref)
 # Native result is an iterator pair plus insertion flag, padded to 24 bytes.
 wq(out,entry);wq(out+8,entry+48);uc.mem_write(out+16,b'\0');return out
def native_hs_pass():
 ev,tr,vector,setmap=args();tid=records[rq(tr)]['scalar'][383]
 assert ev==context and vector==entry+8 and setmap==nb+0x18
 assert rq(reg('RSP')+0x28)==nb-0x18 and rq(reg('RSP')+0x30)==0
 visited.append(tid)
 if tid==10:records[a]['scalar'][9]=6;fill_exhausted(10)
 elif tid==5:records[b]['scalar'][9]=6
 else:raise AssertionError(tid)
 return 0
class_refresh=[]
game_api(0x081bb580,lambda:class_refresh.append(args()[0]) or 0)
game_api(0x081689e0,get_vector);game_api(0x081b40f0,native_hs_pass)
call(hooks[24],nf)
assert visited==[10,5] and records[a]['scalar'][9]==records[b]['scalar'][9]==6 and class_refresh==[context]
call(hooks[24],nf);assert visited==[10,5]
uc.mem_write(context+0x1a0,b'\0');call(hooks[24],nf);assert visited==[10,5]
checks.append('HS finalization retries earlier-school fallbacks after later schools fill; exact native map/vector arguments; terminates after progress; repeated/non-NSD calls inert')
call(exports['DriTestOwnershipReset']);assert all(v==1 for v in refs.values()),[(records[k]['name'],v)for k,v in refs.items()if v!=1]
assert call(exports['DriTestQualityFault'])==0
(OUTPUT/'test26_nsd_results.json').write_text(json.dumps({'checks':checks,'live_game_verified':False},indent=2)+'\n')
print(json.dumps(checks,indent=2))
