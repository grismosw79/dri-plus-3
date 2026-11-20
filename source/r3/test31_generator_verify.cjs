'use strict';
// Independent reparse of delivered FTC bytes against the guarded baseline graph.
const fs=require('fs'),path=require('path'),assert=require('assert/strict'),crypto=require('crypto');
const G=require('./test29_supply_graph.cjs'),api=require(path.resolve(process.env.DRI_SAVE_PARSER));
const [base,out,routeArg,reportArg]=process.argv.slice(2),reportDir=reportArg||process.env.DRI_GENERATOR_REPORT_DIR||path.join(out,'Runtime/Portal/Audit/Test31/Generator'),routes=routeArg===undefined?[0,1,2,3,4]:[Number(routeArg)];
const hash=b=>crypto.createHash('sha256').update(b).digest('hex');
const target=Object.fromEntries(G.POS.map((p,i)=>[p,[165,205,5,400,210,104,104,104,104,104,135,135,220,50,135,135,275,135,135,70,70,0][i]]));
(async()=>{fs.mkdirSync(reportDir,{recursive:true});const reports=[];for(const route of routes){
 const dir=path.join(out,'Runtime/Portal/Audit/Test31/Generator'),old=JSON.parse(fs.readFileSync(path.join(dir,`route${route}_graph.json`))),sol=JSON.parse(fs.readFileSync(path.join(dir,`route${route}_solution.json`))),build=JSON.parse(fs.readFileSync(path.join(dir,`route${route}_generator_build.json`))),file=path.join(out,`frantkdata/${route}/dynasty-generatordata-binary.FTC`);assert.equal(hash(fs.readFileSync(path.join(base,`frantkdata/${route}/dynasty-generatordata-binary.FTC`))),old.baseline_sha256);assert.equal(build.sha256,hash(fs.readFileSync(file)));const m=await G.inspect(file,api);
 const leafOld=new Map(),leafNew=new Map(),paths=[],zeroes=[],recipeRefs=new Set(),noneliteOverrides=new Set();let largestWeight=0;
 const add=(map,key,n)=>map.set(key,(map.get(key)||0)+n);
 for(let bi=0;bi<old.branches.length;bi++){
  const b=old.branches[bi],v=m.branches[bi];assert.equal(v.pipeline.ref,b.pipelineRef);assert.equal(v.children.length,b.children.length);
  for(let ci=0;ci<b.children.length;ci++){
   const c=b.children[ci],vc=v.children[ci];assert.equal(vc.generator.ref,c.gen);assert.equal(vc.weight.ref,c.ref);assert.equal(c.stars.length,vc.starChildren.length);const props=await m.array(vc.generator.r.Properties),pt=props.find(p=>p.r.Name==='PlayerType');assert(pt);const setter=await m.ref(pt.r.Setter);assert.equal(setter.r.Value,c.archetype);largestWeight=Math.max(largestWeight,vc.weight.r.Weight);
   if(c.weight===0){assert.equal(vc.weight.r.Weight,0);zeroes.push(c.ref);}
   for(let si=0;si<c.stars.length;si++){
    const s=c.stars[si],vs=vc.starChildren[si];assert.equal(vs.generator.ref,s.gen);assert.equal(vs.tier,s.tier);assert.equal(vs.weight.ref,s.ref);largestWeight=Math.max(largestWeight,vs.weight.r.Weight);recipeRefs.add(s.gen);if(s.weight===0){assert.equal(vs.weight.r.Weight,0);zeroes.push(s.ref);}
    const before=3200*b.weight/old.pipelineTotal*c.weight/b.total*s.weight/c.total,after=3000*v.pipeline.r.Weight/m.pipelineTotal*vc.weight.r.Weight/v.weightTotal*vs.weight.r.Weight/vc.starTotal;
    if(s.tier>=4){assert.equal(vs.weight.r.Weight,s.weight,'Elite selector must not change');assert.deepEqual(vs.position.distribution,s.position);const key=[s.tier,s.gen,c.archetype].join(':');add(leafOld,key,before);add(leafNew,key,after);for(const [position,p]of Object.entries(s.position))paths.push({pipeline:bi,type_ref:c.gen,star:s.tier,recipe_ref:s.gen,archetype:c.archetype,position,before:before*p,after:after*p,error:(after-before)*p});}
    else if(JSON.stringify(vs.position.distribution)!==JSON.stringify(s.position)){assert([2,3].includes(s.tier));assert(['LT/RT','LOLB/ROLB'].includes(Object.keys(s.position).sort().join('/')));noneliteOverrides.add(s.gen);}
   }
  }
 }
 assert(largestWeight<=1000);assert.equal(noneliteOverrides.size,16);const leafErrors=[...leafOld].map(([key,before])=>({key,before,after:leafNew.get(key),error:leafNew.get(key)-before}));const maximumLeafError=Math.max(...leafErrors.map(x=>Math.abs(x.error))),maximumFullPathError=Math.max(...paths.map(x=>Math.abs(x.error)));assert(maximumLeafError<=.00401);assert(maximumFullPathError<=.01201);
 const positionErrors=G.POS.map(p=>({position:p,target:target[p],actual:m.distribution[p]*3000,error:m.distribution[p]*3000-target[p]}));assert(positionErrors.every(x=>Math.abs(x.error)<=.08001));assert.equal(m.distribution.LS,0);
 const stars=[1,2,3,4,5].map(t=>G.POS.reduce((n,p)=>n+m.joint[p][t-1]*3000,0));for(let t=1;t<=5;t++)assert(Math.abs(stars[t-1]-sol.target_stars[t])<=.02501);assert.equal(stars[0],0);
 const tuning=await api.create(path.join(out,`frantkdata/${route}/dynasty-tuning-binary.FTC`),{gameYearOverride:27,gameTypeOverride:'college',autoUnempty:false,saveOnChange:false});const tt=tuning.getTableByUniqueId(3880326652);await tt.readRecords();assert.equal(tt.records[0].RecruitsToGenerateMinimum,3000);assert.equal(tt.records[0].RecruitsToGenerateMaximum,3000);assert.equal(tt.records[0].MaxFiveStarsInClass,32);assert.equal(tt.records[0].RecruitsToGenerateGroupMax,20);
 const report={route,status:'PASS_STATIC_AND_EXACT_EXPECTATION_MODEL',pool:3000,positionErrors,stars,baselineStars:sol.baseline_stars,maximumFullElitePathAbsoluteError:maximumFullPathError,fullElitePathAbsoluteErrorSum:paths.reduce((n,x)=>n+Math.abs(x.error),0),maximumEliteRecipeArchetypeAbsoluteError:maximumLeafError,elite_recipe_archetype_joint:leafErrors,elite_full_path_joint:paths,structural_zero_paths_checked:zeroes.length,distinct_recipe_refs:recipeRefs.size,all_elite_selector_weights_unchanged:true,nonelite_position_override_leaf_count:16,maximum_selector_weight:largestWeight,native_final_five_star_cap:32,live_generation_verified:false};fs.writeFileSync(path.join(reportDir,`route${route}_verification.json`),JSON.stringify(report,null,2)+'\n');reports.push({route,maximumFullPathError,maximumLeafError,stars});console.log(JSON.stringify(reports.at(-1)));
 }})();
