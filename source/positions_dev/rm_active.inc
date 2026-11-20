/* Active all-CPU compatible position preparation. No roster membership/finance writes. */
#include "dynamic_inputs.h"
#include "conversion_metadata.h"
#include "pilot_policy.c"
static int pilot_ready;
static const char *ability_names[5]={"PhysicalAbility1","PhysicalAbility2","PhysicalAbility3","PhysicalAbility4","PhysicalAbility5"};
static const char *cap_names[6]={"SkillGroupCap1","SkillGroupCap2","SkillGroupCap3","SkillGroupCap4","SkillGroupCap5","SkillGroupCap6"};
static Rec pilot_players[RM_MAX_PLAYERS]; /* owned only during one prep callback */
static const int unit_roles[6][3]={{5,9,-1},{6,8,-1},{10,11,-1},{13,15,-1},{17,18,-1},{19,20,-1}};
static const char *unit_names[6]={"tackle","guard","defensive_end","outside_linebacker","safety","specialist"};
static const char *role_names[22]={"QB","HB","FB","WR","TE","LT","LG","C","RG","RT","LE","RE","DT","LOLB","MLB","ROLB","CB","FS","SS","K","P","LS"};
static int field_slot(const char *name) {
    for(unsigned i=0;i<RM_FIELD_COUNT;i++)if(!strcmp(fields_Player[i].name,name))return (int)i;
    return -1;
}
static __declspec(noinline) int read_all(Rec p,int out[RM_FIELD_COUNT]) {
    U vt,getter,type,data,dvt,fn[3],cache;uint32_t row;int16_t count;
    if(!p||!local_read((U)p,&vt,8)||!local_read(vt+0x38,&getter,8)||!code_pointer(getter))return 0;
    type=((U(*)(Rec))getter)(p);
    if(!type||!local_read(type+0xd0,&cache,8)||!cache||!local_read(type+0xd8,&count,2)||count<=0||!identity(p,&data,&row)||!data||!local_read(data,&dvt,8))return 0;
    for(unsigned j=0;j<3;j++)if(!local_read(dvt+0xf8+8*j,fn+j,8)||!code_pointer(fn[j]))return 0;
    for(unsigned i=0;i<RM_FIELD_COUNT;i++){
        const RMField*f=fields_Player+i;if(f->index>=(unsigned)count||f->kind<1||f->kind>3)return 0;
        U field=((U(*)(U,unsigned))(game_base+0x8055d60))(type,f->index);if(field==0xffffffffu)return 0;
        out[i]=f->kind==1?((unsigned char(*)(U,uint32_t,U))fn[0])(data,row,field)!=0:((int(*)(U,uint32_t,U))fn[f->kind-1])(data,row,field);
    }return 1;
}
#include "../set_record_scalar.inc"
static int set_slot(Rec p,int slot,int value) {
    return slot>=0 && slot<RM_FIELD_COUNT && set_scalar(p,fields_Player[slot].index,fields_Player[slot].kind,value);
}
static __declspec(noinline) Rec cast_record(Rec raw,unsigned tag) {
    U vt,fn;Rec tmp=0,out=0;
    if(!raw || !local_read((U)raw,&vt,8) || !local_read(vt+0x28,&fn,8) || !code_pointer(fn))return 0;
    Rec *r=((Rec*(*)(Rec,Rec*,unsigned))fn)(raw,&tmp,tag);
    if(r){out=*r;*r=0;}release(tmp);return out;
}
static __declspec(noinline) int input_index(Rec p) {
    U store;uint32_t row;if(!identity(p,&store,&row))return -1;
    for(unsigned i=0;i<pilot_count;i++)if(rm_inputs[i].row==row && rm_inputs[i].store==store)return (int)i;
    return -1;
}
static int chart_index(int position) {
    for(unsigned i=0;i<sizeof(charts)/sizeof(charts[0]);i++)if(!strcmp(charts[i].name,role_names[position]))return (int)i;
    return -1;
}
static int destination_type(int oldpos,int newpos,int type) {
    if(!rm_compatible(oldpos,newpos))return -1;
    int family=rm_family(oldpos);
    static const int low[6]={31,35,39,47,58,61},high[6]={34,38,42,50,60,62};
    if(oldpos==RM_C){if(type<27||type>30)return -1;}
    else if(type<low[family]||type>high[family])return -1;
    if((oldpos==RM_C)!=(newpos==RM_C)) {
        for(unsigned i=0;i<sizeof(rm_type_maps)/sizeof(rm_type_maps[0]);i++)if(rm_type_maps[i].old_type==type)return rm_type_maps[i].new_type;
        return -1;
    }
    return type;
}
static __declspec(noinline) int role_grade(Rec module,Rec p,int type,int *grade) {
    /* Native read-only grade calculation, same attribute vector and flags as
       the UI conversion preview. NOT Player.GetOverallGrade (stored only). */
    U vt,fn,typ,wrapper[3];int attributes[57],grades[6]={0};
    if(!local_read((U)p,&vt,8)||!local_read(vt+0x38,&fn,8)||!code_pointer(fn))return 0;
    typ=((U(*)(Rec))fn)(p);if(!typ)return 0;
    wrapper[0]=0;wrapper[1]=(U)p;wrapper[2]=typ;
    ((void(*)(U*,int*))(game_base+0x8592d40))(wrapper,attributes);
    for(unsigned i=0;i<57;i++)if(attributes[i]<0 || attributes[i]>100)return 0;
    ((void(*)(Rec,Rec*,int,int*,int*,unsigned char,int))(game_base+0x8384910))(module,&p,type,attributes,grades,1,-1);
    if(grades[0]<1 || grades[0]>99)return 0;
    *grade=grades[0];return 1;
}
static int same_record(Rec a,Rec b) {
    U as,bs;uint32_t ar,br;return identity(a,&as,&ar)&&identity(b,&bs,&br)&&as==bs&&ar==br;
}
static __declspec(noinline) int array_replace(Rec a,unsigned index,Rec p) {
    U data,vt,fn;int ok=0;
    if(index>=array_count(a)||!local_read((U)a+0x10,&data,8)||!local_read(data,&vt,8)||!local_read(vt+0x160,&fn,8)||!code_pointer(fn))return 0;
    /* Native reference setter takes an owned/borrowed Rec HOLDER, not Rec. */
    ((void(*)(Rec,U,Rec*))(game_base+0x805e530))(a,index,&p);
    Rec after=array_at(a,index,&ok);int match=ok&&same_record(after,p);release(after);return match;
}
static void pilot_clear(void) {
    pilot_ready=0;
    for(unsigned i=0;i<pilot_count;i++){release(pilot_players[i]);pilot_players[i]=0;}
    pilot_count=0;memset(unit_status,0,sizeof(unit_status));team_conversions=0;
}
static int pilot_stage_guard(Rec season) {
    /* Actual PRE_YEAR_SEASON evidence: the broad stage is OffSeason (2),
       while the separate integer CurrentOffseasonStage is 8. Resolve the
       validated native member by name, including its actual scalar kind. */
    static const struct { const char *name; int value; } expected[]={
        {"CurrentStage",2},{"CurrentOffseasonStage",8},{"CurrentWeekType",6},
        {"CurrentWeek",7}
    };
    if(!season){log_line("PILOT_INPUT_REJECT season_reference_missing");return 0;}
    for(unsigned i=0;i<sizeof(expected)/sizeof(expected[0]);i++) {
        const RMField *field=0;int actual=0,read_ok=0;
        for(unsigned j=0;j<sizeof(fields_SeasonInfo)/sizeof(fields_SeasonInfo[0]);j++)
            if(!strcmp(fields_SeasonInfo[j].name,expected[i].name))field=fields_SeasonInfo+j;
        if(field)read_ok=scalar(season,field->index,field->kind,&actual);
        if(!read_ok||actual!=expected[i].value) {
            log_line("PILOT_INPUT_REJECT stage_field=%s expected=%d actual=%d read_ok=%d",expected[i].name,expected[i].value,actual,read_ok);
            return 0;
        }
    }
    log_line("POSITION_STAGE_ACCEPT CurrentStage=2 CurrentOffseasonStage=8 CurrentWeekType=6 CurrentWeek=7");
    return 1;
}
#include "rm_collect.inc"
static int convert_preserving(Rec manager,Rec team,int i,int newpos,int newtype,int newgrade) {
    Rec p=pilot_players[i],nil=0;int before[RM_FIELD_COUNT],after[RM_FIELD_COUNT],expected[RM_FIELD_COUNT];
    if(!read_all(p,before))return 0;memcpy(expected,before,sizeof(expected));
    int oldtype=before[field_slot("PlayerType")];
    expected[field_slot("Position")]=newpos;expected[field_slot("PlayerType")]=newtype;
    expected[field_slot("OverallRating")]=newgrade;
    if(oldtype!=newtype) {
        const RMTypeMap *map=0;
        for(unsigned k=0;k<sizeof(rm_type_maps)/sizeof(rm_type_maps[0]);k++)if(rm_type_maps[k].old_type==oldtype && rm_type_maps[k].new_type==newtype)map=rm_type_maps+k;
        if(!map)return 0;
        for(unsigned k=0;k<5;k++)expected[field_slot(ability_names[k])]=before[field_slot(ability_names[map->ability[k]])];
        for(unsigned k=0;k<6;k++)expected[field_slot(cap_names[k])]=before[field_slot(cap_names[map->cap[k]])];
    }
    player_snapshot("POSITION_BEFORE",p,rm_inputs[i].source,0,0);
    ((void(*)(Rec,Rec*,Rec*,int,int,Rec*))(game_base+0x8612da0))(manager,&team,&p,newpos,newtype,&nil);
    release(nil);player_snapshot("POSITION_NATIVE_AFTER",p,rm_inputs[i].source,0,0);
    if(!read_all(p,after))return 0;
    for(unsigned k=0;k<RM_FIELD_COUNT;k++)if(after[k]!=expected[k]) {
        /* Restore earned state only through verified scalar setters; derived
           position/type/OVR and semantic slot remapping are explicit below. */
        if(!set_slot(p,(int)k,expected[k])){log_line("POSITION_PRESERVATION_FAILURE row=%u field=%s",rm_inputs[i].row,fields_Player[k].name);return 0;}
    }
    if(!read_all(p,after)||memcmp(after,expected,sizeof(after)))return 0;
    log_line("POSITION_APPLIED row=%u old=%d new=%d oldtype=%d newtype=%d grade=%d preservation=PASS",rm_inputs[i].row,before[field_slot("Position")],newpos,oldtype,newtype,newgrade);
    memcpy(rm_expected[i],expected,sizeof(expected));team_conversions++;
    player_snapshot("POSITION_AFTER",p,rm_inputs[i].source,0,0);return 1;
}
#include "rm_positions.inc"
