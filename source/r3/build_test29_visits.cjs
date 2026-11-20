'use strict';
/* Usage: DRI_SAVE_PARSER=/path/save_adapter.cjs node build_test29_visits.cjs R14_DIR TEST29_DIR
   Exactly one named field per route; no curve, cost, interest or influence edits. */
const fs=require('fs'),path=require('path'),assert=require('assert/strict'),crypto=require('crypto');
const api=require(path.resolve(process.env.DRI_SAVE_PARSER));
const [beforeDir,outDir]=process.argv.slice(2).map(x=>path.resolve(x));assert.notEqual(beforeDir,outDir);
const sha=b=>crypto.createHash('sha256').update(b).digest('hex');
const open=p=>api.create(p,{gameYearOverride:27,gameTypeOverride:'college',autoUnempty:false,saveOnChange:false});
(async()=>{
 const routes=[];
 for(let route=0;route<5;route++){
  const rel=`frantkdata/${route}/dynasty-tuning-binary.FTC`,src=path.join(beforeDir,rel),dest=path.join(outDir,rel);
  const f=await open(src),assets=JSON.stringify(f.assetTable),before=new Map(f.tables.map(t=>[t.header.tableId,{data:Buffer.from(t.data),header:{...t.header}}]));
  const t=f.getTableByUniqueId(3880326652);await t.readRecords();const r=t.records[0],field=r.fields.MaxRecruitVisitsPerWeek;
  assert.equal(r.MaxRecruitVisitsPerWeek,4);assert(!field.offset.valueInSecondTable&&!field.offset.valueInThirdTable);
  const edit={table_uid:3880326652,table_id:t.header.tableId,row:0,field:'MaxRecruitVisitsPerWeek',before:4,after:10,bit_offset:field.offset.offset,bit_length:field.offset.length};
  r.MaxRecruitVisitsPerWeek=10;await f.save(dest,{sync:true});const v=await open(dest);
  assert.equal(v.tables.length,before.size);assert.equal(JSON.stringify(v.assetTable),assets);let changed=0;
  for(const table of v.tables){
   const prior=before.get(table.header.tableId);assert(prior);assert.deepEqual(table.header,prior.header);
   const actual=Buffer.from(table.data),expected=Buffer.from(prior.data);assert.equal(actual.length,expected.length);
   if(table.header.tableId===edit.table_id){
    await table.readRecords();const vr=table.records[0],vf=vr.fields[edit.field];assert.equal(vr[edit.field],10);assert.equal(vf.offset.offset,edit.bit_offset);assert.equal(vf.offset.length,edit.bit_length);
    const start=8*table.header.table1StartIndex+edit.bit_offset;
    for(let bit=start;bit<start+edit.bit_length;bit++){const index=bit>>>3,mask=1<<(7-(bit&7));expected[index]=(expected[index]&~mask)|(actual[index]&mask);}
   }
   assert(actual.equals(expected),`Unapproved table bits: ${table.name}`);if(!actual.equals(prior.data))changed++;
  }
  assert.equal(changed,1);
  routes.push({route,before_sha256:sha(fs.readFileSync(src)),after_sha256:sha(fs.readFileSync(dest)),only_approved_field_bits_changed:true,assets_unchanged:true,table_layouts_unchanged:true,table_count:before.size,changed_tables:changed,edits:[edit]});
 }
 const report={status:'PASS_OFFLINE_LIVE_PENDING',scope:'MaxRecruitVisitsPerWeek only; effective ceiling 10 requires the matched active Test29 DLL because native coaching bonuses are additive.',routes};
 fs.mkdirSync(path.join(outDir,'Runtime/Portal/Audit/Test29'),{recursive:true});fs.writeFileSync(path.join(outDir,'Runtime/Portal/Audit/Test29/ftc_changes.json'),JSON.stringify(report,null,2)+'\n');console.log(JSON.stringify(report,null,2));
})().catch(e=>{console.error(e);process.exit(1);});
