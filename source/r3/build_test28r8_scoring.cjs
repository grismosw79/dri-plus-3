'use strict';
// Usage: node build_test28r8_scoring.cjs R7_PACKAGE R8_PACKAGE [--verify]
// The immutable R7 input is mandatory. --verify checks an existing R8 output
// without writing game files. The only approved edits are active HS scoring.
const fs=require('fs'),path=require('path'),assert=require('assert/strict'),crypto=require('crypto');
const api=require('../../../Audit/PreparedSaveEvidence/Parser/save_adapter.cjs');
const [baseArg,outArg,mode]=process.argv.slice(2);
assert(baseArg&&outArg&&(!mode||mode==='--verify'),'Supply R7 baseline and separate R8 output');
const base=path.resolve(baseArg),out=path.resolve(outArg),verifyOnly=mode==='--verify';
assert.notEqual(base,out,'The R7 baseline must remain immutable');
const sha=b=>crypto.createHash('sha256').update(b).digest('hex');
const num=x=>typeof x==='string'?parseInt(x,2):x;
const open=p=>api.create(p,{gameYearOverride:27,gameTypeOverride:'college',autoUnempty:false,saveOnChange:false});
const HS=[[-500,250,250,-500,-500],[-500,250,250,-500,-500],[-500,200,300,-500,-500],[-500,200,300,-500,-500],[-500,200,250,300,-500],[-500,-500,200,250,300],[-500,-500,200,300,300],[-500,-500,200,300,300],[-500,-500,175,300,325],[-500,-500,150,300,325],[-500,-500,150,300,325]];
const OLD_HS=[[40,450,500,-500,-500],[38,423,470,-500,-500],[-500,391,440,-500,-500],[-500,364,410,-500,-500],[-500,270,305,320,-500],[-500,-500,290,310,290],[-500,-500,275,305,320],[-500,-500,235,280,325],[-500,-500,205,265,305],[-500,-500,185,260,295],[-500,-500,175,275,315]];
const INTEREST=[100,90,80,70,60,50,40,30,20,10],OLD_INTEREST=[125,120,115,110,100,80,60,40,20,10];
async function ref(f,v){v=num(v);assert(v>0);const t=f.getTableById(v>>>17);assert(t,'Missing referenced table');await t.readRecords();const r=t.records[v&131071];assert(r&&!r.isEmpty,'Missing referenced row');return {t,r,ref:v};}
async function array(f,v){const a=await ref(f,v);assert(a.t.header.isArray);const count=a.t.data.readUInt32BE(a.t.header.headerSize+a.r.index*4);assert(count<=a.r.fieldsArray.length);return {...a,fields:a.r.fieldsArray.slice(0,count)};}
const values=a=>a.fields.map(x=>num(a.r[x.key]));
function key(r,name,modern,legacy){const k=r.fields[name]?name:'Field_'+(r.fieldsArray.length===11?modern:legacy);assert(r.fields[k],name+' absent');return k;}
function set(t,r,k,value,label,edits){const before=r[k];if(before===value)return;const field=r.fields[k];assert(!field.offset.valueInSecondTable&&!field.offset.valueInThirdTable);edits.push({table:t.header.tableId,tableName:t.name,row:r.index,key:k,label,before,after:value,bitOffset:field.offset.offset,bitLength:field.offset.length});r[k]=value;}
function location(t,r,k){return t.header.tableId+':'+r.index+':'+k;}
async function assertPrivateReferences(f,expected){
 const actual=new Map([...expected.keys()].map(v=>[v,[]]));
 // First-table reference/scalar fields cover both named schemas and the generic
 // modern schema. Avoid compressed string payloads unrelated to references.
 for(const t of f.tables){await t.readRecords();for(const r of t.records)if(!r.isEmpty)for(const fld of r.fieldsArray){
  const o=fld.offset;if(o.length!==32||o.valueInSecondTable||o.valueInThirdTable||(!o.isReference&&o.type!=='int'))continue;
  const v=num(r[fld.key]);if(actual.has(v))actual.get(v).push(location(t,r,fld.key));
 }}
 const result=[];for(const [v,want]of expected){assert.deepEqual(actual.get(v).sort(),want.slice().sort(),'HS reference aliases unrelated tuning: '+v);result.push({reference:v,incoming:actual.get(v)});}return result;
}
async function scoring(f,reference){
 const {t,r}=await ref(f,reference),modern=r.fieldsArray.length===11;assert(modern||r.fieldsArray.length===9);
 const k={stars:key(r,'StarLevelWeightSplineArray',9,7),interest:key(r,'RankInterestWeights',7,5),scheme:key(r,'SchemeFitWeight',8,6),pipeline:key(r,'InPipelineWeight',1,1),home:key(r,'InHomeStateWeight',0,0),need:key(r,'TeamNeedsBoost',10,8)};
 const stars=await array(f,r[k.stars]),interest=await array(f,r[k.interest]);assert.equal(stars.fields.length,11);assert.equal(interest.fields.length,10);
 const curves=[];for(const fld of stars.fields){const c=await ref(f,stars.r[fld.key]);assert.equal(c.t.header.uniqueId,1100607787);const x=await array(f,c.r.CalculateY),y=await array(f,c.r.Y);assert.deepEqual(values(x),[0,1,2,3,4]);assert.equal(y.fields.length,5);curves.push({...c,x,y});}
 return {t,r,k,stars,interest,curves,modern,table:curves.map(c=>values(c.y)),rank:values(interest),scheme:r[k.scheme],pipeline:r[k.pipeline],home:r[k.home],need:r[k.need]};
}
function summary(s){return {table:s.table,interest:s.rank,scheme:s.scheme,pipeline:s.pipeline,home:s.home,need:s.need};}
async function verifyTables(v,before,edits,assets){
 assert.equal(v.tables.length,before.size);assert.equal(JSON.stringify(v.assetTable),assets);let changed=0,unchanged=0;
 for(const t of v.tables){const b=before.get(t.header.tableId);assert(b,'Unexpected table');assert.deepEqual(t.header,b.header,'Table layout changed: '+t.name);const actual=Buffer.from(t.data),expected=Buffer.from(b.data),allowed=edits.filter(e=>e.table===t.header.tableId);assert.equal(actual.length,expected.length);
  if(allowed.length){await t.readRecords();for(const e of allowed){const r=t.records[e.row],field=r.fields[e.key];assert.equal(r[e.key],e.after,e.label);assert.equal(field.offset.offset,e.bitOffset);assert.equal(field.offset.length,e.bitLength);const start=8*(t.header.table1StartIndex+e.row*t.header.record1Size)+e.bitOffset;for(let bit=start;bit<start+e.bitLength;bit++){const i=bit>>>3,m=1<<(7-(bit&7));expected[i]=(expected[i]&~m)|(actual[i]&m);}}}
  assert(actual.equals(expected),'Unapproved table-byte change: '+t.name);if(actual.equals(b.data))unchanged++;else changed++;
 }
 return {unchangedTables:unchanged,changedTables:changed,assetsUnchanged:true,tableLayoutsUnchanged:true,onlyApprovedFieldBitsChanged:true};
}
async function routeBuild(route){
 const rel=`frantkdata/${route}/dynasty-tuning-binary.FTC`,src=path.join(base,rel),dest=path.join(out,rel);
 const f=await open(src),before=new Map(f.tables.map(t=>[t.header.tableId,{data:Buffer.from(t.data),header:{...t.header}}])),assets=JSON.stringify(f.assetTable),edits=[];
 const rt=f.getTableByUniqueId(3880326652);await rt.readRecords();const recruiting=rt.records[0],hsRef=num(recruiting.HighSchoolRecruitScoreTunable),portalRef=num(recruiting.TransferRecruitScoreTunable);
 const s=await scoring(f,hsRef),p=await scoring(f,portalRef);assert.equal(s.t.header.uniqueId,3747201733);assert.equal(s.r.index,0);assert.equal(p.t.header.uniqueId,195834476);assert.equal(p.r.index,1);
 assert.equal(s.modern,[0,3,4].includes(route));assert.deepEqual(s.table,OLD_HS);assert.deepEqual(s.rank,OLD_INTEREST);assert.equal(s.scheme,25);assert.equal(s.pipeline,30);assert.equal(s.home,20);assert.equal(s.need,5);
 const shared=new Map([[s.stars.ref,[location(s.t,s.r,s.k.stars)]],[s.interest.ref,[location(s.t,s.r,s.k.interest)]]]);
 for(let i=0;i<11;i++){const c=s.curves[i];shared.set(c.ref,[location(s.stars.t,s.stars.r,s.stars.fields[i].key)]);shared.set(c.y.ref,[location(c.t,c.r,'Y')]);}
 const privateReferences=await assertPrivateReferences(f,shared);
 for(let prestige=0;prestige<11;prestige++){const a=s.curves[prestige].y;for(let star=0;star<5;star++)set(a.t,a.r,a.fields[star].key,HS[prestige][star],`HS prestige ${prestige}, ${star+1} star`,edits);}
 for(let rank=0;rank<10;rank++)set(s.interest.t,s.interest.r,s.interest.fields[rank].key,INTEREST[rank],`HS interest rank ${rank+1}`,edits);
 set(s.t,s.r,s.k.scheme,20,'HS scheme fit',edits);
 if(!verifyOnly)await f.save(dest,{sync:true});
 const v=await open(dest),scope=await verifyTables(v,before,edits,assets),resultHS=await scoring(v,hsRef),resultPortal=await scoring(v,portalRef);
 assert.deepEqual(resultHS.table,HS);assert.deepEqual(resultHS.rank,INTEREST);assert.equal(resultHS.scheme,20);assert.equal(resultHS.pipeline,30);assert.equal(resultHS.home,20);assert.equal(resultHS.need,5);assert.deepEqual(summary(resultPortal),summary(p),'Portal scoring changed');
 const result={route,status:'PASS_OFFLINE_LIVE_PENDING',sourceSha256:sha(fs.readFileSync(src)),sha256:sha(fs.readFileSync(dest)),schemaFields:s.r.fieldsArray.length,activeHSReference:hsRef,activePortalReference:portalRef,...scope,edits,privateReferences,hs:summary(resultHS),portalUnchanged:summary(resultPortal),initialInterestGenerationUnchanged:true,geographyNonStackingCodeUnchanged:true,live_game_verified:false};
 if(!verifyOnly){const audit=path.join(out,`Audit/Test28/ftc-route-${route}.json`),a=JSON.parse(fs.readFileSync(audit));a.test28R8Scoring={priorSha256:result.sourceSha256,edits,scopeAudit:`Audit/Test28R8/ftc-scoring-route-${route}.json`};a.sha256=result.sha256;fs.writeFileSync(audit,JSON.stringify(a,null,2)+'\n');fs.mkdirSync(path.join(out,'Audit/Test28R8'),{recursive:true});fs.writeFileSync(path.join(out,`Audit/Test28R8/ftc-scoring-route-${route}.json`),JSON.stringify(result,null,2)+'\n');}
 console.log(JSON.stringify({route,mode:verifyOnly?'verify':'build',edits:edits.length,...scope,sha256:result.sha256}));return result;
}
(async()=>{
 const routes=[];for(let route=0;route<5;route++)routes.push(await routeBuild(route));
 const result={release:'Test28R8',baseline:'Test28R7',status:'PASS_OFFLINE_LIVE_PENDING',live_game_verified:false,scope:'Only active HS/JUCO prestige-star scoring, school-interest rank weights and scheme fit; portal scoring, geography, initial interest generation, hours, limits, dealbreakers and all unrelated table bits preserved.',hsTable:HS,interest:INTEREST,schemeFit:20,pipeline:30,homeState:20,routes};
 if(!verifyOnly)fs.writeFileSync(path.join(out,'Audit/Test28R8/scoring-and-scope.json'),JSON.stringify(result,null,2)+'\n');
 console.log(JSON.stringify({release:'Test28R8',status:result.status,routes:routes.length,mode:verifyOnly?'verify':'build',live_game_verified:false}));
})().catch(e=>{console.error(e.stack);process.exit(1);});
