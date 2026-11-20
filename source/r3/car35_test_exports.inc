#ifdef DRI_EMULATION_TEST
static car35_BuildContext car35_test_context;
__declspec(dllexport) void DriTestCar35Configure(U base,int active) {
 game_base=base;mode=active;car35_tls_index=70;car35_gate_tls=71;car35_generation_tls=72;car35_emergency_tls=73;
 TlsSetValue(70,NULL);TlsSetValue(71,NULL);TlsSetValue(72,NULL);TlsSetValue(73,NULL);car35_last_module=NULL;
}
__declspec(dllexport) int DriTestCar35Identity(void *c,void *t) {return car35_identity(c,t,"fixture");}
__declspec(dllexport) int DriTestCar35Eligible(void *c,void *t,int r) {return car35_eligible(c,t,r,"fixture");}
__declspec(dllexport) void DriTestCar35Context(void *m,void *t,int destination,int enabled) {
 memset(&car35_test_context,0,sizeof(car35_test_context));
 car35_test_context.module=m;car35_test_context.team=t;car35_test_context.destination=destination;
 car35_test_context.cap=240;car35_test_context.merit_enabled=enabled;car35_test_context.id=1;
 car35_test_context.coordinator_enabled=enabled && (destination==1 || destination==2);
 car35_test_context.parity_enabled=enabled;
 car35_test_context.destination_conference=-2;
 TlsSetValue(car35_tls_index,&car35_test_context);
}
__declspec(dllexport) void DriTestCar35Raw(void *c,int raw) {
 car35_CandidateMeta *m=car35_meta_for(&car35_test_context,c,1);if(m){m->raw=raw;m->raw_valid=1;}
}
__declspec(dllexport) int DriTestCar35Budget(int original,U caller) {return car35_coordinator_budget(&car35_test_context,original,caller);}
__declspec(dllexport) int DriTestCar35Conference(void *m,void *t) {return car35_hc_destination_conference(m,t);}
__declspec(dllexport) int DriTestCar35HCMove(void *m,void *c,void *old,void *t,int role) {return car35_hc_move_allowed(m,c,old,t,role,-2,"fixture");}
__declspec(dllexport) int DriTestCar35Population(void) {
 return car35_test_context.population=car35_competition_population(car35_test_context.module);
}
__declspec(dllexport) int DriTestCar35Competition(void *coach,int field) {
 car35_CandidateMeta *m=car35_meta_for(&car35_test_context,coach,0);if(!m)return -1;
 switch(field) {
 case 0:return m->qualified;case 1:return m->support_applied;case 2:return m->post_pass;
 case 3:return m->tier;case 4:return m->broad_seen;case 5:return m->broad_pass;
 default:return -1;
 }
}
__declspec(dllexport) int DriTestCar35Compare(void *a,void *b) {
 car35_test_context.sort_active=1;int r=car35_compare(a,b);car35_test_context.sort_active=0;return r;
}
__declspec(dllexport) U DriTestCar35Address(int id) {
 switch(id) {
 case 0:return (U)&car35_broad_adapter;
 case 1:return (U)&car35_reconstruct;
 case 2:return (U)&car35_user_assign;
 case 3:return (U)&car35_identity_writer;
 case 4:return (U)&car35_generation;
 case 5:return (U)&car35_cleanup_pool;
 case 6:return (U)&car35_contract_remove_adapter;
 case 7:return (U)&car35_contract_clear_adapter;
 case 8:return (U)&car35_user_validate_adapter;
 case 9:return (U)&car35_talent_gate_adapter;
 case 10:return (U)&car35_auto_progress;
 case 11:return (U)&car35_talent_purchase;
 case 12:return (U)&car35_generated_batch;
 case 13:return (U)&car35_opening;
 case 14:return (U)&car35_entrant_role;
 case 15:return (U)&car35_generation_count_adapter;
 case 16:return (U)&car35_entrant_profile;
 case 17:return (U)&car35_parity_rbx_adapter;
 case 18:return (U)&car35_parity_rdi_adapter;
 case 19:return (U)&car35_parity_rsi_adapter;
 default:return 0;
 }
}
__declspec(dllexport) int DriTestCar35Verify(U base) {game_base=base;return car35_verify();}
__declspec(dllexport) int DriTestCar35Install(U base) {game_base=base;return car35_install_all();}
#endif
#include "car35_emergency_test.inc"
