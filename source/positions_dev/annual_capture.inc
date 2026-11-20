/* Freeze every team's completed-season evidence before the first rollover.
   Unsupported shapes keep native development. No pointers survive owned refs. */
#include "annual_policy.h"
#define RM_DEV_COUNT (RM_TEAM_LIMIT*RM_MAX_PLAYERS)
typedef struct { U store; volatile LONG ready,seen; } RMDevState;
typedef struct { Rec player,team; int index,score,score_calls,chance_calls; } RMDevContext;
static RMDevRecord dev_records[RM_DEV_COUNT];
static RMDevState dev_state[RM_DEV_COUNT];
static volatile LONG dev_held_by_team[RM_TEAM_LIMIT];
static unsigned dev_count;
static int dev_frozen,dev_failed;
static DWORD dev_tls_index=TLS_OUT_OF_INDEXES;
static double dev_peer_scratch[RM_DEV_COUNT];
static void dev_reset(void){
 dev_count=0;dev_frozen=dev_failed=0;memset(dev_state,0,sizeof(dev_state));
 for(int team=0;team<RM_TEAM_LIMIT;team++)InterlockedExchange(&dev_held_by_team[team],0);
}
static int dev_lookup(unsigned row){unsigned lo=0,hi=dev_count;while(lo<hi){unsigned mid=lo+(hi-lo)/2;if(dev_records[mid].row<row)lo=mid+1;else hi=mid;}return lo<dev_count&&dev_records[lo].row==row?(int)lo:-1;}
static uint64_t dev_draw(unsigned index,uint64_t domain){return annual_draw(dev_records+index,census_year,domain);}
static int dev_record_compare(const void *a,const void *b){unsigned x=((const RMDevRecord*)a)->row,y=((const RMDevRecord*)b)->row;return (x>y)-(x<y);}
static int dev_type(Rec r,U *type,int *kind){
 U vt,getter,name;int16_t count;char text[64];
 if(!r||!local_read((U)r,&vt,8)||!local_read(vt+0x38,&getter,8)||!code_pointer(getter))return 0;
 *type=((U(*)(Rec))getter)(r);
 /* Class lookup 0x8054060 compares the class name at +0x18; +0xd8 is
    the native member-cache count used by the guarded 0x8055d60 getter. */
 if(!*type||!local_read(*type+0x18,&name,8)||!name||!local_read(*type+0xd8,&count,2))return 0;
 unsigned n=0;for(;n<sizeof(text)-1;n++){if(!local_read(name+n,text+n,1))return 0;if(!text[n])break;}text[n]=0;
 static const char *names[]={"SeasonDefensiveStats","SeasonOLineStats","SeasonOffensiveStats","SeasonOffensiveKPReturnStats","SeasonKickingStats","SeasonDefensiveKPReturnStats"};
 static const int counts[]={25,9,30,38,33,33};
 for(unsigned j=0;j<6;j++)if(!strcmp(text,names[j])&&count==counts[j]){*kind=(int)j;return 1;}return 0;
}
static int dev_stat_values(Rec r,int *values,int *supported){
 U type,data,vt,fn;uint32_t row;int kind=-1;
 if(!dev_type(r,&type,&kind)||!identity(r,&data,&row)||!data||!local_read(data,&vt,8)||!local_read(vt+0x100,&fn,8)||!code_pointer(fn))return 0;
 const RMStatFields *sf=rm_stat_fields+(kind==5?0:kind);*supported=kind!=5;
 memset(values,0,RM18R22_STAT_COUNT*sizeof(int));
 for(unsigned j=0;j<sf->count;j++){
  const RMField *f=sf->fields+j;U field=((U(*)(U,unsigned))(game_base+0x8055d60))(type,f->index);
  if(field==0xffffffffu)return 0;int value=((int(*)(U,uint32_t,U))fn)(data,row,field);
  for(unsigned k=0;k<RM18R22_STAT_COUNT;k++)if(!strcmp(f->name,annual_stat_names[k])){values[k]=value;break;}
 }
 return 1;
}
static int dev_read_player(Rec p,RMDevRecord *d,int team,int source,int year){
 U store;uint32_t row;int ok=0,user=0;
 if(!identity(p,&store,&row)||!store||row==UINT32_MAX)return 0;
 memset(d,0,sizeof(*d));d->row=row;d->team=team;d->source=source;d->supported=1;
 int pid;
 if(!scalar(p,228,2,&pid)||!scalar(p,3,3,&d->position)||!scalar(p,202,3,&d->type)||!scalar(p,271,3,&d->before)||
    !scalar(p,242,3,&d->school)||!scalar(p,234,3,&d->redshirt)||!scalar(p,154,3,&d->injury)||!scalar(p,176,2,&d->max_injury)||!scalar(p,0,1,&user))return 0;
 d->pid=(unsigned)pid;if(user)d->supported=0;
 Rec a=array_field(p,243,&ok);U n=array_count(a);if(!ok||n>128){release(a);return 0;}
 int matches=0,values[RM18R22_STAT_COUNT];
 for(U j=0;j<n;j++){
  Rec stat=array_at(a,j,&ok);int supported=0;
  if(!ok||!stat||!dev_stat_values(stat,values,&supported)){d->supported=0;release(stat);continue;}
  if(!supported)d->supported=0;
  if(values[DS_SEAS_YEAR]==year){matches++;memcpy(d->stats,values,sizeof(values));}
  release(stat);
 }
 release(a);d->has_row=matches==1;if(!d->has_row)memset(d->stats,0,sizeof(d->stats));
 return 1;
}
static void dev_freeze(Rec franchise){
 if(dev_frozen||dev_failed)return;dev_failed=1;int ok=0;unsigned rejected=0;
 ULONGLONG start=GetTickCount64();Rec teams=array_field(franchise,45,&ok);U n=array_count(teams);
 if(!ok||n!=RM_TEAM_LIMIT){release(teams);goto fail;}
 for(U i=0;i<n;i++){
  Rec t=array_at(teams,i,&ok);int id=-1;
  if(!ok||!t||!scalar(t,383,2,&id)||id<0||id>=RM_TEAM_LIMIT){release(t);release(teams);goto fail;}
  for(int source=0;source<2;source++){
   Rec a=array_field(t,source?33:239,&ok);U count=array_count(a);
   if(!ok||count>RM_MAX_PLAYERS||dev_count+count>RM_DEV_COUNT){release(a);release(t);release(teams);goto fail;}
   for(U j=0;j<count;j++){
    Rec p=array_at(a,j,&ok);
    if(!ok||!p||!dev_read_player(p,dev_records+dev_count,id,source,census_year)){release(p);release(a);release(t);release(teams);goto fail;}
    rejected+=!dev_records[dev_count].supported;dev_count++;release(p);
   }release(a);
  }release(t);
 }release(teams);
 qsort(dev_records,dev_count,sizeof(*dev_records),dev_record_compare);
 for(unsigned i=1;i<dev_count;i++)if(dev_records[i-1].row==dev_records[i].row)goto fail;
 annual_evaluate(dev_records,dev_count,dev_peer_scratch);dev_frozen=1;dev_failed=0;
 log_line("DEV_ANNUAL_FROZEN year=%d season=%d players=%u unsupported_native_only=%u elapsed_ms=%llu",census_year,census_season,dev_count,rejected,(unsigned long long)(GetTickCount64()-start));return;
fail:
 dev_count=0;log_line("DEV_ANNUAL_DEFER unreadable_or_ambiguous_league; native development only; no custom trait writes");
}
static void dev_start(void *module){
 if(!enabled||capture_fault||InterlockedCompareExchange(&prep_busy,1,0)!=0)return;
 int ok=0;Rec f=reference(module,2,&ok),s=0;if(ok&&f)s=reference(f,44,&ok);
 if(ok&&s&&pilot_stage_guard(s)&&league_census(f,s))dev_freeze(f);
 release(s);release(f);InterlockedExchange(&prep_busy,0);
}
static void dev_prepare(Rec team,Rec season){
 (void)season;int id=-1,accepted=0,rejected=0;if(!dev_frozen||!scalar(team,383,2,&id))return;
 for(unsigned i=0;i<pilot_count;i++){
  int j=dev_lookup(rm_inputs[i].row);if(j<0){rejected++;continue;}
  RMDevRecord *d=dev_records+j;
  if(!d->supported||d->team!=id||d->pid!=rm_inputs[i].pid||d->source!=rm_inputs[i].source||d->position!=rm_inputs[i].position||d->before!=rm_initial[i][field_slot("TraitDevelopment")]){rejected++;continue;}
  dev_state[j].store=rm_inputs[i].store;dev_state[j].ready=1;accepted++;
 }
 log_line("DEV_COHORT_PREPARED accepted=%d rejected=%d year=%d annual_live_inputs=1",accepted,rejected,census_year);
}
