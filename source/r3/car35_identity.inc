/* Persistent identity uses the existing serialized Coach.SpecialtyType.
   No career statistics, attributes, schemes or generated histories are added. */
static int car35_hc_destination_conference(void *module,void *team);
static int car35_hc_move_allowed(void *module,void *coach,void *old_team,void *team,int role,int conference,const char *path);
static int car35_get(void *r,unsigned f,int kind) {
    int value=-1;car35_record_field(r,f,kind,&value);return value;
}
static uint32_t car35_row(void *r) {
    uint32_t v=UINT32_MAX;if(r)car35_local_read((U)r+0x18,&v,4);return v&0x1ffffu;
}
static int car35_set_coach_enum(void *coach,unsigned member,int value) {
    U vt,getter,type,f,data,dv,writer;uint32_t id;int16_t count;
    if(value<0 || !coach || !car35_local_read((U)coach,&vt,8) ||
       !car35_local_read(vt+0x38,&getter,8) || !car35_code_pointer(getter))return 0;
    type=((U(*)(void*))getter)(coach);
    if(!type || !car35_local_read(type+0xd8,&count,2) || count<0 || (unsigned)count<=member)return 0;
    f=((U(*)(void*,unsigned))(game_base+0x08055d60))((void*)type,member);
    if(f==0xffffffffu || !car35_local_read((U)coach+0x10,&data,8) || !data ||
       !car35_local_read((U)coach+0x18,&id,4) || !car35_local_read(data,&dv,8) ||
       !car35_local_read(dv+0x148,&writer,8) || !car35_code_pointer(writer))return 0;
    ((void(*)(void*,uint32_t,U,int))writer)((void*)data,id,f,value);
    ((void(*)(void*))(game_base+0x080595e0))((void*)data);
    return car35_get(coach,member,0)==value;
}
static int car35_set_side(void *coach,int value) {
    return value>=0 && value<=1 && car35_set_coach_enum(coach,134,value);
}
/* A verified occupied coordinator slot is the migration authority. Otherwise
   a known side is immutable; Any may use actual current/previous OC/DC role. */
static int car35_identity(void *coach,void *current_team,const char *path) {
    int side=car35_get(coach,134,0),role=car35_get(coach,16,0),prev=car35_get(coach,17,0);
    int desired=side,bound=0;
    if(current_team && (role==1 || role==2)) {
        void *incumbent=car35_retain_reference(current_team,role==1?183:58);
        bound=incumbent==coach;car35_release_reference(incumbent);
    }
    if(bound)desired=role-1;
    else if(side==2 && (role==1 || role==2))desired=role-1;
    else if(side==2 && role==0 && (prev==1 || prev==2))desired=prev-1;
    if(desired!=side && desired>=0 && desired<=1) {
        int written=mode==1 && car35_set_side(coach,desired);
        log_line("CAR35_IDENTITY path=%s coach_row=%lu role=%d prev_role=%d old_side=%d proposed_side=%d verified_assignment=%d written=%d mode=%ld",
            path,(unsigned long)car35_row(coach),role,prev,side,desired,bound,written,mode);
        if(written)side=desired;
    }
    return side;
}
static int car35_eligible_side(int destination,int side) {
    return destination==0 || (destination==1 && side==0) || (destination==2 && side==1);
}
static int car35_eligible_at_floor(void *coach,void *current_team,int destination,const char *path,int floor) {
    int side=car35_identity(coach,current_team,path),ok=car35_eligible_side(destination,side);
    if(!ok)log_line("CAR35_EXCLUDE path=%s coach_row=%lu destination=%d side=%d reason=%s mode=%ld",
        path,(unsigned long)car35_row(coach),destination,side,side==2?"unknown_identity":"opposite_or_invalid_identity",mode);
    int role=car35_get(coach,16,0),previous=car35_get(coach,17,0);
    int source=car35_promotion_source_role(role,previous),promotion_ok=1;
    if(destination==0 && (source==1 || source==2)) {
        int grade=car35_get(coach,112,0);
        promotion_ok=floor==6?car35_promotion_grade_allowed(destination,role,previous,grade):
            car35_promotion_grade_at_floor(destination,role,previous,grade,floor);
        log_line("CAR35_PROMOTION_GATE path=%s coach_row=%lu destination=%d current_role=%d prev_role=%d source_role=%d status=%d prestige=%d minimum_grade=%d eligible=%d returned=%d reason=%s mode=%ld",
            path,(unsigned long)car35_row(coach),destination,role,previous,source,car35_get(coach,12,0),grade,
            floor,promotion_ok,mode!=1 || promotion_ok,promotion_ok?"grade_allowed":"below_floor_or_invalid_grade",mode);
    }
    return mode!=1 || (ok && promotion_ok);
}
static int car35_eligible(void *coach,void *current_team,int destination,const char *path) {
    return car35_eligible_at_floor(coach,current_team,destination,path,6);
}
/* Seven native parameters, including the final optional-points structure.
   All four reconstruction call sites share this ABI. Reject before cleanup,
   budget writes, assignment or contract rebuilding; never skip a void signer. */
static unsigned char car35_reconstruct(void *m,void **team,void **coach,int role,U a5,U a6,U a7) {
    if(coach && *coach && !car35_eligible(*coach,NULL,role,"reconstruct"))return 0;
    if(coach && *coach && !car35_hc_move_allowed(m,*coach,NULL,team?*team:NULL,role,-2,"reconstruct"))return 0;
    return ((unsigned char(*)(void*,void**,void**,int,U,U,U))(game_base+0x0834a8e0))(m,team,coach,role,a5,a6,a7);
}
/* This one virtual call is the native Any -> destination-preference writer.
   Known identities are already resolved before reconstruction. Leaving Any
   intact prevents an HC job from manufacturing offensive/defensive history. */
static void car35_identity_writer(void *data,uint32_t id,void *f,int value) {
    if(mode!=1) {
        U vt=0,writer=0;
        if(car35_local_read((U)data,&vt,8) && car35_local_read(vt+0x148,&writer,8) && car35_code_pointer(writer))
            ((void(*)(void*,uint32_t,void*,int))writer)(data,id,f,value);
    } else log_line("CAR35_IDENTITY_PRESERVE row=%lu native_proposed_side=%d reason=do_not_invent_identity_at_assignment",(unsigned long)(id&0x1ffff),value);
}
static unsigned char car35_user_assign(void *m,void **coach,void **team) {
    int role=coach && *coach?car35_get(*coach,16,0):-1;
    if(!coach || !*coach || !car35_eligible(*coach,NULL,role,"user_assignment"))return 0;
    return ((unsigned char(*)(void*,void**,void**))(game_base+0x08830ae0))(m,coach,team);
}
/* The direct assignment caller ignores its return value. Check at its earlier
   native validation boundary, before character creation/team mutation. RDI is
   the old coach reference, R9 the team reference, and argument 7 the success
   flag consumed immediately by the native caller's failure branch. */
__attribute__((naked,used)) static void car35_user_validate_adapter(void) {
    __asm__ volatile("sub $0x58, %rsp\n\t"
      "mov 0x80(%rsp), %rax\n\tmov %rax, 0x20(%rsp)\n\t"
      "mov 0x88(%rsp), %rax\n\tmov %rax, 0x28(%rsp)\n\t"
      "mov 0x90(%rsp), %rax\n\tmov %rax, 0x30(%rsp)\n\t"
      "mov %rdi, 0x38(%rsp)\n\tcall car35_user_validate\n\t"
      "add $0x58, %rsp\n\tret\n\t");
}
__declspec(dllexport) int car35_user_validate(void *m,U request,int key,void **team,unsigned char flag,U response,unsigned char *success,void **coach) {
    if(coach && *coach && ((unsigned char(*)(void*,void**))(game_base+0x088057f0))(m,coach)) {
        int role=car35_get(*coach,16,0);
        if(!car35_eligible(*coach,team?*team:NULL,role,"user_prevalidation")) {
            if(success)*success=0;return 0;
        }
    }
    return ((int(*)(void*,U,int,void**,unsigned char,U,unsigned char*))(game_base+0x088384b0))(m,request,key,team,flag,response,success);
}
