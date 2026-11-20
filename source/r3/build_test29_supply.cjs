'use strict';
// node build_test29_supply.cjs GENERATOR_BASELINE_DIR TEST29_ROOT [--verify]
// DRI_SAVE_PARSER points to the retained CFB27 save_adapter.cjs. Baseline hashes
// are accepted HS Test02, also verified byte-identical in Recruiting Test01.
const fs=require('fs'),path=require('path'),zlib=require('zlib'),assert=require('assert/strict'),crypto=require('crypto');
const G=require('./test29_supply_graph.cjs');
const [baseArg,outArg,mode]=process.argv.slice(2);assert(baseArg&&outArg&&(!mode||mode==='--verify'));
assert(process.env.DRI_SAVE_PARSER,'Set DRI_SAVE_PARSER to the authoritative adapter');
const api=require(path.resolve(process.env.DRI_SAVE_PARSER)),base=path.resolve(baseArg),out=path.resolve(outArg),verify=mode==='--verify';
const HASH=['ce997d4b5c96aa084cbd5c37b496d366a589fdbda6391df53cb2a3c6d7ec96d0','dc07230df02dfb18a4ad313bcbc50b9f323d20444dd0bdcdb818a251db8b9c10','8beedf7d84adef8b23464b6fe3259dc0e7241daf8f593e514482fdcb3f976e33','056db323197419f4a0607a376dc3eeadcc1157dbdfbc2b5c5fa530e0dd720551','ce997d4b5c96aa084cbd5c37b496d366a589fdbda6391df53cb2a3c6d7ec96d0'];
const hash=b=>crypto.createHash('sha256').update(b).digest('hex');
const key=x=>`${x.t.header.uniqueId}:${x.r.index}:Weight`;
const only=(x,p)=>Object.keys(x.position.distribution).length===1&&x.position.distribution[p]===1;
function apportion(amount,weights){const total=weights.reduce((a,b)=>a+b,0);assert(total>0);const ideal=weights.map(w=>w*amount/total),a=ideal.map(Math.floor);let left=amount-a.reduce((a,b)=>a+b,0);const order=ideal.map((x,i)=>[x-a[i],i]).sort((a,b)=>b[0]-a[0]||a[1]-b[1]);for(let j=0;j<left;j++)a[order[j][1]]++;assert.equal(a.reduce((a,b)=>a+b,0),amount);return a;}
function current(m,override=new Map()){const weight=x=>override.has(x.ref)?override.get(x.ref):x.r.Weight;const joint=Object.fromEntries(G.POS.map(p=>[p,[0,0,0,0,0]]));for(const b of m.branches){const bt=b.children.reduce((s,x)=>s+x.weight.r.Weight,0);for(const c of b.children){const st=c.starChildren.reduce((s,x)=>s+weight(x.weight),0);for(const s of c.starChildren)for(const [p,pp]of Object.entries(s.position.distribution))joint[p][s.tier-1]+=3200*b.pipeline.r.Weight/m.pipelineTotal*c.weight.r.Weight/bt*weight(s.weight)/st*pp;}}
return {joint,positions:Object.fromEntries(G.POS.map(p=>[p,joint[p].reduce((a,b)=>a+b,0)])),stars:[0,1,2,3,4].map(i=>G.POS.reduce((s,p)=>s+joint[p][i],0))};}
async function routeBuild(route){
 const src=path.join(base,String(route),'dynasty-generatordata-binary.FTC'),dest=path.join(out,'frantkdata',String(route),'dynasty-generatordata-binary.FTC');
 assert.notEqual(src,dest);const source=fs.readFileSync(src);assert.equal(hash(source),HASH[route],'Wrong generator baseline');
 const m=await G.inspect(src,api),eliteRefs=new Set(m.branches.flatMap(b=>b.children.flatMap(c=>c.starChildren.filter(s=>s.tier>=4).map(s=>s.weight.ref)))),before=current(m),original=Buffer.from(m.f.unpackedFileContents),raw=Buffer.from(original),assets=JSON.stringify(m.f.assetTable),edits=new Map(),lowRows=new Map();
 function set(x,value,reason){assert(Number.isInteger(value)&&value>=0&&value<=1000);const k=key(x),e=edits.get(k);if(!e)edits.set(k,{x,before:x.r.Weight,after:value,reason});else{assert.equal(e.reason,reason);e.after=value;}x.r.Weight=value;}
 // Shared rows can be referenced by several native selectors. Capture each
 // unique low-tier row once and require that its users agree on the position.
 for(const b of m.branches)for(const c of b.children)for(const st of c.starChildren)if(st.tier===2||st.tier===3){const pos=Object.keys(c.position.distribution).join(',');const old=lowRows.get(st.weight.ref);if(old)assert.equal(old.position,pos,'Shared star row crosses position families');else lowRows.set(st.weight.ref,{x:st.weight,before:st.weight.r.Weight,tier:st.tier,position:pos});}
 // Keep every pipeline's total type weight exactly fixed. Every untouched
 // position therefore has exactly its prior selection probability. Additional
 // punter draws are paid for only by the existing WR/CB nonelite selections.
 for(const b of m.branches){
  const ps=b.children.filter(x=>only(x,'P')),wr=b.children.filter(x=>only(x,'WR')),cb=b.children.filter(x=>only(x,'CB'));
  assert(ps.length===2&&wr.length&&cb.length);
  const oldP=ps.reduce((s,x)=>s+x.weight.r.Weight,0),delta=Math.round(oldP*(95/before.positions.P-1));assert(delta>0);
  const add=apportion(delta,ps.map(x=>x.weight.r.Weight));
  for(let i=0;i<ps.length;i++){const c=ps[i],old=c.weight.r.Weight;set(c.weight,old+add[i],'punter_type_supply');}
  for(const [name,children,amount]of [['WR',wr,Math.floor(delta/2)],['CB',cb,delta-Math.floor(delta/2)]]){
   const shares=children.map(c=>c.weight.r.Weight*c.starChildren.filter(s=>s.tier<=3).reduce((v,s)=>v+s.weight.r.Weight,0)/c.starTotal),remove=apportion(amount,shares);
   for(let i=0;i<children.length;i++)if(remove[i]){
    const c=children[i],old=c.weight.r.Weight,next=old-remove[i];assert(next>0);set(c.weight,next,'nonelite_'+name+'_funds_punter_supply');

   }
  }
  assert.equal(b.children.reduce((s,x)=>s+x.weight.r.Weight,0),b.weightTotal);
 }
 // Fit donor low-tier denominators against each position's original elite
 // probabilities. Reference-shared rows receive one consistent edit. The elite
 // selector fields themselves remain byte-identical.
 for(const position of ['WR','CB']){
  const rows=[...lowRows.values()].filter(r=>r.position===position&&r.before>0),target=before.joint[position].slice(3),factors=[.97,.97];
  function predict(f){const values=new Map(rows.map(r=>[r.x.ref,Math.min(1000,r.before*f[r.tier-2])]));return current(m,values).joint[position].slice(3);}
  for(let i=0;i<60;i++){const v=predict(factors),e=v.map((x,j)=>x-target[j]);if(Math.max(...e.map(Math.abs))<1e-10)break;const eps=1e-5,cols=[0,1].map(k=>{const f=factors.slice();f[k]+=eps;return predict(f).map((x,j)=>(x-v[j])/eps);});const [a,c]=cols[0],[b,d]=cols[1],det=a*d-b*c;assert(Math.abs(det)>1e-10);const step=[(d*e[0]-b*e[1])/det,(-c*e[0]+a*e[1])/det];for(let k=0;k<2;k++)factors[k]-=Math.max(-.1,Math.min(.1,step[k]));}
  assert(factors.every(f=>f>0&&f<=2),'Donor solution unexpectedly expands a low-tier weight '+factors);
  for(const r of rows)set(r.x,Math.min(1000,Math.round(r.before*factors[r.tier-2])),'preserve_'+position+'_elite_probability_low_tiers');
  const contexts=[],owners=new Map();
  for(const b of m.branches)for(const c of b.children)if(only(c,position)){const ctx={total:c.starChildren.reduce((v,z)=>v+z.weight.r.Weight,0),mass:3200*b.pipeline.r.Weight/m.pipelineTotal*c.weight.r.Weight/b.weightTotal,elite:[4,5].map(t=>c.starChildren.filter(z=>z.tier===t).reduce((v,z)=>v+z.weight.r.Weight,0))};contexts.push(ctx);for(const z of c.starChildren.filter(z=>z.tier===2||z.tier===3)){if(!owners.has(z.weight.ref))owners.set(z.weight.ref,[]);owners.get(z.weight.ref).push(ctx);}}
  let err=current(m).joint[position].slice(3).map((v,j)=>v-target[j]);
  const starting=new Map(rows.map(r=>[r.x.ref,r.x.r.Weight]));
  for(let it=0;it<5000&&Math.max(...err.map(Math.abs))>.0002;it++){
   const cost=err[0]**2+16*err[1]**2;let best=null;
   for(const r of rows)for(const change of [-1,1]){const value=r.x.r.Weight+change;if(value<1||value>1000||Math.abs(value-starting.get(r.x.ref))>8)continue;const delta=[0,0];for(const ctx of owners.get(r.x.ref))for(let k=0;k<2;k++)delta[k]+=ctx.mass*ctx.elite[k]*(1/(ctx.total+change)-1/ctx.total);const e=err.map((v,k)=>v+delta[k]),score=e[0]**2+16*e[1]**2;if(score<cost-1e-18&&(!best||score<best.score))best={r,change,value,e,score};}
   if(!best){
    const unique=new Map();for(const r of rows)for(const change of [-1,1]){const value=r.x.r.Weight+change;if(value<1||value>1000||Math.abs(value-starting.get(r.x.ref))>16)continue;const delta=[0,0];for(const ctx of owners.get(r.x.ref))for(let k=0;k<2;k++)delta[k]+=ctx.mass*ctx.elite[k]*(1/(ctx.total+change)-1/ctx.total);const sig=delta.map(x=>x.toPrecision(10)).join(',');if(!unique.has(sig))unique.set(sig,{r,change,value,delta});}
    const candidates=[...unique.values()];let pair=null;for(let i=0;i<candidates.length;i++)for(let j=i+1;j<candidates.length;j++){const a=candidates[i],b=candidates[j];if(a.r.x.ref===b.r.x.ref)continue;const e=err.map((v,k)=>v+a.delta[k]+b.delta[k]),score=e[0]**2+16*e[1]**2;if(score<cost-1e-16&&(!pair||score<pair.score))pair={a,b,score};}
    if(!pair)break;for(const item of [pair.a,pair.b]){set(item.r.x,item.value,'preserve_'+position+'_elite_probability_low_tiers');for(const ctx of owners.get(item.r.x.ref))ctx.total+=item.change;}err=current(m).joint[position].slice(3).map((v,j)=>v-target[j]);continue;
   }
   set(best.r.x,best.value,'preserve_'+position+'_elite_probability_low_tiers');for(const ctx of owners.get(best.r.x.ref))ctx.total+=best.change;err=best.e;

  }
  assert(Math.max(...err.map(Math.abs))<.003,'Elite quantization did not converge '+position+':'+err);

 }
 // Preserve the global two/three-star mix using the existing punter recipes.
 // Common tier factors retain original ratios among recipe rows before integer
 // rounding; every one-star and specialist elite zero remains untouched.
 const pRows=[...lowRows.values()].filter(r=>r.position==='P'&&r.before>0);
 const pTarget=before.stars[1]-(current(m).stars[1]-current(m).joint.P[1]);
 function pmix(factor,apply){const scale=Math.min(.8,1000/Math.max(...pRows.map(r=>r.before*(r.tier===2?factor:1)))),vals=new Map(pRows.map(r=>[r.x.ref,Math.max(1,Math.round(r.before*scale*(r.tier===2?factor:1)))]));if(apply)for(const r of pRows)set(r.x,vals.get(r.x.ref),'preserve_global_low_tier_mix_punter');return current(m,vals).joint.P[1];}
 let lo=.01,hi=100;for(let i=0;i<60;i++){const mid=(lo+hi)/2;if(pmix(mid,false)<pTarget)lo=mid;else hi=mid;}pmix((lo+hi)/2,true);
 const po=new Map();for(const b of m.branches)for(const c of b.children)if(only(c,'P')){const ctx={total:c.starChildren.reduce((v,s)=>v+s.weight.r.Weight,0),two:c.starChildren.filter(s=>s.tier===2).reduce((v,s)=>v+s.weight.r.Weight,0),mass:3200*b.pipeline.r.Weight/m.pipelineTotal*c.weight.r.Weight/b.weightTotal};for(const st of c.starChildren.filter(s=>s.tier===2||s.tier===3)){if(!po.has(st.weight.ref))po.set(st.weight.ref,[]);po.get(st.weight.ref).push(ctx);}}
 let pError=current(m).stars[1]-before.stars[1];for(let i=0;i<500&&Math.abs(pError)>.0002;i++){let best=null;for(const r of pRows)for(const change of [-1,1]){const value=r.x.r.Weight+change;if(value<1||value>1000)continue;let delta=0;for(const ctx of po.get(r.x.ref))delta+=ctx.mass*((ctx.two+(r.tier===2?change:0))/(ctx.total+change)-ctx.two/ctx.total);const e=pError+delta;if(Math.abs(e)<Math.abs(pError)&&(!best||Math.abs(e)<Math.abs(best.e)))best={r,change,value,e};}if(!best)break;set(best.r.x,best.value,'preserve_global_low_tier_mix_punter');for(const ctx of po.get(best.r.x.ref)){ctx.total+=best.change;if(best.r.tier===2)ctx.two+=best.change;}pError=best.e;}


 const after=current(m);assert(Math.abs(after.positions.P-95)<.15);
 for(const p of G.POS)if(!['P','WR','CB'].includes(p))assert(Math.abs(after.positions[p]-before.positions[p])<1e-9,'Unrelated position changed '+p);
 assert(Math.abs(after.positions.WR-before.positions.WR+10)<.15);assert(Math.abs(after.positions.CB-before.positions.CB+10)<.15);
 for(let i=0;i<5;i++)assert(Math.abs(after.stars[i]-before.stars[i])<.01,'Protected star total moved '+JSON.stringify({before:before.stars,after:after.stars}));
 assert(after.stars[0]===0&&after.joint.P[3]===0&&after.joint.P[4]===0);assert(Math.abs(after.stars[4]-38)<.02);
 const changes=[];const mask=Buffer.alloc(raw.length);
 for(const e of edits.values())if(e.before!==e.after){const {x}=e,o=x.r.fields.Weight.offset;assert(e.before>0&&e.after>0,'Zero restrictions must stay zero');assert(!eliteRefs.has(x.ref),'Elite selector field changed');if(lowRows.has(x.ref))assert(['P','WR','CB'].includes(lowRows.get(x.ref).position),'Unrelated star selector changed');assert(x.t.name==='WeightedGenerator'&&x.t.header.uniqueId===3382428657);assert(!o.valueInSecondTable&&!o.valueInThirdTable&&o.length<=32);const start=8*(x.t.offset+x.t.header.table1StartIndex+x.r.index*x.t.header.record1Size)+o.offset;let encoded=0;for(let j=0;j<o.length;j++)encoded=encoded*2+((original[(start+j)>>3]>>(7-((start+j)&7)))&1);const bias=encoded-e.before;assert(bias===0||bias===2147483648||bias===-o.minValue);const value=e.after+bias;assert(value>=0&&value<2**o.length);for(let j=0;j<o.length;j++){const bit=start+j,b=bit>>3,mk=1<<(7-(bit&7));mask[b]|=mk;raw[b]=(raw[b]&~mk)|((Math.floor(value/2**(o.length-1-j))%2)?mk:0);}changes.push({table_uid:x.t.header.uniqueId,table_id:x.t.header.tableId,table:x.t.name,row:x.r.index,field:'Weight',before:e.before,after:e.after,reason:e.reason,bit_offset:start,bit_length:o.length,encoding_bias:bias});}
 for(let i=0;i<raw.length;i++)assert.equal((raw[i]^original[i])&~mask[i],0);
 if(!verify){fs.mkdirSync(path.dirname(dest),{recursive:true});fs.writeFileSync(dest,zlib.deflateSync(raw,{level:6}));}
 const actual=fs.readFileSync(dest);assert(zlib.inflateSync(actual).equals(raw),'Output is not baseline plus exact approved fields');
 const v=await G.inspect(dest,api);assert.deepEqual(current(v),after);assert.equal(JSON.stringify(v.f.assetTable),assets);assert.equal(v.f.tables.length,m.f.tables.length);
 for(let i=0;i<m.f.tables.length;i++){assert.deepEqual(v.f.tables[i].header,m.f.tables[i].header);assert.equal(v.f.tables[i].offset,m.f.tables[i].offset);assert.deepEqual(v.f.tables[i].offsetTable,m.f.tables[i].offsetTable);}
 const r={route,path:`frantkdata/${route}/dynasty-generatordata-binary.FTC`,baseline_path:`Runtime/Portal/Audit/Test29/Inputs/GeneratorBaseline/${route}/dynasty-generatordata-binary.FTC`,before_sha256:hash(source),after_sha256:hash(actual),before_unpacked_sha256:hash(original),after_unpacked_sha256:hash(raw),only_approved_field_bits_changed:true,assets_unchanged:true,table_layouts_unchanged:true,all_non_weight_recipe_bytes_unchanged:true,pipeline_type_weight_totals_unchanged:true,elite_weight_fields_unchanged:true,forbidden_zeroes_unchanged:true,field_edits:changes,before_model:before,after_model:after,live_game_verified:false};
 console.log(JSON.stringify({route,changes:changes.length,positions:{P:after.positions.P,WR:after.positions.WR,CB:after.positions.CB},stars:after.stars,sha256:r.after_sha256}));return r;
}
(async()=>{const routes=[];for(const r of (process.env.DRI_SUPPLY_ROUTE?[Number(process.env.DRI_SUPPLY_ROUTE)]:[0,1,2,3,4]))routes.push(await routeBuild(r));const result={release:'Test29',status:'PASS_OFFLINE_LIVE_PENDING',live_game_verified:false,accepted_baseline:'DRI-3.0-HS-Test02; byte-identical in Recruiting-Test01',pool_size:3200,requested_expected_positions:{P:95,WR:365,CB:295},scope:'Punter supply funded by nonelite WR/CB selections. Native player recipes remain byte-identical. All other position expectations are unchanged. Native integer rounding keeps every global tier within0.01 recruit of its accepted baseline.',source_provenance:'Runtime/Portal/Audit/Test29/Inputs/GeneratorBaseline/provenance.json',routes};if(!verify&&routes.length===5)fs.writeFileSync(path.join(out,'Runtime/Portal/Audit/Test29/generator_changes.json'),JSON.stringify(result,null,2)+'\n');})().catch(e=>{console.error(e.stack);process.exit(1);});
