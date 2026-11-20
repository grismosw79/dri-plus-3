"""Published incoming HS players may retain soft stage in native funding vectors."""
exec(compile((__import__('pathlib').Path(__file__).parent/'portal_runtime_test.py').read_text(),'portal_runtime_test.py','exec'))
configure(1);call(exports['DriTestOwnershipReset']);call(exports['DriTestHsPlanReset'])
records[season]['scalar'].update({2:2040,4:8,5:1})
for x in [departures,roster,hslist,tplist,committed,arr]:records[x]['array']=[]
for pos,e in enumerate(ideal_entries):records[e]['scalar'].update({2:ideals[pos],4:1})
records[ideal_entries[0]]['scalar'][2]=4
for pos in range(22):
 count=2 if pos==0 else ideals[pos]
 records[roster]['array'] += [pp('returner '+str(pos)+' '+str(j),65,pos=pos)for j in range(count)]
primary,pt,ppp=pr(63901,79,year=0,pos=0,stage=1,portal=False)
acquired,at,ap=pr(63902,80,year=0,pos=0,stage=4,portal=False)
backup,bt,bpp=pr(63903,73,year=0,pos=0,stage=1,portal=False)
for r,t,p in [(primary,pt,ppp),(acquired,at,ap),(backup,bt,bpp)]:
 records[p]['scalar'][231]=3;valid_offer(r,t)
records[committed]['array']=[ap]
assert call(exports['DriTestHsPlanRole'],acquired,team,context,0)==0
assert call(exports['DriTestHsPlanRole'],primary,team,context,0)==1
assert call(exports['DriTestHsPlanRole'],backup,team,context,0)==2
ff=STACK+0x55000;fb=ff+0x400;wq(ff+10*8,fb);wq(ff+9*8,context)
tr=alloc(8);wq(tr,team);wq(fb-0x60,tr)
vec=alloc(216);ind=alloc(24);wq(fb-0x50,vec);wq(fb-0x48,vec+216);wq(fb+0x58,ind);wq(fb+0x60,ind+24)
for j,(r,t)in enumerate([(primary,pt),(acquired,at),(backup,bt)]):
 wq(vec+j*72,r);wi(vec+j*72+40,300 if j!=1 else 777);wq(vec+j*72+48,t);wi(vec+j*72+64,1);wq(ind+j*8,2-j)
secured_bytes=bytes(uc.mem_read(vec+72,72));player_before=dict(records[ap]['scalar']);recruit_before=dict(records[acquired]['scalar'])
call(hooks[23],ff)
assert call(exports['DriTestQualityFault'])==0
assert bytes(uc.mem_read(vec+72,72))==secured_bytes
assert [rq(ind+j*8)for j in range(3)]==[0,1,2]
assert (ri(vec+40),ri(vec+184))==(400,200) and ri(vec+112)==777
assert records[committed]['array']==[ap] and records[ap]['scalar']==player_before and records[acquired]['scalar']==recruit_before
# The acquired-only native vector also passes through completely unchanged.
wq(fb-0x50,vec+72);wq(fb-0x48,vec+144);wq(fb+0x58,ind);wq(fb+0x60,ind+8);wq(ind,0)
call(hooks[23],ff)
assert call(exports['DriTestQualityFault'])==0 and bytes(uc.mem_read(vec+72,72))==secured_bytes and rq(ind)==0
call(exports['DriTestOwnershipReset']);call(exports['DriTestHsPlanReset'])
assert all(v==1 for v in refs.values())
checks=['Published soft-stage incoming player excluded from pair planning and funding redistribution without identity fault','Acquired candidate bytes and physical index position preserved; active primary/backup native sum conserved at 400:200','Acquired-only native funding vector remains byte-identical; ownership, stage, player and offer preserved; references balanced']
(OUTPUT/'test28r8_funding_published_results.json').write_text(json.dumps({'checks':checks,'live_game_verified':False},indent=2)+'\n');print(json.dumps(checks,indent=2))
