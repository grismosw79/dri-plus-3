/* Engine services below are fixtures. The production scope, builder hook,
   promotion guards and signing hook execute unchanged in the PE emulator. */
#ifdef DRI_EMULATION_TEST
static void **car35_e_test_pool;
static int car35_e_test_count,car35_e_test_flags,car35_e_test_stats[32];
static unsigned char car35_e_test_rows[32][48];
static void *car35_e_test_other_team;
__declspec(dllexport) void DriTestEmergencySet(void **pool,int n,int flags,void *other_team) {
    car35_e_test_pool=pool;car35_e_test_count=n;car35_e_test_flags=flags;
    car35_e_test_other_team=other_team;memset(car35_e_test_stats,0,sizeof(car35_e_test_stats));
}
__declspec(dllexport) int DriTestEmergencyStat(int n) {return n>=0 && n<32?car35_e_test_stats[n]:-1;}
__declspec(dllexport) int DriTestEmergencyFloor(void *m,void *t,void *c,int role) {
    return car35_emergency_candidate_floor(m,t,c,role,TlsGetValue(car35_tls_index));
}
__declspec(dllexport) void DriTestEmergencyNativeBuilder(void *m,void **tr,int role,void *out,unsigned char a5,unsigned char a6,int points) {
    (void)a5;(void)points;
    Car35Emergency *e=car35_emergency_context();
    int visit=car35_e_test_stats[0]++;
    if(visit<3)car35_e_test_stats[1+visit]=e && e->building?e->floor:6;
    void *build=TlsGetValue(car35_tls_index);
    /* A nested out-of-scope build must not borrow the parent tier. */
    if(e && e->building && car35_e_test_count) {
        TlsSetValue(car35_tls_index,NULL);
        if(car35_emergency_candidate_floor(m,*tr,car35_e_test_pool[0],role,TlsGetValue(car35_tls_index))==6)car35_e_test_stats[10]++;
        TlsSetValue(car35_tls_index,build);
    }
    int count=0;
    for(int i=0;i<car35_e_test_count;i++) {
        void *c=car35_e_test_pool[i],*old=NULL;
        if(a6 && car35_get(c,12,0)!=7)continue; /* native emergency pool is free agents */
        if(car35_broad_hook(m,&c,tr,&old,1,0,role)) {
            unsigned char *row=car35_e_test_rows[count++];memset(row,0,48);
            memcpy(row,&c,8);int score=car35_get(c,14,2);memcpy(row+24,&score,4);
        }
    }
    void **v=out;v[0]=car35_e_test_rows;v[1]=car35_e_test_rows+count;v[2]=car35_e_test_rows+32;
    if(car35_e_test_flags&1)v[1]=(char*)v[0]+1;
    if(car35_e_test_flags&2) {car35_BuildContext *c=TlsGetValue(car35_tls_index);if(c)c->bad=1;}
    if(car35_e_test_flags&4) {void *nil=NULL;memcpy((char*)m+0x28,&nil,8);}
}
__declspec(dllexport) unsigned char DriTestEmergencyNativeFill(void *m,void **tr,unsigned char fill,unsigned char report) {
    (void)report;car35_e_test_stats[4]++;
    if(!fill)return 0;
    void *v[4]={NULL,NULL,NULL,NULL};
    /* This thunk contains a call at the captured emergency builder site, so
       the production return-address scope check is exercised. */
    ((void(*)(void*,void**,int,void*,unsigned char,unsigned char,int))(game_base+0x08336290))(m,tr,0,v,0,1,100);
    U b=0,e=0;
    if(!car35_emergency_vector(v,&b,&e) || b==e)return 0;
    void *coach=*(void**)b;
    void *offer=((void*(*)(void*,void*))(game_base+0x00200100))(coach,*tr);
    void *manager=NULL;car35_local_read((U)m+0x178,&manager,8);
    if(car35_e_test_flags&8) {
        Car35Emergency *before=car35_emergency_context();
        car35_fill_staff_hook(m,&car35_e_test_other_team,0,0);
        car35_e_test_stats[12]=before==car35_emergency_context();
    }
    car35_e_test_stats[5]=car35_emergency_sign_floor(manager,*tr,coach,offer,0,0x08361e14);
    car35_e_test_stats[6]=car35_emergency_sign_floor(manager,*tr,coach,offer,0,0x0834ae7f);
    car35_e_test_stats[7]=car35_emergency_sign_floor(manager,car35_e_test_other_team,coach,offer,0,0x08361e14);
    car35_e_test_stats[8]=car35_emergency_sign_floor(manager,*tr,coach,offer,1,0x08361e14);
    ((void(*)(void*,void**,void**,unsigned char))(game_base+0x08361df0))(manager,tr,&offer,1);
    Car35Emergency *scope=car35_emergency_context();
    car35_e_test_stats[9]=scope?scope->consumed:0;
    car35_e_test_stats[11]=car35_emergency_sign_floor(manager,*tr,coach,offer,0,0x08361e14);
    return 1;
}
#endif
