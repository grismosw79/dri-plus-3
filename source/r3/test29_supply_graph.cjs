'use strict';
// Read-only traversal of the actual external GeneratorRootData recipe. Field
// names identify decoded FTC records; native field-index consumers are proved
// separately in test29_supply_native.py. No Player record is changed here.
const assert=require('assert/strict');
const POS=['QB','HB','FB','WR','TE','LT','LG','C','RG','RT','LE','RE','DT','LOLB','MLB','ROLB','CB','FS','SS','K','P','LS'];
const num=v=>typeof v==='string'?parseInt(v,2):v;
async function inspect(file,api){
 const f=await api.create(file,{gameYearOverride:27,gameTypeOverride:'college',autoUnempty:false,saveOnChange:false});
 const cache=new Map();
 async function ref(v){v=num(v);if(!v)return null;if(cache.has(v))return cache.get(v);const t=f.getTableById(v>>>17);assert(t,'Missing generator reference '+v);await t.readRecords();const r=t.records[v&131071];assert(r&&!r.isEmpty);const x={t,r,ref:v};cache.set(v,x);return x;}
 async function array(v){const x=await ref(v);if(!x)return [];assert(x.t.header.isArray);const n=x.t.data.readUInt32BE(x.t.header.headerSize+x.r.index*4);assert(n<=x.r.fieldsArray.length);return Promise.all(x.r.fieldsArray.slice(0,n).map(y=>ref(x.r[y.key])));}
 async function sub(g,name){const a=(await array(g.r.SubGenerators)).filter(Boolean).filter(x=>x.r.Name===name);assert.equal(a.length,1,'Ambiguous '+name);const s=await ref(a[0].r.Setter);assert.equal(s.t.name,'RandomGeneratorProperty');return {property:a[0],setter:s,weights:await array(s.r.Generators)};}
 async function position(g,optional=false){const props=(await array(g.r.Properties)).filter(Boolean).filter(x=>x.r.Name==='Position');if(optional&&!props.length)return null;assert.equal(props.length,1,'Position must be explicit on the selected native PlayerTypeGen');const setter=await ref(props[0].r.Setter);let p={};if(setter.t.name==='FixedEnumValue')p[POS[setter.r.Value]]=1;else{assert.equal(setter.t.name,'RandomEnumPropertySetter');const ws=await array(setter.r.EnumValues);const total=ws.reduce((s,x)=>s+x.r.Weight,0);assert(total>0);for(const w of ws){const i=Number(w.r.Name);assert(i>=0&&i<22);p[POS[i]]=(p[POS[i]]||0)+w.r.Weight/total;}}assert(!Object.hasOwn(p,'undefined'));return {distribution:p,setter,property:props[0]};}
 const rootTable=f.getTableByUniqueId(4149836600);assert(rootTable&&rootTable.name==='GeneratorRootData');await rootTable.readRecords();const root=rootTable.records[0];
 const entries=await array(root.GeneratorDataList);assert.equal(entries.length,5);const generators=await Promise.all(entries.map(x=>ref(x.r.Generator)));assert(generators.every(x=>x.ref===generators[0].ref),'Multiple distinct class recipes require explicit modeling');
 const prospect=generators[0];assert.equal(prospect.r.TargetAssetDataType,'Player');
 const pipeline=await sub(prospect,'Pipeline');assert.equal(pipeline.weights.length,42);
 const pipelineTotal=pipeline.weights.reduce((s,x)=>s+x.r.Weight,0);assert(pipelineTotal>0);
 const distribution=Object.fromEntries(POS.map(p=>[p,0])),joint=Object.fromEntries(POS.map(p=>[p,[0,0,0,0,0]])),branches=[];
 for(const w of pipeline.weights){
  const g=await ref(w.r.Generator);const types=await sub(g,'PlayerTypeGen');const total=types.weights.reduce((s,x)=>s+x.r.Weight,0);assert(total>0);
  const local=Object.fromEntries(POS.map(p=>[p,0])),children=[];
  for(const tw of types.weights){assert(tw.r.Weight>=0);const tg=await ref(tw.r.Generator);assert.equal(tg.r.TargetAssetDataType,'Player');const p=await position(tg);
   const stars=await sub(tg,'StarRating'),starTotal=stars.weights.reduce((s,x)=>s+x.r.Weight,0),starChildren=[];assert(starTotal>0);
   for(const sw of stars.weights){const sg=await ref(sw.r.Generator);const st=(await array(sg.r.Properties)).filter(Boolean).find(x=>x.r.Name==='ProspectStarRating');assert(st);const sv=await ref(st.r.Setter);assert.equal(sv.t.name,'FixedEnumValue');const tier=sv.r.Value+1;assert(tier>=1&&tier<=5);const override=await position(sg,true);const effective=override||p;for(const [k,prob]of Object.entries(effective.distribution)){const pr=tw.r.Weight/total*sw.r.Weight/starTotal*prob;local[k]+=pr;distribution[k]+=w.r.Weight/pipelineTotal*pr;joint[k][tier-1]+=w.r.Weight/pipelineTotal*pr;}starChildren.push({weight:sw,generator:sg,tier,position:effective,override:!!override});}
   children.push({weight:tw,generator:tg,position:p,stars,starTotal,starChildren});
  }
  branches.push({pipeline:w,generator:g,types,weightTotal:total,distribution:local,children});
 }
 assert(Math.abs(Object.values(distribution).reduce((a,b)=>a+b,0)-1)<1e-10);
 return {f,root,entries,prospect,pipeline,pipelineTotal,distribution,joint,branches,ref,array};
}
function summary(m){return {rootTableUID:4149836600,recipeEntries:m.entries.length,prospectReference:m.prospect.ref,pipelineCount:m.branches.length,pipelineWeightTotal:m.pipelineTotal,starExpected:[0,1,2,3,4].map(s=>POS.reduce((v,p)=>v+m.joint[p][s]*3200,0)),jointExpectedAt3200:Object.fromEntries(POS.map(p=>[p,m.joint[p].map(v=>v*3200)])),expectedAt3200:Object.fromEntries(Object.entries(m.distribution).map(([p,x])=>[p,x*3200])),pipelines:m.branches.map(b=>({pipelineRow:b.pipeline.r.index,pipelineWeight:b.pipeline.r.Weight,typeWeightTotal:b.weightTotal,expectedPositionShare:b.distribution,typeGenerators:b.children.map(x=>({tableUID:x.weight.t.header.uniqueId,tableId:x.weight.t.header.tableId,row:x.weight.r.index,weight:x.weight.r.Weight,generatorReference:x.generator.ref,positionDistribution:x.position.distribution,positionSetterReference:x.position.setter.ref}))}))};}
module.exports={inspect,summary,POS};
if(require.main===module){const [file,parser,out]=process.argv.slice(2);assert(file&&parser&&out,'usage: node test29_supply_graph.cjs GENERATOR_FTC PARSER_CJS OUTPUT_JSON');inspect(file,require(require('path').resolve(parser))).then(m=>{require('fs').writeFileSync(out,JSON.stringify(summary(m),null,2)+'\n');console.log(JSON.stringify({file,expectedAt3200:summary(m).expectedAt3200}));}).catch(e=>{console.error(e.stack);process.exit(1);});}
