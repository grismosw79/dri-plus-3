/* Exact current-title sites, matched against both accepted capture identities. */
typedef struct { unsigned rva,size; unsigned char expected[8]; U entry; } Car35Call;
static const Car35Call car35_calls[]={
 {0x08361abf,5,{0xe8,0xac,0x00,0x00,0x00},(U)&car35_fill_staff_hook},
 {0x083354ec,5,{0xe8,0x7f,0xc6,0x02,0x00},(U)&car35_fill_staff_hook},
 {0x0816bbcd,5,{0xe8,0xfe,0x6f,0xfe,0xff},(U)&r2_before_actions},
 {0x08170f96,5,{0xe8,0x75,0xa3,0x03,0x00},(U)&t36_hours_published},
 {0x08437c8f,5,{0xe8,0x3c,0x91,0xe8,0xff},(U)&t36_effective_nil},
 {0x0816e5ee,5,{0xe8,0x7d,0x40,0x03,0x00},(U)&t36_initial_interest},
 {0x0842d107,5,{0xe8,0x54,0xc0,0xff,0xff},(U)&t36_transfers},
 {0x0816ff89,5,{0xe8,0xe2,0x15,0xff,0xff},(U)&t36_prep_adapter},
 {0x08156791,5,{0xe8,0x7a,0xd3,0x6a,0xfb},(U)&t36_instant_roll},
 {0x0842db17,5,{0xe8,0xc4,0x10,0x00,0x00},(U)&t36_plv},
 {0x082e801c,5,{0xe8,0x2f,0xfa,0xfc,0xff},(U)&t36_retention_pay},
 {0x0842da77,5,{0xe8,0xf4,0x0c,0x00,0x00},(U)&t36_nfl},
 {0x0842ea65,5,{0xe8,0xc6,0x50,0x00,0x00},(U)&t36_nfl_chance},
 {0x0842f381,5,{0xe8,0x5a,0x3d,0x00,0x00},(U)&t36_leave_grade},
 {0x08330f98,6,{0x8b,0x53,0x18,0x41,0xff,0xd1},(U)&car35_parity_rbx_adapter},
 {0x0834f024,6,{0x8b,0x53,0x18,0x41,0xff,0xd1},(U)&car35_parity_rbx_adapter},
 {0x08345b42,6,{0x8b,0x57,0x18,0x41,0xff,0xd1},(U)&car35_parity_rdi_adapter},
 {0x08345df0,6,{0x8b,0x57,0x18,0x41,0xff,0xd1},(U)&car35_parity_rdi_adapter},
 {0x08345e29,6,{0x8b,0x57,0x18,0x41,0xff,0xd1},(U)&car35_parity_rdi_adapter},
 {0x08345f48,6,{0x8b,0x56,0x18,0x41,0xff,0xd1},(U)&car35_parity_rsi_adapter},
 {0x0832c7d8,6,{0x8b,0x57,0x18,0x41,0xff,0xd1},(U)&car35_parity_rdi_adapter},
 {0x0817aa29,7,{0x41,0x8b,0x57,0x18,0x41,0xff,0xd1},(U)&car35_generation_count_adapter},
 {0x082b6b06,5,{0xe8,0xe5,0x7e,0x02,0x00},(U)&car35_talent_gate_adapter},
 {0x082ba8ea,5,{0xe8,0xc1,0x61,0x02,0x00},(U)&car35_talent_purchase},
 {0x0817ac5c,5,{0xe8,0xef,0xc7,0xff,0xff},(U)&car35_generated_batch},
 {0x082bad73,5,{0xe8,0x78,0xf7,0xff,0xff},(U)&car35_auto_progress},
 {0x082badbd,5,{0xe8,0x2e,0xf7,0xff,0xff},(U)&car35_auto_progress},
 {0x082bae07,5,{0xe8,0xe4,0xf6,0xff,0xff},(U)&car35_auto_progress},
 {0x082bae51,5,{0xe8,0x9a,0xf6,0xff,0xff},(U)&car35_auto_progress},
 {0x0835b336,5,{0xe8,0xb5,0xf1,0xf5,0xff},(U)&car35_auto_progress},
 {0x08326716,5,{0xe8,0x35,0x09,0x00,0x00},(U)&car35_opening},
 {0x0832f0df,5,{0xe8,0x6c,0x7f,0xff,0xff},(U)&car35_opening},
 {0x083369c2,5,{0xe8,0x89,0x06,0xff,0xff},(U)&car35_opening},
 {0x08354ecc,5,{0xe8,0x7f,0x21,0xfd,0xff},(U)&car35_opening},
 {0x0835509f,5,{0xe8,0xac,0x1f,0xfd,0xff},(U)&car35_opening},
 {0x08342be6,5,{0xe8,0x95,0xc4,0xac,0xff},(U)&car35_school_curve_hook},
 {0x08330f0c,5,{0xe8,0x8f,0xdf,0x01,0x00},(U)&car35_gate_hook},
 {0x08345c8d,5,{0xe8,0x7e,0x69,0xfe,0xff},(U)&car35_score_hook},
 {0x083462e4,5,{0xe8,0x07,0xd6,0xfd,0xff},(U)&car35_sort_hook},
 {0x08329414,5,{0xe8,0x57,0xc4,0x01,0x00},(U)&car35_builder_hook},
 {0x083362d3,5,{0xe8,0x98,0xf5,0x00,0x00},(U)&car35_builder_hook},
 {0x08352cd8,5,{0xe8,0x93,0x2b,0xff,0xff},(U)&car35_builder_hook},
 {0x0892df49,5,{0xe8,0x22,0x79,0xa1,0xff},(U)&car35_builder_hook},
 {0x083364b4,5,{0xe8,0x97,0xe6,0x00,0x00},(U)&car35_length_hook},
 {0x0833a734,5,{0xe8,0x17,0xa4,0x00,0x00},(U)&car35_length_hook},
 {0x08344b35,5,{0xe8,0x16,0x00,0x00,0x00},(U)&car35_length_hook},
 {0x08362a7d,5,{0xe8,0xce,0x20,0xfe,0xff},(U)&car35_length_hook},
 {0x083631c4,5,{0xe8,0x87,0x19,0xfe,0xff},(U)&car35_length_hook},
 {0x087dd885,5,{0xe8,0xc6,0x72,0xb6,0xff},(U)&car35_length_hook},
 {0x0834ae7a,5,{0xe8,0x21,0x3f,0x26,0x00},(U)&car35_sign_hook},
 {0x08361e0f,5,{0xe8,0x8c,0xcf,0x24,0x00},(U)&car35_sign_hook},
 {0x08361f02,5,{0xe8,0x99,0xce,0x24,0x00},(U)&car35_sign_hook},
 {0x08361fe8,5,{0xe8,0xb3,0xcd,0x24,0x00},(U)&car35_sign_hook},
 {0x08830cce,5,{0xe8,0xcd,0xe0,0xd7,0xff},(U)&car35_sign_hook},
 {0x08356c3a,5,{0xe8,0x41,0xbb,0xcf,0xff},(U)&car35_contract_remove_adapter},
 {0x0832e108,5,{0xe8,0x43,0x91,0xd1,0xff},(U)&car35_contract_clear_adapter},
 {0x08327eb7,5,{0xe8,0xf4,0x68,0x11,0x00},(U)&car35_contract_max_hook},
 {0x0834aae4,5,{0xe8,0xb7,0x45,0xff,0xff},(U)&car35_contract_lookup_hook},
 {0x08364730,5,{0xe8,0xcb,0x24,0xff,0xff},(U)&car35_contract_withdraw_hook},
 {0x08345b6d,5,{0xe8,0xbe,0xb2,0xfe,0xff},(U)&car35_broad_adapter},
 {0x08338a7c,5,{0xe8,0x5f,0x1e,0x01,0x00},(U)&car35_reconstruct},
 {0x08338ee4,5,{0xe8,0xf7,0x19,0x01,0x00},(U)&car35_reconstruct},
 {0x0836410a,5,{0xe8,0xd1,0x67,0xfe,0xff},(U)&car35_reconstruct},
 {0x0893eaed,5,{0xe8,0xee,0xbd,0xa0,0xff},(U)&car35_reconstruct},
 {0x0883b78e,5,{0xe8,0x1d,0xcd,0xff,0xff},(U)&car35_user_validate_adapter},
 {0x0883b961,5,{0xe8,0x7a,0x51,0xff,0xff},(U)&car35_user_assign},
 {0x0816caa7,5,{0xe8,0x04,0xdf,0x00,0x00},(U)&car35_generation},
 {0x0860732f,5,{0xe8,0x4c,0xfb,0xff,0xff},(U)&car35_cleanup_pool},
 {0x086073c6,5,{0xe8,0xb5,0xfa,0xff,0xff},(U)&car35_cleanup_pool},
 {0x0834ae0e,7,{0x41,0xff,0x92,0x48,0x01,0x00,0x00},(U)&car35_identity_writer},
};
#define CAR35_CALL_COUNT ((int)(sizeof(car35_calls)/sizeof(car35_calls[0])))
typedef struct { unsigned rva,size; uint64_t fnv; } Car35Guard;
static const Car35Guard car35_guards[]={
 {0x083361c0,1233,0xbb637eb97f7c7b07ULL}, /* emergency_offer_selection_full */
 {0x08362220,293,0x89d6ba60081de88dULL}, /* emergency_HC_offer_helper */
 {0x08361b70,1397,0x5d31740d9d5b5bfcULL}, /* automatic_fill_staff_full */
 {0x08147960,782,0xc2134cd779d058baULL}, /* native add recruiting action and charge board hours */
 {0x08191440,93,0x43724272d572e33dULL}, /* native assigned hours writer */
 {0x0816ba90,353,0x587ec62c5f99b539ULL}, /* per-school action dispatcher */
 {0x08152bd0,83,0x6ab6353bb142a4aeULL}, /* native pre-action entry */
 {0x08170f40,86,0xfefb691bf4036819ULL}, /* hours dispatcher queries DYRM and forwards manager/event */
 {0x0816fe25,35,0x733cb70ccac5e8c6ULL}, /* prep dispatcher queries RcEv and retains RBX */
 {0x0816ff82,7,0x72aae1563e96b755ULL}, /* prep passes evaluator-owned manager */
 {0x0816e4f8,41,0x95be276d9f657ed3ULL}, /* initial-interest dispatcher queries RcEv */
 {0x081ab310,2006,0x98d71b96e0224222ULL}, /* complete native all-schools hours publisher */
 {0x082e5980,1024,0xb3f214e7617e75daULL}, /* NIL charge */
 {0x082e6780,384,0x68103b7e99c2f5a2ULL}, /* NIL refund */
 {0x0819ee10,1120,0xf35c5d92b4da402dULL}, /* native scholarship */
 {0x08156380,1430,0x691168efd49a423bULL}, /* immediate commitment */
 {0x082c0c40,384,0x38f0b5da7ffa7543ULL}, /* recruit price */
 {0x082c0dd0,216,0xcb2ac88385a4c302ULL}, /* departure price */
 {0x0843eca0,136,0x1349e68a85c2edfdULL}, /* LeavingPlayer factory */
 {0x0842ebe0,256,0x4c9cadf814f423ceULL}, /* PLV evaluator */
 {0x08429160,256,0x92a79768cc21d9e1ULL}, /* portal population */
 {0x081a2670,256,0x407f8b5e53915eb7ULL}, /* initial interest */
 {0x0842e770,240,0x161e2fd4cf29aa5ULL}, /* NFL entry */
 {0x082b7a50,256,0x82fd5d32e810432ULL}, /* retention spending */
 {0x08437c80,75,0x45afd3b1abd6fe7bULL}, /* effective price comparison */
 {0x8607700,542,0xe661c4d8c8a954aaULL}, /* native_single_coach_remove */
 {0x8177450,1285,0x82865c1f27a070dULL}, /* native_batch_initialize_full */
 {0x817a9b0,1072,0x9e54ef9595aabcbeULL}, /* native_generation_full */
 {0x0833bd00,1544,0xdb2252126abeaa56ULL}, /* native_per_vacancy_available_funds */
 {0x082b6800,1248,0x28072bcd78729670ULL}, /* entrant_candidate_context */
 {0x082ba4f0,1536,0xebae01c4d50ca485ULL}, /* native_auto_progress */
 {0x082de9f0,480,0x6eb65215790e4c2bULL}, /* native_talent_predicate */
 {0x0817a9b0,704,0x8c000a4701df634eULL}, /* generation_batch_context */
 {0x08606e80,851,0x5b65438552eb08e1ULL}, /* cleanup_pool_native */
 {0x082d3970,720,0x7d353c8cc342ab8cULL}, /* native_talent_weights */
 {0x08342920,1289,0xb329c2732f435df8ULL}, /* prestige_fit_and_curve_observation */
 {0x08330e30,724,0xdb82bde6f7bec25bULL}, /* broad_eligibility */
 {0x0834eea0,515,0xf32d35870dd3b631ULL}, /* status_gate */
 {0x08345870,778,0x20c6f9a0e6316105ULL}, /* candidate_builder_context */
 {0x0832cce0,260,0x46ea629c910b05fbULL}, /* capacity */
 {0x08327050,1619,0x58635fd4d497c5dfULL}, /* create_opening */
 {0x08356850,937,0xc87c1494175b632fULL}, /* remove_renewal_opening */
 {0x083366a0,1674,0x96f457f2bc605014ULL}, /* select_outside_coach */
 {0x08606aa0,84,0xfbea5e71e0831253ULL}, /* coach_record_wrapper */
 {0x0832c610,1096,0x7a398e57ba0c8fbbULL}, /* school_interest */
 {0x08345870,2711,0x121f62d421529ef7ULL}, /* candidate_builder_full */
 {0x08359470,412,0xa8a5869a64b7333cULL}, /* performance_rank_writer */
 {0x08344b50,1616,0x24fe95283bd88565ULL}, /* contract_length_helper */
 {0x08353fe0,71,0xec9bfc4321d80106ULL}, /* candidate_comparator */
 {0x085aeda0,576,0x516d429dc6971b35ULL}, /* signing_dispatch */
 {0x08330f0c,5,0xd5d922f2c9f1c874ULL}, /* call_0x832d1cc */
 {0x08345c8d,5,0x3ea7623c53986635ULL}, /* call_0x8341e8d */
 {0x083462e4,5,0x174747d265d2c86ULL}, /* call_0x83424e4 */
 {0x08329414,5,0x7b167ee9d6a88315ULL}, /* call_0x83256d4 */
 {0x083362d3,5,0x3f8cd0a18c72ba18ULL}, /* call_0x8332593 */
 {0x08352cd8,5,0x40ab1d489bf63383ULL}, /* call_0x834eed8 */
 {0x0892df49,5,0x20773cb502be0df2ULL}, /* call_0x892a0e9 */
 {0x083364b4,5,0x48d8f528b1c389e2ULL}, /* call_0x8332774 */
 {0x0833a734,5,0xbb13baf9df1643ccULL}, /* call_0x83369f4 */
 {0x08344b35,5,0x77f69b03f8ab85d1ULL}, /* call_0x8340d35 */
 {0x08362a7d,5,0x637736e10047a696ULL}, /* call_0x835ec7d */
 {0x083631c4,5,0x98e9b3abfed23a70ULL}, /* call_0x835f3c4 */
 {0x087dd885,5,0xae20849d558d17b0ULL}, /* call_0x87d9a25 */
 {0x0834ae7a,5,0x956a6dbef2d58929ULL}, /* call_0x834707a */
 {0x08361e0f,5,0xa41a6f0bc2750b7eULL}, /* call_0x835e00f */
 {0x08361f02,5,0xe089e19f05244268ULL}, /* call_0x835e102 */
 {0x08361fe8,5,0x2022f353e152414fULL}, /* call_0x835e1e8 */
 {0x08830cce,5,0xd5f64ce819da3f68ULL}, /* call_0x882ce6e */
 {0x08356c3a,5,0x9eb35f9fc4b08cb9ULL}, /* call_0x8352e3a */
 {0x0832e108,5,0x848cbd8a159a4f77ULL}, /* call_0x832a3c8 */
 {0x08327eb7,5,0x31457832fb298f42ULL}, /* call_0x8324177 */
 {0x0834aae4,5,0x2ce54032d8c01b6dULL}, /* call_0x8346ce4 */
 {0x08364730,5,0x482ad4fb26c5bb86ULL}, /* call_0x8360930 */
 {0x08356c00,336,0x2cfeaf56e59485f6ULL}, /* contract_native_08352e00 */
 {0x08327df0,573,0xbd89071c32057727ULL}, /* contract_native_083240b0 */
 {0x0834a8e0,2021,0x95e3c10840a62488ULL}, /* contract_native_08346ae0 */
 {0x08364390,997,0x55310b7988981cdaULL}, /* contract_native_08360590 */
 {0x0832de60,942,0xde5fb9143b0da8aaULL}, /* contract_native_0832a120 */
 {0x0843e7b0,538,0xf78727eddf7434b7ULL}, /* contract_native_0843a9b0 */
 {0x08052780,596,0xf56255e48c955e5aULL}, /* contract_native_0804ea10 */
 {0x08047250,138,0xa7b59cd3e0978a80ULL}, /* contract_native_080434e0 */
 {0x0833f0a0,846,0xe77dd4b8cf236d58ULL}, /* contract_native_0833b2a0 */
 {0x08345870,2711,0x121f62d421529ef7ULL}, /* candidate_full */
 {0x0834a8e0,2021,0x95e3c10840a62488ULL}, /* reconstruct_full */
 {0x08830ae0,584,0x636bfa8ba1f4fcbbULL}, /* user_assign */
 {0x0817a9b0,73,0xe931696505e84e12ULL}, /* generation_entry */
 {0x086071e0,561,0x161b6d6bad3c3019ULL}, /* generation_cleanup */
 {0x0883b93d,44,0x7eea79380b63df1fULL}, /* user_predicate_context */
 {0x0883b75e,73,0x76ca36a6e7808fefULL}, /* user_prevalidation_context */
 {0x088057f0,51,0xa1bffb3e28677ca3ULL}, /* user_character_type */
};
/* R10_MEMBER_GUARDS_BEGIN
   Normalize descriptor name pointers to RVAs so ASLR does not change their
   fingerprints. Scalar indices and string locations must still match. */
static const Car35Guard car35_member_guards[]={
 {0x0bd5d450,528,0xca4a24469ea67681ULL}, /* Current evaluator route: FranchiseEval */
 {0x0bd61260,1072,0xcc4553969f6fc52ULL}, /* Current evaluator route: PlayerGoalsEval */
 {0x0bd36400,1872,0x6e35837f6bbd7372ULL}, /* Current evaluator route: StoryArcUtil */
 {0x0bd29f80,112,0xf766071a6edbea96ULL}, /* RecruitManager.Dynasty and FirstRecruitingStageDetails */
 {0x0bd2a380,112,0xd6e72eaa61fce9ebULL}, /* RecruitingEval.Franchise */
 {0x0bdf7610,304,0x1e57f12c3f4c4cfaULL}, /* RecruitTarget member order */
 {0x0bd77c30,80,0x237614b89001d301ULL}, /* LeavingPlayer member order */
 {0x0bd74330,1168,0xe5b06d7ead7b1e70ULL}, /* PlayersLeavingEval member order */
 {0x0bdc9540,4544,0xc1952a8b4c0eebd0ULL}, /* Player member order */
 {0x0bd57500,1744,0xa584b1792eed6a55ULL}, /* StaffHiringEval member order */
 {0x0bdc5040,320,0xa763f7a9253aa070ULL}, /* Coach member order */
 {0x0bdd17f0,6688,0xd3c9e27f12c2ebf8ULL}, /* Team member order */
 {0x0bde1730,224,0xf49e9ff33f66c84cULL}, /* League member order */
 {0x0bde4000,672,0xc53b0bde7e2602a0ULL}, /* Conference member order */
 {0x0bded2b0,752,0x59bd0b0d3d0ab633ULL}, /* Franchise member order */
};
static int car35_verify_members(void) {
 for(unsigned i=0;i<sizeof(car35_member_guards)/sizeof(car35_member_guards[0]);i++) {
  const Car35Guard *g=&car35_member_guards[i];uint64_t h=14695981039346656037ULL;
  const uint64_t *words=(const uint64_t*)(game_base+g->rva);
  for(unsigned k=0;k<g->size/8;k++) {
   uint64_t value=words[k];
   if(k&1) {
    if(value<game_base || value-game_base>=302002176u)return 0;
    value-=game_base;
   }
   for(unsigned j=0;j<8;j++){h=(h^(unsigned char)(value&255))*1099511628211ULL;value>>=8;}
  }
  if(h!=g->fnv)return 0;
 }
 return 1;
}
/* R10_MEMBER_GUARDS_END */
static int car35_verify(void) {
 if(!car35_verify_members())return 0;
 for(unsigned i=0;i<sizeof(car35_guards)/sizeof(car35_guards[0]);i++) {
  uint64_t h=14695981039346656037ULL;const unsigned char *p=(const unsigned char*)(game_base+car35_guards[i].rva);
  for(unsigned k=0;k<car35_guards[i].size;k++)h=(h^p[k])*1099511628211ULL;
  if(h!=car35_guards[i].fnv)return 0;
 }
 for(int i=0;i<CAR35_CALL_COUNT;i++)if(memcmp((void*)(game_base+car35_calls[i].rva),car35_calls[i].expected,car35_calls[i].size))return 0;
 return 1;
}
