#include "pilot_profile.h"
#include "rm_policy.h"
#include "transaction_fields.h"
enum {PILOT_SCALARS=sizeof(pilot_player_fields)/sizeof(RMField), PILOT_VECTOR_MAX=512};
typedef struct {unsigned char data[16];U allocator;} PilotReason;
typedef struct {int value[PILOT_SCALARS];} PilotPlayerState;
typedef struct {int value[6];} PilotMoney;
typedef struct {U address,begin,end,cap;unsigned n,selected;Rec refs[PILOT_VECTOR_MAX];} PilotVector;
static void pilot_reason_text(const PilotReason *reason,char out[201]) {
    unsigned n=0;out[0]=0;
    if(reason->data[15]&0x80) {
        U data=0;uint32_t length=0,capacity=0;
        memcpy(&data,reason->data,8);memcpy(&length,reason->data+8,4);memcpy(&capacity,reason->data+12,4);capacity&=0x7fffffffu;
        if(!data||length>capacity||capacity>1048576){strcpy(out,"UNAVAILABLE_NATIVE_REASON");return;}
        n=length>200?200:length;
        if(n&&!local_read(data,out,n)){strcpy(out,"UNREADABLE_NATIVE_REASON");return;}
    } else {
        if(reason->data[15]>15){strcpy(out,"UNAVAILABLE_INLINE_REASON");return;}
        n=15-reason->data[15];memcpy(out,reason->data,n);
    }
    out[n]=0;
}
static int pilot_same(Rec a,Rec b) {
    U sa,sb;uint32_t ra,rb;
    return identity(a,&sa,&ra)&&identity(b,&sb,&rb)&&sa==sb&&ra==rb;
}
static int pilot_get(Rec r,unsigned index,unsigned kind) {
    int v;if(!scalar(r,index,kind,&v)){capture_error("RM17R3 stored field");return INT_MIN;}return v;
}
static int pilot_not_in_array(Rec owner,unsigned field,Rec player,unsigned limit) {
    int ok=0;Rec a=array_field(owner,field,&ok);U n=array_count(a);
    if(!ok||n>limit){release(a);return 0;}
    for(U i=0;i<n;i++){Rec p=array_at(a,i,&ok);int same=p&&pilot_same(p,player);release(p);
        if(!ok||same){release(a);return 0;}}
    release(a);return 1;
}
static int pilot_unheld(Rec f,Rec p) {
    if(!pilot_not_in_array(f,14,p,10000)||!pilot_not_in_array(f,42,p,10000))return 0;
    int ok=0;Rec a=array_field(f,11,&ok);U n=array_count(a);
    if(!ok||n>200){release(a);return 0;}
    for(U i=0;i<n;i++){Rec t=array_at(a,i,&ok);int valid=ok&&t&&pilot_not_in_array(t,239,p,200)&&pilot_not_in_array(t,33,p,200);
        release(t);if(!valid){release(a);return 0;}}
    release(a);return !capture_fault;
}
static int pilot_money(Rec team,PilotMoney *m) {
    const unsigned fields[6]={170,217,225,227,238,286};
    for(unsigned i=0;i<6;i++)if(!scalar(team,fields[i],2,&m->value[i]))return 0;
    return 1;
}
static int pilot_log_snapshot(const PilotPlayerState *s,int action,int after) {
    char b[16000];size_t n=0;append(b,sizeof(b),&n,"\"event\":\"RM17R3_PLAYER_STATE\",\"action\":%d,\"after\":%d,\"fields\":{",action,after);
    for(unsigned i=0;i<PILOT_SCALARS;i++)if(!append(b,sizeof(b),&n,"%s\"%s\":%d",i?",":"",pilot_player_fields[i].name,s->value[i]))return 0;
    append(b,sizeof(b),&n,"}");emit(b);return !capture_fault;
}
static int pilot_snapshot(Rec p,PilotPlayerState *s,int action,int after) {
    for(unsigned i=0;i<PILOT_SCALARS;i++)if(!scalar(p,pilot_player_fields[i].index,pilot_player_fields[i].kind,&s->value[i]))return 0;
    if(rm18_deep())return pilot_log_snapshot(s,action,after);
    return !capture_fault;
}
/* The existing two-snapshot diagnostic entry uses the same field policy as
   the live transaction receipt. It has no game state or logging dependency. */
static __declspec(noinline) int pilot_preserved(const PilotPlayerState *before,const PilotPlayerState *after) {
    for(unsigned i=0;i<PILOT_SCALARS;i++)
        if(before->value[i]!=after->value[i]&&rm_transaction_field(pilot_player_fields[i].index)==RM_PRESERVE_FIELD)return 0;
    return 1;
}
static int pilot_vector(U module,unsigned group,unsigned slot,Rec p,PilotVector *v) {
    const unsigned offsets[3]={0xb80,0xb88,0xb90},lengths[3]={22,18,11};U root=0,table=0,end=0;
    if(group>2||slot>=lengths[group]||!local_read(module+offsets[group],&root,8)||!root||
       !local_read(root,&table,8)||!local_read(root+8,&end,8)||end<table||end-table!=lengths[group]*32)return 0;
    v->address=table+slot*32;
    if(!local_read(v->address,&v->begin,8)||!local_read(v->address+8,&v->end,8)||!local_read(v->address+16,&v->cap,8)||
       !v->begin||v->end<v->begin||v->cap<v->end||(v->end-v->begin)%8||(v->cap-v->begin)%8)return 0;
    U n=(v->end-v->begin)/8;if(!n||n>PILOT_VECTOR_MAX)return 0;v->n=(unsigned)n;
    if(!local_read(v->begin,v->refs,v->n*8))return 0;
    unsigned matches=0;
    for(unsigned i=0;i<v->n;i++)if(pilot_same(v->refs[i],p)){v->selected=i;matches++;}
    return matches==1;
}
static __declspec(noinline) int pilot_rotate(PilotVector *v) {
    if(!v||!v->n||v->n>PILOT_VECTOR_MAX||v->selected>=v->n||v->end-v->begin!=v->n*8)return 0;
    MEMORY_BASIC_INFORMATION m;U current=v->begin;
    while(current<v->end){
        if(!VirtualQuery((void*)current,&m,sizeof(m))||m.State!=MEM_COMMIT||(m.Protect&(PAGE_GUARD|PAGE_NOACCESS))||
           !(m.Protect&(PAGE_READWRITE|PAGE_WRITECOPY))||(U)m.BaseAddress>current||!m.RegionSize||
           (U)m.BaseAddress>UINTPTR_MAX-m.RegionSize||(U)m.BaseAddress+m.RegionSize<=current)return 0;
        current=(U)m.BaseAddress+m.RegionSize;
    }
    Rec check[PILOT_VECTOR_MAX];if(!local_read(v->begin,check,v->n*8)||memcmp(check,v->refs,v->n*8))return 0;
    /* Move selected existing reference to the back without changing reference
       counts. Native mode-0 pop consumes exactly that one reference. */
    Rec *a=(Rec*)v->begin;Rec selected=a[v->selected];
    for(unsigned i=v->selected;i+1<v->n;i++)a[i]=a[i+1];a[v->n-1]=selected;
    return 1;
}
static int pilot_removed_vector(const PilotVector *v) {
    U begin=0,end=0;if(!local_read(v->address,&begin,8)||!local_read(v->address+8,&end,8)||begin!=v->begin||end!=v->end-8)return 0;
    Rec now[PILOT_VECTOR_MAX];if(!local_read(begin,now,(v->n-1)*8))return 0;
    for(unsigned i=0,j=0;i<v->n;i++)if(i!=v->selected&&now[j++]!=v->refs[i])return 0;
    return 1;
}
typedef struct {Rec player,recruit;U store;uint32_t row;} RMRef;
static RMState *rm_state;
static RMRef *rm_refs;
static PilotPlayerState *rm_cut_states;
static PilotPlayerState *rm_expected_states;
static int *rm_expected_nil;
static PilotMoney rm_expected_money[RM_TEAMS];
/* Native Team member indices, independently verified against the captured
   Team name table. These are part of the program budget, not player NIL. */
static int rm_expected_facilities[RM_TEAMS][2]; /* spent(82), renewal reserve(83) */
static unsigned char rm_cut_verified[RM_ACTIONS];
static unsigned char rm_team_phase[RM_TEAMS];
static int rm_audit_ready,rm_end_complete,rm_encourage_started,rm_encourage_complete;
static unsigned char rm_encouraged[RM_TEAMS];
static const char *rm_audit_phase="UNSPECIFIED";
static Rec rm_teams[RM_TEAMS],rm_franchise;
/* Presentation IDs exceed 65,535 in real rosters. This is an open-addressed
   index, not a dense array indexed by ID. Identity is still store + row. */
static int rm_pid[65536];
static unsigned rm_pid_slot(int id){unsigned slot=((uint32_t)id*UINT32_C(2654435761))&65535;for(unsigned n=0;n<65536;n++,slot=(slot+1)&65535)if(rm_pid[slot]<0||rm_state->p[rm_pid[slot]].id==id)return slot;return 65536;}
static int rm_lookup(int id){if(id<0)return -1;unsigned slot=rm_pid_slot(id);return slot<65536?rm_pid[slot]:-1;}
static int rm_index_put(int id,int i){unsigned slot=rm_pid_slot(id);if(slot>=65536)return 0;rm_pid[slot]=i;return 1;}
static U rm_manager,rm_context_store;
static uint32_t rm_context_row;
static int rm_year,rm_active,rm_done,rm_failed,rm_planned,rm_executed,rm_validation_count;
static unsigned char rm_validated[RM_TEAMS];
static int rm_native_capacity_delegated,rm_fallback,rm_mutation_started,rm_gate_inflight,rm_native_diverged;
static char rm_first_tx_issue[224];
static __declspec(noreturn) void rm_halt_failed_branch(void){
 log_line("RM17R3 UNRECOVERABLE_BRANCH_STOP; native continuation forbidden after mutation, uncertain native effects, or skipped processing");
 FlushFileBuffers(log_file);
 MessageBoxW(0,L"Error code: ROSTER_PREPARE_FAILED\n\nFiles needed:\nCurrent session RM18R22 CSV, JSONL logs and receipt TXT.\nLast good Encourage Transfers save from before the error.",L"DRI+ RM18R22 - advance stopped",MB_OK|MB_ICONERROR);
 TerminateProcess(GetCurrentProcess(),0x524d1701);ExitProcess(0x524d1701);
}
static void rm_failed_route(void){
 if(!enabled||(!rm_active&&!rm_failed))return;
 if(!rm_failed&&!capture_fault)return;
 if(rm_mutation_started||rm_gate_inflight||rm_native_diverged)rm_halt_failed_branch();
 if(!rm_fallback){rm_fallback=1;log_line("RM17R3 PREFLIGHT_DECLINED_NATIVE_FALLTHROUGH; zero custom mutation attempts; forward all remaining native processing; test branch remains rejected");FlushFileBuffers(log_file);}
}
static void rm_event(const char *event,int pid,int src,int dst,int value){char b[512];snprintf(b,sizeof(b),"\"event\":\"RM17R3\",\"action\":\"%s\",\"pid\":%d,\"source\":%d,\"destination\":%d,\"value\":%d,\"year\":%d",event,pid,src,dst,value,rm_year);emit(b);}
static void rm_abort(const char *why){if(!rm_failed){rm_failed=1;log_line("RM17R3 FAILED_DISCARD_BRANCH %s; do not use resulting save; no reverse transactions attempted",why);FlushFileBuffers(log_file);}}
/* Read the record/type/vtable once per snapshot. All 268 values are still
   checked around every gate; avoid thousands of redundant OS reads per edge. */
static int rm_snapshot_quiet(Rec p,PilotPlayerState*s){U vt,getter,type,data,dvt,fn[3],cache;uint32_t row;int16_t count;
 if(!p||!local_read((U)p,&vt,8)||!local_read(vt+0x38,&getter,8)||!code_pointer(getter))return 0;type=((U(*)(Rec))getter)(p);
 if(!type||!local_read(type+0xd0,&cache,8)||!cache||!local_read(type+0xd8,&count,2)||count<=0||!identity(p,&data,&row)||!data||!local_read(data,&dvt,8))return 0;
 for(unsigned j=0;j<3;j++)if(!local_read(dvt+0xf8+8*j,&fn[j],8)||!code_pointer(fn[j]))return 0;
 for(unsigned i=0;i<PILOT_SCALARS;i++){const RMField*f=&pilot_player_fields[i];if(f->index>=(unsigned)count||f->kind<1||f->kind>3)return 0;U field=((U(*)(U,unsigned))(game_base+0x8055d60))(type,f->index);if(field==0xffffffffu)return 0;s->value[i]=f->kind==1?((unsigned char(*)(U,uint32_t,U))fn[0])(data,row,field)!=0:((int(*)(U,uint32_t,U))fn[f->kind-1])(data,row,field);}return 1;
}
static int rm_clock(Rec f,int*year){int ok=0;Rec r=reference(f,44,&ok);int valid=ok&&r&&pilot_get(r,3,3)==2&&pilot_get(r,5,3)==6&&pilot_get(r,8,2)==9&&pilot_get(r,4,2)==9;*year=r?pilot_get(r,2,2):0;release(r);return valid&&*year>=2027&&*year<2200&&!capture_fault;}
static int rm_owned(Rec p,int expected){int count=0;for(int t=0;t<RM_TEAMS;t++)for(int slot=0;slot<2;slot++){int ok=0;Rec a=array_field(rm_teams[t],slot?33:239,&ok);U n=array_count(a);if(!ok||n>RM_ROOM){release(a);return 0;}for(U i=0;i<n;i++){Rec q=array_at(a,i,&ok);int same=q&&pilot_same(p,q);release(q);if(!ok||(same&&(slot||t!=expected))){release(a);return 0;}count+=same;}release(a);}return count==(expected>=0?1:0);}
static int rm_native_gate(void*ctx,int pi,int ti,int*quote){(void)ctx;Rec p=rm_refs[pi].player,r=rm_refs[pi].recruit,t=rm_teams[ti];int pid=rm_state->p[pi].id;U ptype=0,rtype=0,grades=0,finance=0,vt=0,getter=0;PilotPlayerState before,after;PilotMoney bm,am;
 if(capture_fault||!rm_snapshot_quiet(p,&before)||!pilot_money(t,&bm))return -1;
 if(!local_read(rm_manager+0x80,&ptype,8)||!local_read(rm_manager+0xa0,&rtype,8)||!local_read(rm_manager+0x180,&grades,8)||!local_read(rm_manager+0x1a0,&finance,8)||!ptype||!rtype||!grades||!finance)return -1;
 Rec input=r?r:p;if(!local_read((U)input,&vt,8)||!local_read(vt+0x38,&getter,8)||!code_pointer(getter))return -1;U type=((U(*)(Rec))getter)(input);if(!((unsigned char(*)(U,U))(game_base+0x8059c60))(type,r?rtype:ptype))return -1;
 PilotReason reason={0};reason.data[15]=15;rm_gate_inflight=1;int allowed=((unsigned char(*)(U,Rec*,Rec*,PilotReason*))(game_base+0x816b190))(rm_manager,&t,&input,&reason)!=0;
 char detail[201];pilot_reason_text(&reason,detail);log_line("RM17R3_NATIVE_GATE pid=%d destination=%d allowed=%d reason=%s",pid,ti,allowed,detail[0]?detail:"EMPTY");
 if(reason.data[15]&0x80){U data=0;uint32_t len=0,cap=0;memcpy(&data,reason.data,8);memcpy(&len,reason.data+8,4);memcpy(&cap,reason.data+12,4);cap&=0x7fffffffU;if(!data||len>cap||cap>1048576){log_line("RM18R22 NATIVE_REASON_HEADER_INVALID pid=%d destination=%d; native call integrity uncertain",pid,ti);return -1;}((void(*)(U*,U,U))(game_base+0x19ea6a0))(&reason.allocator,data,(U)cap+1);}else if(reason.data[15]>15)return -1;
 int grade=-1;*quote=-1;if(allowed){int key=((int(*)(U,int))(game_base+0x840a660))(grades,9);grade=((int(*)(U,Rec*,int,Rec*))(game_base+0x8407000))(grades,&t,key,&p);*quote=r?((int(*)(U,Rec*,Rec*))(game_base+0x82c0c40))(finance,&r,&t):((int(*)(U,Rec*,Rec*))(game_base+0x82c0dd0))(finance,&p,&t);}
 log_line("RM18R22 NATIVE_GATE_RESULT pid=%d destination=%d allowed=%d grade=%d quote=%d",pid,ti,allowed,grade,*quote);rm_event("NATIVE_QUOTE",pid,rm_state->p[pi].original,ti,*quote);
 if(!rm_snapshot_quiet(p,&after)||memcmp(&before,&after,sizeof(before))||!pilot_money(t,&am)||memcmp(&bm,&am,sizeof(bm))||capture_fault)return -1;
 rm_gate_inflight=0;if(allowed&&(grade<0||grade>12||*quote<0))return -1;return allowed;
}
static int rm_add_player(Rec p,int t,int arrival,Rec recruit,int hs){int id=pilot_get(p,228,2),pos=pilot_get(p,3,3);if(id<0||pos<0||pos>=RM_POS||rm_state->n>=RM_PLAYERS)return 0;
 if(rm_lookup(id)>=0)return 0;int i=rm_state->n++;RMPlayer*q=&rm_state->p[i];q->id=id;q->pos=pos;q->ovr=pilot_get(p,191,2);q->dev=pilot_get(p,271,3);q->year=pilot_get(p,242,3);q->injured=pilot_get(p,155,3)!=1;q->arrival=arrival;q->original=t;q->team=-1;q->user=pilot_get(p,0,1);q->pool=recruit!=0;q->hs=hs;q->paid=pilot_get(p,139,2);q->next=-1;
 if(q->ovr<0||q->ovr>99||q->dev<0||q->dev>3||q->year<0||q->year>3||q->paid<0)return 0;
 if(pos==19||pos==20){const unsigned fields[3]={166,165,125};for(int k=0;k<3;k++){int v=pilot_get(p,fields[k],2);if(v<0||v>99)return 0;rm_state->kicking[i][k]=v;}}
 int owner=pilot_get(p,261,2);if(t>=0&&(!arrival?owner!=t:owner!=255&&owner!=t))return 0;
 rm_refs[i]=(RMRef){p,recruit,0,0};if(!identity(p,&rm_refs[i].store,&rm_refs[i].row))return 0;if(!rm_index_put(id,i))return 0;if(t>=0)rm_link(rm_state,i,t);
 /* The two accepted R3 unsigned-HB limits remain held on that cohort only. */
 if(recruit&&q->paid)q->held=1;
 if(recruit&&rm_year==2027&&q->pos==1&&q->year==0&&((id==2341&&rm_refs[i].row==4040)||(id==1434&&rm_refs[i].row==4750)))q->held=1;
 return !capture_fault;
}
static int rm_configs(int tid){int ok=0;Rec config=reference(rm_teams[tid],64,&ok),off=0,def=0;if(!ok||!config)goto bad;off=reference(config,5,&ok);if(!ok||!off)goto bad;def=reference(config,0,&ok);if(!ok||!def)goto bad;
 const int oi[10]={5,2,1,9,8,4,3,0,6,7};const int di[9]={3,6,1,4,5,7,0,2,8};
 for(int p=0;p<RM_POS;p++){Rec parent=p<10?off:p<19?def:config;int slot=p<10?oi[p]:p<19?di[p-10]:p==19?1:p==20?6:4;Rec e=reference(parent,(unsigned)slot,&ok);if(!ok||!e){release(e);goto bad;}RMConfig*c=&rm_state->t[tid].config[p];int extra=pilot_get(e,0,2);c->depth=pilot_get(e,1,2);c->ideal=pilot_get(e,2,2);c->minimum=pilot_get(e,3,2);c->starters=pilot_get(e,4,2)+extra;release(e);if(c->depth<0||c->depth>30||c->ideal<0||c->ideal>30||c->minimum<0||c->minimum>30||c->starters<0||c->starters>12)goto bad;}
 release(def);release(off);release(config);return !capture_fault;
 bad:release(def);release(off);release(config);return 0;
}
static int rm_charts(int tid){int ok=0;Rec d=reference(rm_teams[tid],63,&ok);if(!ok||!d){release(d);return 0;}for(int pos=0;pos<RM_CHARTS;pos++){Rec a=array_field(d,(unsigned)pos,&ok);U n=array_count(a);if(!ok||n>30){release(a);release(d);return 0;}int keep=pos>=21?1:rm_state->t[tid].config[pos].starters;for(U j=0;j<n;j++){Rec p=array_at(a,j,&ok);if(!ok){release(p);release(a);release(d);return 0;}if(p){int id=pilot_get(p,228,2);if(id<0){release(p);release(a);release(d);return 0;}int i=rm_lookup(id);if(i>=0&&rm_state->p[i].original==tid&&pilot_same(p,rm_refs[i].player)){
 if((int)j<keep)rm_state->p[i].chart|=UINT64_C(1)<<pos;
 if(pos==RM_FB_POS && (!rm_state->p[i].fb_chart_depth || (int)j+1<rm_state->p[i].fb_chart_depth))
  rm_state->p[i].fb_chart_depth=(int)j+1;
 if(pos<21&&rm_job_unit(pos)==rm_job_unit(rm_state->p[i].pos)&&(!rm_state->p[i].chart_depth||(int)j+1<rm_state->p[i].chart_depth))rm_state->p[i].chart_depth=(int)j+1;
 }}release(p);}release(a);}release(d);return !capture_fault;}
static int rm_collect(void){rm_state=calloc(1,sizeof(*rm_state));rm_refs=calloc(RM_PLAYERS,sizeof(*rm_refs));if(!rm_state||!rm_refs)return 0;rm_state->gate=rm_native_gate;for(int i=0;i<65536;i++)rm_pid[i]=-1;for(int t=0;t<RM_TEAMS;t++)rm_state->t[t].head=-1;
 int ok=0;Rec teams=array_field(rm_franchise,45,&ok);if(!ok||array_count(teams)!=RM_TEAMS){release(teams);return 0;}
 for(int j=0;j<RM_TEAMS;j++){Rec t=array_at(teams,j,&ok);if(!ok||!t){release(t);release(teams);return 0;}int tid=pilot_get(t,383,2);if(tid<0||tid>=RM_TEAMS||rm_teams[tid]){release(t);release(teams);return 0;}rm_teams[tid]=t;RMTeam*q=&rm_state->t[tid];Rec user=reference(t,407,&ok);q->user=user!=0;release(user);if(!ok){release(teams);return 0;}q->academy=pilot_get(t,132,1);q->prestige=pilot_get(t,384,2);q->budget=pilot_get(t,227,2);if(q->prestige<0||q->prestige>10||q->budget<0){release(teams);return 0;}
  for(int slot=0;slot<2;slot++){Rec a=array_field(t,slot?33:239,&ok);U n=array_count(a);if(!ok||n>RM_ROOM){release(a);release(teams);return 0;}for(U k=0;k<n;k++){Rec p=array_at(a,k,&ok);if(!ok||!p||!rm_add_player(p,tid,slot,0,0)){/* Successfully retained records are owned by rm_refs. */if(p&&(rm_state->n==0||rm_refs[rm_state->n-1].player!=p))release(p);release(a);release(teams);return 0;}}release(a);}q->initial=q->count;
 }
 release(teams);for(int t=0;t<RM_TEAMS;t++)if(!rm_teams[t]||!rm_configs(t)||!rm_charts(t))return 0;
 for(int pool=0;pool<2;pool++){Rec a=array_field(rm_franchise,pool?46:39,&ok);U n=array_count(a);if(!ok||n>6000){release(a);return 0;}for(U j=0;j<n;j++){Rec r=array_at(a,j,&ok),p=r?reference(r,5,&ok):0;if(!ok||!p||!r){release(p);release(r);release(a);return 0;}int id=pilot_get(p,228,2),stage=pilot_get(r,9,3),klass=pilot_get(r,2,3);if(id<0){release(p);release(r);release(a);return 0;}
   int owned=rm_lookup(id)>=0;if(owned&&!pilot_same(p,rm_refs[rm_lookup(id)].player)){release(p);release(r);release(a);return 0;}
   if(owned||stage<0||stage>3||klass<0||klass>7||pilot_get(p,261,2)!=255||pilot_get(p,0,1)||pilot_get(p,155,3)!=1||!pilot_unheld(rm_franchise,p)){release(p);release(r);continue;}
   /* Require genuine HighSchool for academies; JUCO allowed only at ordinary CPUs. */
   int hs=!pool&&klass==0?1:!pool&&klass>=1&&klass<=3?2:0;
   if(!rm_add_player(p,-1,0,r,hs)){if(rm_state->n==0||rm_refs[rm_state->n-1].player!=p){release(p);release(r);}release(a);return 0;}
  }release(a);}
 for(int i=0;i<rm_state->n;i++)if(rm_state->p[i].pool&&!rm_state->p[i].held){int pos=rm_state->p[i].pos,g=((int(*)(int))(game_base+0x8598d10))(pos),u=((int(*)(int))(game_base+0x8598d20))(pos);PilotVector v;
 if(g<0||u<0||!pilot_vector(rm_manager,0,pos,rm_refs[i].player,&v)||!pilot_vector(rm_manager,1,g,rm_refs[i].player,&v)||!pilot_vector(rm_manager,2,u,rm_refs[i].player,&v)){rm_state->p[i].held=1;rm_event("UNSIGNED_NATIVE_BUCKET_UNAVAILABLE",rm_state->p[i].id,-1,-1,0);}}
 return !capture_fault;
}
static void rm_release_state(void){if(rm_refs&&rm_state)for(int i=0;i<rm_state->n;i++){release(rm_refs[i].player);release(rm_refs[i].recruit);}free(rm_refs);rm_refs=0;free(rm_cut_states);rm_cut_states=0;free(rm_expected_states);rm_expected_states=0;free(rm_expected_nil);rm_expected_nil=0;rm_audit_ready=0;if(rm_state){free(rm_state->quotes);free(rm_state->stale_edges);free(rm_state);}rm_state=0;for(int t=0;t<RM_TEAMS;t++){release(rm_teams[t]);rm_teams[t]=0;}release(rm_franchise);rm_franchise=0;}
/* Exact payment primitive: finance(player, payer, desired compensation, notify).
   It changes compensation only after native budget debit/refund succeeds. */
static int rm_pay(Rec p,Rec team,int target){U finance=0;PilotMoney before,after;int old=pilot_get(p,139,2);if(!local_read(rm_manager+0x1a0,&finance,8)||!finance||old<0||target<0||!pilot_money(team,&before)||before.value[3]<target-old||before.value[0]+target-old<0)return 0;
 ((void(*)(U,Rec*,Rec*,int,unsigned char))(game_base+0x82c1510))(finance,&p,&team,target,1);
 if(!pilot_money(team,&after)||pilot_get(p,139,2)!=target)return 0;for(int i=0;i<6;i++){int expected=before.value[i]+(i==0?target-old:i==3?old-target:0);if(after.value[i]!=expected)return 0;}return !capture_fault;
}
/* Record the exact rejected invariant, including expected/observed values.
   These receipts are diagnostics, never substitutes for the existing checks. */
static int rm_tx_failure(const RMAction*a,int ordinal,const char*check,const char*field,int expected,int actual){
 if(!rm_first_tx_issue[0]){
  snprintf(rm_first_tx_issue,sizeof(rm_first_tx_issue),"move %d player %d team %d to %d, %s %s expected %d observed %d",
           ordinal,rm_state->p[a->player].id,a->src,a->dst,check,field,expected,actual);
 }
 char b[640];snprintf(b,sizeof(b),"\"event\":\"RM17R3_CHECK_FAILED\",\"ordinal\":%d,\"pid\":%d,\"source\":%d,\"destination\":%d,\"check\":\"%s\",\"field\":\"%s\",\"expected\":%d,\"actual\":%d",ordinal,rm_state->p[a->player].id,a->src,a->dst,check,field,expected,actual);emit(b);FlushFileBuffers(log_file);return 0;
}
static int rm_tx_money(const RMAction*a,int ordinal,const char*stage,int tid,Rec team,const PilotMoney*before,int delta){
 PilotMoney after;if(!pilot_money(team,&after))return rm_tx_failure(a,ordinal,stage,"LEDGER_READ",1,0);
 char b[640];snprintf(b,sizeof(b),"\"event\":\"RM17R3_LEDGER\",\"ordinal\":%d,\"pid\":%d,\"team\":%d,\"stage\":\"%s\",\"native_fields\":{\"170\":%d,\"217\":%d,\"225\":%d,\"227\":%d,\"238\":%d,\"286\":%d}",ordinal,rm_state->p[a->player].id,tid,stage,after.value[0],after.value[1],after.value[2],after.value[3],after.value[4],after.value[5]);emit(b);
 const char*fields[6]={"NILSpent","Native217","Native225","NILRemaining","Native238","Native286"};
 for(int i=0;i<6;i++){int expected=before->value[i]+(i==0?delta:i==3?-delta:0);if(after.value[i]!=expected)return rm_tx_failure(a,ordinal,stage,fields[i],expected,after.value[i]);}return !capture_fault;
}
static int rm_tx_preserved(const RMAction*a,int ordinal,const char*stage,const PilotPlayerState*before,const PilotPlayerState*after){
 int errors=0;
 for(unsigned i=0;i<PILOT_SCALARS;i++){
  int old=before->value[i],now=after->value[i];
  if(old==now)continue;
  const RMField *field=&pilot_player_fields[i];
  switch(rm_transaction_field(field->index)){
  case RM_TEAM_PRESENTATION:
   log_line("RM18R22 TEAM_PRESENTATION move=%d pid=%d source=%d destination=%d stage=%s field=%s old=%d new=%d",
            ordinal,rm_state->p[a->player].id,a->src,a->dst,stage,field->name,old,now);
   break;
  case RM_PRESERVE_FIELD:
   rm_tx_failure(a,ordinal,stage,field->name,old,now);
   errors++;
   break;
  case RM_ROSTER_BOOKKEEPING:
   break;
  }
 }
 if(errors&&!rm18_deep()){
  pilot_log_snapshot(before,ordinal,0);
  pilot_log_snapshot(after,ordinal,1);
  FlushFileBuffers(log_file);
 }
 return errors==0&&!capture_fault;
}
static int rm_array_occurrences(Rec owner,unsigned field,Rec p,int *size){
 int ok=0,found=0,members=0;Rec a=array_field(owner,field,&ok);U n=array_count(a);
 if(!ok||n>10000){release(a);return -1;}
 for(U i=0;i<n;i++){Rec x=array_at(a,i,&ok);if(!ok){release(x);release(a);return -1;}if(x){members++;found+=pilot_same(x,p);}release(x);}
 /* Native pools may contain empty slots. Count live members so either append
    or reuse of an empty slot must add exactly one verified retired player. */
 if(size)*size=members;release(a);return found;
}
/* The native make-room path retires its selected player through PlayerManager
   method 44 (8615a50), not by erasing a player row. Use that same operation.
   Refund NIL first, so its contract event sees zero and cannot refund twice. */
static int rm_native_cut(RMAction*a,int ordinal){
 if(!rm_state||!rm_refs||!rm_cut_states||a->player<0||a->player>=rm_state->n||a->src<0||a->src>=RM_TEAMS||a->dst!=-1||a->phase!=RM_FINAL_CUT||ordinal<1||ordinal>rm_state->na)return 0;
 RMPlayer*q=&rm_state->p[a->player];Rec p=rm_refs[a->player].player,src=rm_teams[a->src],tm=0,pm=0,f=0,check=0;int ok=0,success=0,retired_before=0,total=0;
 PilotPlayerState before,after;PilotMoney money;int active=pilot_get(src,3,2),oldpay=pilot_get(p,139,2);
 #define CUT_REQUIRE(value,name) do{if(!(value)){rm_tx_failure(a,ordinal,name,"CHECK",1,0);goto done;}}while(0)
 #define CUT_EQUAL(value,want,name,field) do{int got_=(value),want_=(want);if(got_!=want_){rm_tx_failure(a,ordinal,name,field,want_,got_);goto done;}}while(0)
 CUT_REQUIRE(rm_open(rm_state,a->src)&&q->original==a->src&&q->team==-1&&q->moved&&!q->held&&!q->arrival&&!q->pool&&!q->user&&!q->injured,"CUT_POLICY_BOUNDARY");
 CUT_REQUIRE(active>0&&oldpay>=0&&!capture_fault&&rm_owned(p,a->src),"CUT_BEFORE_OWNERSHIP");
 for(int slot=0;slot<2;slot++){Rec arr=array_field(src,slot?33:239,&ok);U n=array_count(arr);release(arr);CUT_REQUIRE(ok&&n<=RM_ROOM,"CUT_SOURCE_ARRAY");total+=(int)n;}
 CUT_REQUIRE(total>90,"CUT_ONLY_ABOVE_90");
 CUT_EQUAL(rm_array_occurrences(rm_franchise,42,p,&retired_before),0,"CUT_BEFORE_POOL","RetiredOccurrences");
 CUT_EQUAL(rm_array_occurrences(rm_franchise,14,p,0),0,"CUT_BEFORE_POOL","FreeAgentOccurrences");
 tm=reference((Rec)rm_manager,24,&ok);CUT_REQUIRE(ok&&tm,"CUT_TEAM_MANAGER");
 pm=reference(tm,6,&ok);CUT_REQUIRE(ok&&pm,"CUT_PLAYER_MANAGER");
 f=reference(pm,6,&ok);CUT_REQUIRE(ok&&f&&pilot_same(f,rm_franchise),"CUT_FRANCHISE_IDENTITY");
 check=reference(pm,27,&ok);CUT_REQUIRE(ok&&check&&pilot_same(check,tm),"CUT_TEAM_MANAGER_IDENTITY");
 CUT_REQUIRE(pilot_snapshot(p,&before,ordinal,0)&&pilot_money(src,&money),"CUT_BEFORE_SNAPSHOT");
 rm_cut_states[ordinal-1]=before;
 rm_event("BEGIN_FINAL_CUT",q->id,a->src,-1,oldpay);FlushFileBuffers(log_file);CUT_REQUIRE(!capture_fault,"CUT_BEFORE_CAPTURE");rm_mutation_started=1;
 if(oldpay)CUT_REQUIRE(rm_pay(p,src,0),"CUT_REFUND");
 CUT_EQUAL(pilot_get(p,139,2),0,"CUT_BEFORE_RETIRE","CurrentNILCompensation");
 CUT_REQUIRE(rm_tx_money(a,ordinal,"CUT_AFTER_REFUND",a->src,src,&money,-oldpay),"CUT_REFUND_LEDGER");
 ((void(*)(Rec,Rec*,Rec*,unsigned char))(game_base+0x8615ab0))(pm,&p,&src,0);
 CUT_EQUAL(pilot_get(p,261,2),255,"CUT_AFTER_RETIRE","TeamIndex");
 CUT_EQUAL(pilot_get(p,230,2),a->src,"CUT_AFTER_RETIRE","PrevTeamIndex");
 CUT_EQUAL(pilot_get(p,139,2),0,"CUT_AFTER_RETIRE","CurrentNILCompensation");
 CUT_EQUAL(pilot_get(src,3,2),active-1,"CUT_AFTER_RETIRE","ActiveRosterSize");
 CUT_REQUIRE(rm_owned(p,-1),"CUT_AFTER_UNOWNED");
 int retired_after=0;CUT_EQUAL(rm_array_occurrences(rm_franchise,42,p,&retired_after),1,"CUT_AFTER_POOL","RetiredOccurrences");
 CUT_EQUAL(retired_after,retired_before+1,"CUT_AFTER_POOL","RetiredCount");
 CUT_EQUAL(rm_array_occurrences(rm_franchise,14,p,0),0,"CUT_AFTER_POOL","FreeAgentOccurrences");
 CUT_REQUIRE(pilot_snapshot(p,&after,ordinal,1),"CUT_FINAL_PLAYER");
 if(!rm_tx_preserved(a,ordinal,"CUT_PRESERVATION",&before,&after))goto done;
 CUT_REQUIRE(rm_tx_money(a,ordinal,"CUT_FINAL_SOURCE",a->src,src,&money,-oldpay),"CUT_FINAL_LEDGER");
 rm_event("FINAL_CUT_PASS_IMMEDIATE",q->id,a->src,-1,oldpay);success=!capture_fault;if(success)rm_cut_verified[ordinal-1]=1;
 done:release(check);release(f);release(pm);release(tm);return success;
 #undef CUT_EQUAL
 #undef CUT_REQUIRE
}
static int rm_native_execute(RMAction*a,int ordinal){
 if(a->phase==RM_FINAL_CUT)return rm_native_cut(a,ordinal);
 RMPlayer*q=&rm_state->p[a->player];Rec p=rm_refs[a->player].player,dst=rm_teams[a->dst],src=a->src>=0?rm_teams[a->src]:0;
 PilotPlayerState before,ownership,after;PilotMoney bm,bs;int active=pilot_get(dst,3,2),sa=src?pilot_get(src,3,2):0,oldpay=pilot_get(p,139,2),quote=-1;
 #define TX_REQUIRE(value,name) do{if(!(value))return rm_tx_failure(a,ordinal,name,"CHECK",1,0);}while(0)
 #define TX_EQUAL(value,want,name,field) do{int actual_=(value),expected_=(want);if(actual_!=expected_)return rm_tx_failure(a,ordinal,name,field,expected_,actual_);}while(0)
 TX_REQUIRE(rm_managed(rm_state,a->dst)&&(a->src<0||rm_managed(rm_state,a->src)),"USER_SCOPE_BOUNDARY");
 TX_REQUIRE(active>=0&&sa>=0&&oldpay>=0&&!capture_fault,"BEFORE_SCALARS");
 TX_REQUIRE(rm_owned(p,a->src),"BEFORE_UNIQUE_OWNERSHIP");
 TX_REQUIRE(pilot_snapshot(p,&before,ordinal,0),"BEFORE_PLAYER_SNAPSHOT");
 TX_REQUIRE(pilot_money(dst,&bm),"BEFORE_DESTINATION_LEDGER");if(src)TX_REQUIRE(pilot_money(src,&bs),"BEFORE_SOURCE_LEDGER");
 int gate=rm_native_gate(0,a->player,a->dst,&quote);
 /* Native eligibility and prices can change after an earlier signing. A
    clean requalification decline is not a failed ownership transaction.
    Return2 requests detached pending-plan refresh BEFORE this player moves.
    Unreadable/changed native state remains fatal, even on a denied edge. */
 TX_REQUIRE(gate>=0&&!capture_fault&&!rm_gate_inflight,"REQUALIFICATION_INTEGRITY");
 if(gate!=1||quote!=a->quote){
  PilotMoney check;TX_REQUIRE(!src||(pilot_money(src,&check)&&!memcmp(&bs,&check,sizeof(bs))),"REQUALIFICATION_SOURCE_LEDGER");
  char b[640];snprintf(b,sizeof(b),"\"event\":\"RM18R22_PLAN_STALE\",\"ordinal\":%d,\"pid\":%d,\"source\":%d,\"destination\":%d,\"gate\":%d,\"planned_quote\":%d,\"current_quote\":%d,\"player_mutated\":0",ordinal,q->id,a->src,a->dst,gate,a->quote,quote);emit(b);
  return capture_fault?0:2;
 }
 PilotVector vectors[3];if(q->pool){int g=((int(*)(int))(game_base+0x8598d10))(q->pos),u=((int(*)(int))(game_base+0x8598d20))(q->pos);
  TX_REQUIRE(g>=0&&u>=0&&pilot_vector(rm_manager,0,q->pos,p,&vectors[0])&&pilot_vector(rm_manager,1,g,p,&vectors[1])&&pilot_vector(rm_manager,2,u,p,&vectors[2]),"UNSIGNED_BUCKETS_BEFORE");TX_EQUAL(oldpay,0,"UNSIGNED_BEFORE","CurrentNILCompensation");}
 if(!src)TX_EQUAL(oldpay,0,"UNOWNED_BEFORE","CurrentNILCompensation");
 U tm=0;if(!q->pool){int ok=0;Rec check=reference((Rec)rm_manager,24,&ok);local_read(rm_manager+0x128,&tm,8);int valid=ok&&check&&tm&&pilot_same(check,(Rec)tm);release(check);TX_REQUIRE(valid,"TEAM_MANAGER_IDENTITY");}
 if(!rm_tx_money(a,ordinal,"BEFORE_DESTINATION",a->dst,dst,&bm,0)||(src&&!rm_tx_money(a,ordinal,"BEFORE_SOURCE",a->src,src,&bs,0)))return 0;
 rm_event("BEGIN_TRANSACTION",q->id,a->src,a->dst,quote);FlushFileBuffers(log_file);TX_REQUIRE(!capture_fault,"BEFORE_MUTATION_CAPTURE");rm_mutation_started=1;
 /* Native SignPlayer sends a contract event. TeamRevenueEval refunds the OLD
    team using the player's CURRENT compensation. Never install the new team's
    amount while that event can still see the old owner (RM17R1 double refund).
    Refund old -> ownership event at zero -> charge new. No direct ledger edits. */
 if(src&&oldpay)TX_REQUIRE(rm_pay(p,src,0),"SOURCE_REFUND");
 TX_EQUAL(pilot_get(p,139,2),0,"BEFORE_OWNERSHIP","CurrentNILCompensation");
 if(!rm_tx_money(a,ordinal,"BEFORE_SIGN_DESTINATION",a->dst,dst,&bm,0)||(src&&!rm_tx_money(a,ordinal,"BEFORE_SIGN_SOURCE",a->src,src,&bs,-oldpay)))return 0;
 if(q->pool){TX_REQUIRE(pilot_rotate(&vectors[0]),"UNSIGNED_BUCKET_ROTATION");((void(*)(U,Rec*,Rec*,unsigned))(game_base+0x81b5fe0))(rm_manager,&dst,&p,0);}
 else{Rec contract=0;((void(*)(U,Rec*,Rec*,Rec*))(game_base+0x85ae7a0))(tm,&dst,&p,&contract);release(contract);}
 TX_EQUAL(pilot_get(p,139,2),0,"AFTER_OWNERSHIP","CurrentNILCompensation");
 TX_EQUAL(pilot_get(p,261,2),a->dst,"AFTER_OWNERSHIP","TeamIndex");TX_REQUIRE(rm_owned(p,a->dst),"AFTER_UNIQUE_OWNERSHIP");
 TX_EQUAL(pilot_get(dst,3,2),active+1,"AFTER_OWNERSHIP","DestinationActiveRosterSize");
 if(src){TX_EQUAL(pilot_get(src,3,2),sa-1,"AFTER_OWNERSHIP","SourceActiveRosterSize");TX_EQUAL(pilot_get(p,230,2),a->src,"AFTER_OWNERSHIP","PrevTeamIndex");}
 TX_REQUIRE(rm_snapshot_quiet(p,&ownership),"AFTER_OWNERSHIP_PLAYER_READ");if(!rm_tx_preserved(a,ordinal,"OWNERSHIP_PRESERVATION",&before,&ownership))return 0;
 if(!rm_tx_money(a,ordinal,"AFTER_SIGN_DESTINATION",a->dst,dst,&bm,0)||(src&&!rm_tx_money(a,ordinal,"AFTER_SIGN_SOURCE",a->src,src,&bs,-oldpay)))return 0;
 if(q->pool)for(int i=0;i<3;i++)TX_REQUIRE(pilot_removed_vector(&vectors[i]),"UNSIGNED_BUCKET_REMOVAL");
 if(quote)TX_REQUIRE(rm_pay(p,dst,quote),"DESTINATION_PAYMENT");
 TX_REQUIRE(pilot_snapshot(p,&after,ordinal,1),"FINAL_PLAYER_SNAPSHOT");if(!rm_tx_preserved(a,ordinal,"FINAL_PRESERVATION",&ownership,&after))return 0;
 TX_EQUAL(pilot_get(p,139,2),quote,"FINAL_PAYMENT","CurrentNILCompensation");TX_EQUAL(pilot_get(p,261,2),a->dst,"FINAL_OWNERSHIP","TeamIndex");
 TX_REQUIRE(rm_owned(p,a->dst),"FINAL_UNIQUE_OWNERSHIP");TX_EQUAL(pilot_get(dst,3,2),active+1,"FINAL_OWNERSHIP","DestinationActiveRosterSize");
 if(src){TX_EQUAL(pilot_get(src,3,2),sa-1,"FINAL_OWNERSHIP","SourceActiveRosterSize");TX_EQUAL(pilot_get(p,230,2),a->src,"FINAL_OWNERSHIP","PrevTeamIndex");}
 if(!rm_tx_money(a,ordinal,"FINAL_DESTINATION",a->dst,dst,&bm,quote)||(src&&!rm_tx_money(a,ordinal,"FINAL_SOURCE",a->src,src,&bs,-oldpay)))return 0;
 if(q->pool)for(int i=0;i<3;i++)TX_REQUIRE(pilot_removed_vector(&vectors[i]),"FINAL_UNSIGNED_BUCKET_REMOVAL");
 rm_event("TRANSACTION_PASS_IMMEDIATE",q->id,a->src,a->dst,quote);return !capture_fault;
 #undef TX_EQUAL
 #undef TX_REQUIRE
}
static void rm_coverage_detail(const char *phase,int team,int pos,int listed,int required,const RMFullbackCover *cover,int kicking) {
 int credited=cover?cover->count:0;
 for(int j=0;j<credited;j++) {
  const RMFullbackMember *m=&cover->member[j];const RMPlayer *p=&rm_state->p[m->player];
  log_line("RM18R22 FB_CHART_COVER phase=%s team=%d pid=%d listed_position=%d fb_rank=%d",phase,team,p->id,p->pos,m->rank);
 }
 if(kicking>=0) {
  const RMPlayer *p=&rm_state->p[kicking];credited++;
  log_line("RM18R22 KICKING_CHART_COVER phase=%s team=%d pid=%d listed_position=%d chart_position=%d rank=1",phase,team,p->id,p->pos,pos);
 }
 log_line("RM18R22 COVERAGE_DETAIL phase=%s team=%d unit=%d natural=%d required=%d chart_cover=%d unresolved=%d",
          phase,team,pos,listed,required,credited,required-listed-credited);
 if(required>listed+credited)
  log_line("RM18R22 ROSTER_SHORTAGE phase=%s team=%d position=%d remaining=%d action=CONTINUE",phase,team,pos,required-listed-credited);
}
#include "final_audit.inc"
static void rm_handoff_capacity_if_clean(void) {
 if(rm_failed||capture_fault||rm_mutation_started||rm_gate_inflight||rm_native_diverged||rm_executed||
    (rm_state&&(rm_state->fault||rm_state->committed)))return;
 rm_fallback=rm_native_capacity_delegated=1;
 log_line("RM18R22 NATIVE_CAPACITY_HANDOFF; original roster and finances intact; native processing for this offseason");
}
#include "roster_limits.inc"
static void rm_cycle(U module){if(!enabled||capture_fault)return;if(rm_active){rm_abort("PREVIOUS_ENC_CYCLE_INCOMPLETE");return;}int ok=0,year=0;Rec f=reference((Rec)module,5,&ok);U store=0;uint32_t row=0;if(!ok||!f||!identity(f,&store,&row)||!rm_clock(f,&year)){release(f);rm_abort("STAGE_OR_FRANCHISE_CONTEXT");return;}
 if(rm_context_store==store&&rm_context_row==row&&rm_year==year){release(f);rm_active=1;rm_done=0;rm_validation_count=0;memset(rm_validated,0,sizeof(rm_validated));rm_event("DUPLICATE_CYCLE_BLOCKED",0,-1,-1,0);rm_abort("DUPLICATE_SAME_SEASON_CYCLE; restart with original branch");return;}
 rm_release_state();memset(rm_cut_verified,0,sizeof(rm_cut_verified));memset(rm_team_phase,0,sizeof(rm_team_phase));memset(rm_encouraged,0,sizeof(rm_encouraged));rm_end_complete=rm_encourage_started=rm_encourage_complete=0;rm_franchise=f;rm_context_store=store;rm_context_row=row;rm_year=year;rm_manager=module;rm_active=1;rm_native_capacity_delegated=rm_fallback=rm_mutation_started=rm_gate_inflight=rm_native_diverged=0;rm_done=rm_failed=rm_planned=rm_executed=rm_validation_count=0;memset(rm_validated,0,sizeof(rm_validated));rm_event("ANNUAL_CYCLE_BEGIN",0,-1,-1,year);
 (void)rm_require_roster_limits(RM_CAP_START);
}
static int rm_plan_ready(int revision){
 int over=0,under=0,gaps=0,cuts=0;for(int i=0;i<rm_state->na;i++)cuts+=rm_state->a[i].phase==RM_FINAL_CUT;
 for(int t=0;t<RM_TEAMS;t++){
  if(!rm_promises(rm_state,t)||rm_state->t[t].reserved>rm_state->t[t].budget){rm_abort("PLAN_PROMISE_OR_RESERVATION_FAILED");return 0;}
  if(!rm_managed(rm_state,t))continue;
  over+=rm_state->t[t].count>90;under+=rm_state->t[t].count<85;rm_event("PLANNED_TEAM_COUNT",0,t,t,rm_state->t[t].count);
  if(rm_state->t[t].user)log_line("RM18R22 USER_PLAN revision=%d team=%d initial_count=%d planned_count=%d fallback=%d",revision,t,rm_state->t[t].initial,rm_state->t[t].count,rm_state->user_fallback[t]);
  if(rm_state->t[t].count<85)log_line("RM18R22 FLOOR_EXCEPTION revision=%d team=%d count=%d target=85; qualifying affordable strict and bounded-flex additions exhausted",revision,t,rm_state->t[t].count);
  int kickers=rm_count(rm_state,t,RM_K_POS,0),punters=rm_count(rm_state,t,RM_P_POS,0);
  if(kickers+punters<3)log_line("RM18R22 SPECIALIST_ROOM revision=%d team=%d kickers=%d punters=%d roster=%d",revision,t,kickers,punters,rm_state->t[t].count);
  for(int p=0;p<RM_POS;p++)if(rm_unit(p)==p) {
   int count=rm_count(rm_state,t,p,1),required=rm_cfg(rm_state,t,p,3);
   if(count>=required)continue;
   RMFullbackCover cover={0};
   if(p==RM_FB_POS)rm_planned_fullback_cover(rm_state,t,required-count,-1,&cover);
   int kicking=count==0 && rm_kicking_unit(p)?rm_planned_kicking_cover(rm_state,t,p):-1;
   rm_coverage_detail(revision?"PLAN_REFRESH":"PLAN",t,p,count,required,&cover,kicking);
   gaps+=required-count-cover.count-(kicking>=0);
  }
 }
 log_line("RM18R22 PLAN revision=%d actions=%d committed=%d placements=%d final_cuts=%d under85_exceptions=%d over90=%d compatible_minimum_gaps=%d",revision,rm_state->na,rm_state->committed,rm_state->na-cuts,cuts,under,over,gaps);
 if(over){
  if(rm_state->committed)rm_abort("REVISED_ROSTER_ABOVE90_AFTER_COMMITTED_MOVES");
  else log_line("RM18R22 CAPACITY_HANDOFF_REQUEST over90=%d; no custom transactions started",over);
  return 0;
 }
 for(int i=rm_state->committed;i<rm_state->na;i++){RMAction*a=&rm_state->a[i];RMPlayer*p=&rm_state->p[a->player];char b[640];snprintf(b,sizeof(b),"\"event\":\"%s\",\"revision\":%d,\"ordinal\":%d,\"pid\":%d,\"source\":%d,\"destination\":%d,\"phase\":%d,\"role\":%d,\"position\":%d,\"ovr\":%d,\"class\":%d,\"dev\":%d,\"nil_quote\":%d",a->phase==RM_FINAL_CUT?"RM18R22_FINAL_CUT":"RM17R3_PLACEMENT",revision,i+1,p->id,a->src,a->dst,a->phase,a->role,p->pos,p->ovr,p->year,p->dev,a->quote);emit(b);}
 return !capture_fault;
}
static int rm_run_plan(void){
 int revision=0;
 while(rm_state->committed<rm_state->na){
  int i=rm_state->committed;
  if(capture_fault||rm_executed!=i){rm_abort("EXECUTION_PREFIX_OR_CAPTURE_FAILED");return 0;}
  rm_first_tx_issue[0]=0;
  int result=rm_native_execute(&rm_state->a[i],i+1);
  if(result==1){rm_executed++;rm_state->committed++;continue;}
  if(result!=2){rm_abort(rm_first_tx_issue[0]?rm_first_tx_issue:"TRANSACTION_OR_REQUALIFICATION_FAILED");return 0;}
  const RMAction *stale=&rm_state->a[i];
  int count=rm_note_stale(rm_state,stale->player,stale->dst);
  if(count<0){rm_abort("REQUALIFICATION_TRACKING_FAILED");return 0;}
  if(count==2)log_line("RM18R22 REQUALIFICATION_PAIR_SKIPPED pid=%d destination=%d; checking other homes and candidates",rm_state->p[stale->player].id,stale->dst);
  revision++;log_line("RM18R22 PLAN_REBUILD_BEGIN revision=%d committed=%d discarded_pending=%d",revision,i,rm_state->na-i);
  if(!rm_replan_remaining(rm_state)){rm_abort("PENDING_PLAN_REFRESH_FAILED");return 0;}
  if(!rm_plan_ready(revision)){rm_handoff_capacity_if_clean();return 0;}
  log_line("RM18R22 PLAN_REBUILD_READY revision=%d committed=%d pending=%d",revision,i,rm_state->na-i);
 }
 return !capture_fault;
}
static int rm_phase_team(U tp);
static void rm_before_enroll(U module,U tp){if(!enabled||!rm_active||rm_fallback||rm_planned||rm_failed||capture_fault)return;rm_planned=1;ULONGLONG rm18_started=GetTickCount64(),rm18_planned_ms=0;if(module!=rm_manager||(DWORD)observer_thread!=GetCurrentThreadId()){rm_abort("PREP_CALLBACK_CONTEXT_CHANGED_BEFORE_MUTATION");return;}__try{
 if(!rm_require_roster_limits(RM_CAP_ENROLL))__leave;
 if(!rm_collect()){rm_abort("SNAPSHOT_SCHEMA_OR_OWNERSHIP");__leave;}
 if(rm_phase_team(tp)<0)__leave;
 rm_select_user_fallback(rm_state);
 for(int t=0;t<RM_TEAMS;t++)if(rm_state->t[t].user){int reason=rm_state->user_fallback[t];
  log_line("RM18R22 USER_SCOPE team=%d initial_count=%d academy=%d fallback=%d under85=%d over90=%d compatible_minimum_gap=%d; %s",t,rm_state->t[t].initial,rm_state->t[t].academy,reason,!!(reason&RM_USER_UNDER85),!!(reason&RM_USER_OVER90),!!(reason&RM_USER_COVERAGE),reason?"CPU_RULES_FOR_THIS_ENC_PASS":"VALID_USER_ROSTER_UNTOUCHED");
 }
 if(!rm_audit_prepare()){
  char reason[420];snprintf(reason,sizeof(reason),"AUDIT_BASELINE_UNREADABLE_OR_INCONSISTENT %s; no custom transactions started",
        rm_first_audit_issue[0]?rm_first_audit_issue:"incomplete player or finance snapshot");
  rm_abort(reason);__leave;
 }
 if(!rm_plan(rm_state)){rm_abort("PLANNER_OR_NATIVE_GATE_FAILED");__leave;}
 rm18_planned_ms=GetTickCount64()-rm18_started;
 if(!rm_plan_ready(0)){
  rm_handoff_capacity_if_clean();
  __leave;
 }
 /* The suffix may gain cuts during refresh. Allocate once before mutation;
    completed cut snapshots keep their stable committed ordinal. */
 rm_cut_states=calloc(RM_ACTIONS,sizeof(*rm_cut_states));if(!rm_cut_states){rm_abort("CUT_PRESERVATION_ALLOCATION; no custom transactions started");__leave;}
 if(!rm_require_roster_limits(RM_CAP_COMMIT))__leave;
 if(!rm_run_plan())__leave;
 if(!rm_audit_seal()){rm_abort("POST_TRANSACTION_AUDIT_SEAL_FAILED");__leave;}
 log_line("RM18R22_ROSTER_TIMING plan_ms=%llu transactions_ms=%llu total_ms=%llu",(unsigned long long)rm18_planned_ms,(unsigned long long)(GetTickCount64()-rm18_started-rm18_planned_ms),(unsigned long long)(GetTickCount64()-rm18_started));
 rm_done=1;log_line("RM17R3 PREP_APPLIED transactions=%d; final native enrollment/charts and saved-state review still pending",rm_executed);
 }__except(EXCEPTION_EXECUTE_HANDLER){rm_abort("NATIVE_OR_PLANNER_EXCEPTION");}rm_failed_route();}
static int rm_block(U tp,unsigned site){
 if(!enabled||!rm_active)return 0;rm_failed_route();if(rm_fallback)return 0;
 Rec t=0;int ok=0;
 if(!local_read(tp,&t,8)||!t){rm_abort("TEAM_CALLBACK_UNREADABLE");rm_failed_route();return 0;}
 Rec user=reference(t,407,&ok);int isuser=user!=0;release(user);
 if(!ok){rm_abort("USER_BOUNDARY_UNREADABLE");rm_failed_route();return 0;}
 int id=pilot_get(t,383,2);if(id<0||id>=RM_TEAMS){rm_abort("TEAM_INDEX_UNREADABLE");rm_failed_route();return 0;}
 if(!rm_state||!rm_teams[id]||!pilot_same(t,rm_teams[id])||isuser!=rm_state->t[id].user){rm_abort("TEAM_RECORD_OR_USER_BOUNDARY_CHANGED");rm_failed_route();return 0;}
 int fallback_user=isuser&&rm_state&&rm_state->user_fallback[id];
 if(isuser&&!fallback_user&&site!=8&&site!=9)return 0;
 if(!rm_done){rm_abort("PREP_NOT_READY");rm_failed_route();return 0;}
 if(site==8||site==9||site==2||site==3){rm_native_diverged=1;rm_event(fallback_user?"UNPLANNED_FALLBACK_USER_FILL_OR_CUT_BLOCKED":isuser?"AUTOMATIC_USER_FILL_BLOCKED":"UNPLANNED_CPU_FILL_OR_CUT_BLOCKED",0,id,id,(int)site);return 1;}return 0;
}
static void rm_finish(void){if(!rm_active||!rm_done||rm_failed||capture_fault||!rm_state)return;
 __try{if(!rm_audit_final())rm_abort("FINAL_STATE_AUDIT_FAILED; see RM18R22_INVARIANT records for all observed failures");}
 __except(EXCEPTION_EXECUTE_HANDLER){rm_abort("FINAL_STATE_AUDIT_EXCEPTION");}
}
#include "phase_guards.inc"
static void rm_after_validate(U tp){if(!rm_active)return;Rec t=0;if(!local_read(tp,&t,8)||!t){rm_abort("VALIDATION_TEAM_UNREADABLE");rm_failed_route();return;}int id=pilot_get(t,383,2);if(id<0||id>=RM_TEAMS||rm_validated[id]){rm_abort("VALIDATION_TEAM_DUPLICATE_OR_INVALID");rm_failed_route();return;}rm_validated[id]=1;rm_validation_count++;
 if(rm_validation_count==RM_TEAMS){rm_audit_phase="AFTER_PRESEASON_VALIDATE";rm_finish();rm_failed_route();if(rm_native_capacity_delegated&&!rm_failed&&!capture_fault)
   rm_event("NATIVE_CAPACITY_CYCLE_COMPLETE",0,-1,-1,rm_validation_count);
  else rm_event(rm_fallback?"PREFLIGHT_DECLINED_NATIVE_CYCLE_COMPLETE":"ANNUAL_CYCLE_COMPLETE",0,-1,-1,rm_failed?-1:rm_executed);rm_release_state();rm_active=0;FlushFileBuffers(log_file);
  if(rm_fallback&&(!rm_native_capacity_delegated||rm_failed||capture_fault))MessageBoxW(0,L"Error code: ROSTER_PLAN_DECLINED\n\nFiles needed:\nCurrent session RM18R22 CSV, JSONL logs and receipt TXT.\nLast good Encourage Transfers save from before the error.",L"DRI+ RM18R22 - plan declined",MB_OK|MB_ICONWARNING);
 }
}
