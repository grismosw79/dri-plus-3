'use strict';
// Scoped R6 changes: two HS hour curves and preseason board totals only.
// Every other decoded table byte is checked against the immutable R5 input.
const fs=require('fs'),path=require('path'),assert=require('assert/strict'),crypto=require('crypto');
const api=require('../../../Audit/PreparedSaveEvidence/Parser/save_adapter.cjs');
const [baseArg,outArg]=process.argv.slice(2),base=path.resolve(baseArg),out=path.resolve(outArg);
assert.notEqual(base,out);
const num=x=>typeof x==='string'?parseInt(x,2):x,sha=b=>crypto.createHash('sha256').update(b).digest('hex');
const open=p=>api.create(p,{gameYearOverride:27,gameTypeOverride:'college',autoUnempty:false,saveOnChange:false});
async function ref(f,v){v=num(v);const t=f.getTableById(v>>>17);await t.readRecords();return {t,r:t.records[v&131071]};}
async function array(f,v){let {t,r}=await ref(f,v);const n=t.data.readUInt32BE(t.header.headerSize+r.index*4);return {t,r,fields:r.fieldsArray.slice(0,n)};}
function snapshot(f){return new Map(f.tables.map(t=>[t.header.tableId,Buffer.from(t.data)]));}
function set(t,r,key,value,edits){const old=r[key];edits.push({table:t.header.tableId,row:r.index,key,before:old,after:value});r[key]=value;}
async function saveVerify(f,dest,before,edits){
 await f.save(dest,{sync:true});const v=await open(dest);let unchanged=0,changed=0;
 assert.equal(v.tables.length,before.size);
 for(const t of v.tables){
  const prior=before.get(t.header.tableId),actual=Buffer.from(t.data),allowed=edits.filter(e=>e.table===t.header.tableId);
  if(!allowed.length){assert(actual.equals(prior),t.name+' unapproved change');unchanged++;continue;}
  await t.readRecords();const expected=Buffer.from(prior);assert.equal(actual.length,expected.length);
  for(const e of allowed){const r=t.records[e.row],field=r.fields[e.key];assert.equal(r[e.key],e.after);
   const start=8*(t.header.table1StartIndex+e.row*t.header.record1Size)+field.offset.offset;
   for(let bit=start;bit<start+field.offset.length;bit++){const i=bit>>>3,m=1<<(7-(bit&7));expected[i]=(expected[i]&~m)|(actual[i]&m);}
  }
  assert(actual.equals(expected),t.name+' changed outside approved fields');changed++;
 }
 return {unchangedTables:unchanged,changedTables:changed,sha256:sha(fs.readFileSync(dest)),edits};
}
async function curve(f,v){const {r}=await ref(f,v);const x=await array(f,r.CalculateY),y=await array(f,r.Y);return {x:x.fields.map(k=>x.r[k.key]),y:y.fields.map(k=>y.r[k.key])};}
(async()=>{
 const reports=[];
 for(let route=0;route<5;route++){
  const rel='frantkdata/'+route+'/dynasty-tuning-binary.FTC',src=path.join(base,rel),dest=path.join(out,rel);
  const f=await open(src),before=snapshot(f),edits=[],t=f.getTableByUniqueId(3880326652);await t.readRecords();const r=t.records[0];
  const refs=[];
  for(const key of ['PreseasonRecruitingHoursSpline','RegularSeasonRecruitingHoursSpline']){
   const c=await ref(f,r[key]),a=await array(f,c.r.Y);assert.equal(a.fields.length,11);refs.push(num(c.r.Y));
   for(const k of a.fields){assert.equal(a.r[k.key],key.startsWith('Preseason')?1000:800);set(a.t,a.r,k.key,1500,edits);}
  }
  assert.notEqual(refs[0],refs[1]);
  const splines=f.getTableByUniqueId(1100607787);await splines.readRecords();
  for(const value of refs)assert.equal(splines.records.filter(c=>!c.isEmpty && num(c.Y)===value).length,1,'Shared hour array');
  const nil={};
  for(const table of f.tables)if(table.name==='ProgramPointsTuning'){
   await table.readRecords();
   for(const [name,ordinal]of [['BaseNILValueSpline',1],['NILDealbreakerModifierSpline',20]]){
    const row=table.records[0],key=row.fields[name]?name:'Field_'+ordinal;
    nil[name]=await curve(f,row[key]);
   }
  }
  const report=await saveVerify(f,dest,before,edits);report.route=route;report.sourceSha256=sha(fs.readFileSync(src));report.nilCurvesUnchanged=nil;
  reports.push(report);
  const audit=path.join(out,'Audit/Test28/ftc-route-'+route+'.json'),a=JSON.parse(fs.readFileSync(audit));a.test28R6Hours={priorSha256:report.sourceSha256,edits};a.sha256=report.sha256;fs.writeFileSync(audit,JSON.stringify(a,null,2)+'\n');
 }
 const saveSrc=path.join(base,'Prepared-Save/DYNASTY-TEST28R5START'),saveDest=path.join(out,'Prepared-Save/DYNASTY-TEST28R6START');
 assert.equal(sha(fs.readFileSync(saveSrc)),'13e69593d1cdc98a759f46d280cccc4816ac4b5bc2bd35ee7ee24eec925bbe34');
 const f=await open(saveSrc),before=snapshot(f),edits=[],b=f.getTableByUniqueId(220276943);await b.readRecords();
 for(const r of b.records.filter(r=>!r.isEmpty)){assert.equal(r.RecruitingHoursAssigned,0);assert.equal(r.RecruitingHoursProcessed,0);assert.equal(r.RecruitingHoursTotal,1000);set(b,r,'RecruitingHoursTotal',1500,edits);}
 assert.equal(edits.length,138);
 const save=await saveVerify(f,saveDest,before,edits);save.sourceSha256=sha(fs.readFileSync(saveSrc));
 const result={status:'STRUCTURALLY_VERIFIED_LIVE_PENDING',routes:reports,preparedSave:save,scope:'Only preseason/regular hours changed; offseason, playoff, NIL, dealbreakers, board capacities and roster eligibility preserved'};
 fs.mkdirSync(path.join(out,'Audit/Test28R6'),{recursive:true});fs.writeFileSync(path.join(out,'Audit/Test28R6/hours-and-save.json'),JSON.stringify(result,null,2)+'\n');
 console.log(JSON.stringify({routes:reports.map(r=>({route:r.route,sha256:r.sha256,unchangedTables:r.unchangedTables})),preparedSaveSha256:save.sha256}));
})().catch(e=>{console.error(e);process.exit(1);});
