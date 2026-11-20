"""Captured native generator graph expansion order; SDF/property dispatch modeled.

This executes 081219a0's parent/property/child recursion. The enum draw itself
and final player creation are separate checks; no full live generation claim.
"""
from pathlib import Path
exec(compile((Path(__file__).parent/'runtime_fixture_test.py').read_text().split('prefix=bytes.fromhex')[0],__file__,'exec'))
import hashlib
start,end=0x081219a0,0x08121c97
code=raw[start-0x1000:end-0x1000]
uc.mem_map(GAME+start,len(code));uc.mem_write(GAME+start,code)
events=[];result={}
def property_dispatch():
 p=rq(reg('R8'));name=records[p]['name'];events.append(name)
 result[name.split(':')[0]]=records[p]['scalar'][0]
 return 0
def selected_child():
 _,out,index,arrayptr=args();a=rq(arrayptr);child=records[a]['array'][index]
 incref(child);wq(out,child);return out
api(property_dispatch,GAME+0x08121740)
api(selected_child,GAME+0x08123540)
empty=record('empty',array=[])
def node(name,properties,parent=0,children=()):
 return record(name,ref={0:parent,2:record(name+' props',array=properties),3:record(name+' branches',array=list(children))})
def prop(name,value):return record(name,scalar={0:value})
parent=node('shared parent',[prop('Position:shared',5),prop('Stars:parent',3)])
low=node('nonelite leaf',[prop('Position:private',9),prop('Stars:low',1)])
elite=node('elite leaf',[prop('Stars:elite',4)])
def run(child):
 events.clear();result.clear();top=node('recipe',[],parent,[child]);ptr=alloc(8);wq(ptr,top)
 before=dict(refs);rsp=STACK+0xf0000-8;wi(rsp+0x28,0);wq(rsp+0x30,0)
 call(GAME+start,0,0,ptr,0)
 assert all(refs[k]==v for k,v in before.items()),'Native expansion leaked a modeled record reference'
 return list(events),dict(result)
a,av=run(low);assert a==['Position:shared','Stars:parent','Position:private','Stars:low'] and av=={'Position':9,'Stars':1},(a,av)
b,bv=run(elite);assert b==['Position:shared','Stars:parent','Stars:elite'] and bv=={'Position':5,'Stars':4},(b,bv)
r={'passed':True,'native_rva':hex(start),'native_code_sha256':hashlib.sha256(code).hexdigest(),'nonelite_order':a,'elite_order':b,'private_override_wins':True,'elite_sibling_unchanged':True,'record_references_balanced':True,'modeled_services':['SDF records','weighted child selection','property setter value application'],'scope':'Actual captured recursion order; not full generation or a random draw distribution test','live_game_verified':False}
(OUTPUT/'test31_generator_order_results.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r))
