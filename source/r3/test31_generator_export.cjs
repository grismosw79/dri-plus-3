'use strict';
const fs=require('fs'),path=require('path'),crypto=require('crypto'),assert=require('assert/strict');
const G=require('./test29_supply_graph.cjs');
const [base,out]=process.argv.slice(2);assert(base&&out);const api=require(path.resolve(process.env.DRI_SAVE_PARSER));
const hash=b=>crypto.createHash('sha256').update(b).digest('hex');
(async()=>{fs.mkdirSync(out,{recursive:true});for(let route=0;route<5;route++){
 const fn=path.join(base,'frantkdata',String(route),'dynasty-generatordata-binary.FTC'),m=await G.inspect(fn,api),branches=[];
 for(const b of m.branches){const children=[];for(const c of b.children){const props=await m.array(c.generator.r.Properties),pt=props.find(x=>x.r.Name==='PlayerType');assert(pt);const setter=await m.ref(pt.r.Setter);assert.equal(setter.t.name,'FixedEnumValue');
 children.push({ref:c.weight.ref,weight:c.weight.r.Weight,gen:c.generator.ref,archetype:setter.r.Value,total:c.starTotal,position:c.position.distribution,stars:c.starChildren.map(s=>({ref:s.weight.ref,weight:s.weight.r.Weight,tier:s.tier,gen:s.generator.ref,position:s.position.distribution}))});}
 branches.push({pipelineRef:b.pipeline.ref,weight:b.pipeline.r.Weight,total:b.weightTotal,children});}
 const graph={route,baseline_sha256:hash(fs.readFileSync(fn)),pipelineTotal:m.pipelineTotal,branches};
 fs.writeFileSync(path.join(out,`route${route}_graph.json`),JSON.stringify(graph));console.log(JSON.stringify({route,sha256:graph.baseline_sha256,branches:branches.length}));
}})().catch(e=>{console.error(e.stack);process.exit(1)});
