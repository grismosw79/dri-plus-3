/* Test35R5: narrow validity filter at native AppendPurchasableTalents.
   Costs, unlocks, weights, random order, purchase limits and point awards stay
   native. Existing talents are never removed or rebuilt. This scope follows
   a historyless NPC through free agency and its first coordinator appointment;
   it ends when YearsCoaching or actual saved results record experience.
   A CareerStats reference alone can be prestige bookkeeping at first hire. */
static int car35_entrant_empty_results(void *coach,unsigned member,int career) {
    int present=-1,value=-1;
    if(!car35_record_reference_present(coach,member,&present))return 0;
    if(!present)return 1;
    void *stats=car35_retain_reference(coach,member);
    int empty=stats!=NULL;
    /* Current-title CareerCoachStats: 26 members; SeasonCoachStats: 2.
       DraftPicks (5), FirstRoundDraftPicks (6) and NumPrestigeIncreases (11)
       can be credited before the first game. RecentYearNCWon (16) must
       remain its native no-title default -2. Actual result counters are zero. */
    for(unsigned i=0;empty && i<(career?26u:2u);i++) {
        if(career && (i==5 || i==6 || i==11))continue;
        if(!car35_record_field(stats,i,2,&value) ||
           value!=((career && i==16)?-2:0))empty=0;
    }
    car35_release_reference(stats);return empty;
}
static int car35_entrant_role(void *coach) {
    int role=-1,user=-1,years=-1;
    if(!coach || !car35_record_field(coach,0,1,&user) || user ||
       !car35_record_field(coach,135,2,&years) || years!=0 ||
       !car35_record_field(coach,16,0,&role) || (role!=1 && role!=2) ||
       !car35_entrant_empty_results(coach,29,1) ||
       !car35_entrant_empty_results(coach,61,0))return -1;
    return role;
}
/* A position specialty is a trait, not fabricated employment history.
   Scope to native newborns or a first-appointment opposite-side default.
   Prefer the strongest existing own-side position rating. Native newborn
   ratings can all be zero: break that tie from existing birth data without
   consuming the game's RNG or changing ratings, XP, schemes or archetypes. */
static void car35_entrant_profile(void *coach,int newborn) {
    int role=car35_entrant_role(coach);
    if(role<0)return;
    int before_side=car35_get(coach,134,0),before=car35_get(coach,106,0);
    int first=role==1?0:4,count=role==1?4:3;
    int valid=before>=first && before<first+count;
    int desired=before;
    if(newborn || !valid) {
        static const unsigned fields[7]={99,101,109,96,85,91,79};
        int ratings[7]={0},best=-1,readable=1;
        for(int i=first;i<first+count;i++) {
            if(!car35_record_field(coach,fields[i],2,&ratings[i]) || ratings[i]<0)readable=0;
            if(ratings[i]>best)best=ratings[i];
        }
        if(readable) {
            uint32_t seed=car35_row(coach)*2654435761u;
            seed^=(uint32_t)car35_get(coach,41,2)*2246822519u;
            seed^=(uint32_t)car35_get(coach,1,2)*3266489917u;
            seed^=seed>>16;
            int ties=0;
            for(int i=first;i<first+count;i++)if(ratings[i]==best)ties++;
            unsigned choice=seed%(unsigned)ties;
            for(int i=first;i<first+count;i++)if(ratings[i]==best && choice--==0) {desired=i;break;}
        }
    }
    int side=car35_identity(coach,NULL,"entrant_profile");
    int written=0;
    if(mode==1 && desired>=first && desired<first+count && desired!=before)
        written=car35_set_coach_enum(coach,106,desired);
    log_line("CAR35_ENTRANT_PROFILE coach_row=%lu newborn=%d role=%d old_side=%d side=%d old_specialty=%d proposed_specialty=%d specialty=%d written=%d rule=existing_position_strength_birth_tiebreak history_added=0 quality_changed=0 mode=%ld",
        (unsigned long)car35_row(coach),newborn,role,before_side,side,before,desired,car35_get(coach,106,0),written,mode);
}
/* Native R13 owns coach_ref at 082B6AA6; RCX/RDX/R8 are the three original
   predicate arguments. The enclosing native window is version-guarded. */
__attribute__((naked,used)) static void car35_talent_gate_adapter(void) {
    __asm__ volatile("mov %r13, %r9\n\tjmp car35_talent_gate_hook\n\t");
}
__declspec(dllexport) unsigned char car35_talent_gate_hook(void *module,int status,void **node_ref,void **coach_ref) {
    unsigned char original=((unsigned char(*)(void*,int,void**))(game_base+0x082de9f0))(module,status,node_ref);
    void *coach=coach_ref?*coach_ref:NULL,*node=node_ref?*node_ref:NULL;
    int role=car35_entrant_role(coach);
    if(!original || role<0)return original;
    int root=-1,group=-1,effect=-1,amount=-1,result=original;
    void *talent=NULL,*data=NULL;const char *reason="native_valid";
    if(!car35_record_field(node,1,1,&root))reason="unreadable_node_native_retained";
    else if(root)reason="archetype_unlock_retained";
    else {
        talent=car35_retain_reference(node,5);
        if(!talent || !car35_record_field(talent,9,0,&group) || group<0 || group>8 ||
           !car35_record_field(talent,5,0,&effect))reason="unreadable_talent_native_retained";
        else {
            int opposite=(role==1 && (group==4 || group==5 || group==7)) ||
                         (role==2 && group>=0 && group<=3);
            if(opposite) {result=0;reason="opposite_unit_position";}
            /* These three effects are numeric TalentDataInt amounts, verified
               in all five FTC routes. Do not treat every zero as a no-op:
               other talents use ownership as an enable flag. */
            else if(effect==68 || effect==148 || effect==147) {
                data=car35_retain_reference(talent,role==1?8:2);
                if(data && car35_record_field(data,0,2,&amount) && amount==0) {
                    result=0;reason="zero_role_effect";
                }
            }
        }
    }
    log_line("CAR35_ENTRANT_CANDIDATE coach_row=%lu role=%d status=%d node_row=%lu talent_row=%lu group=%d effect=%d role_amount=%d archetype_unlock=%d native=%d proposed=%d returned=%d reason=%s mode=%d",
      (unsigned long)car35_row(coach),role,car35_get(coach,12,0),(unsigned long)car35_row(node),(unsigned long)car35_row(talent),group,effect,amount,root,original,result,mode==1?result:original,reason,mode);
    car35_release_reference(data);car35_release_reference(talent);
    return mode==1?(unsigned char)result:original;
}
/* Observation surrounds each of the five native auto-progress callers. No
   extra passes or purchases: the native free-agent initializer already loops. */
static void car35_auto_progress(void *module,void **coach_ref,void **team_ref,int limit) {
    void *coach=coach_ref?*coach_ref:NULL;int role=car35_entrant_role(coach);
    if(role>=0)car35_entrant_profile(coach,0);
    LONG event=InterlockedIncrement(&car35_event_count);
    if(role>=0)log_line("CAR35_ENTRANT_PROGRESS event=%ld boundary=begin coach_row=%lu role=%d level=%d xp=%d points=%d native_limit=%d caller_rva=0x%llx",
      event,(unsigned long)car35_row(coach),role,car35_get(coach,14,2),car35_get(coach,41,2),car35_get(coach,111,2),limit,(unsigned long long)((U)__builtin_return_address(0)-game_base));
    ((void(*)(void*,void**,void**,int))(game_base+0x082ba4f0))(module,coach_ref,team_ref,limit);
    if(role>=0)log_line("CAR35_ENTRANT_PROGRESS event=%ld boundary=end coach_row=%lu role=%d level=%d xp=%d points=%d native_limit=%d",
      event,(unsigned long)car35_row(coach),role,car35_get(coach,14,2),car35_get(coach,41,2),car35_get(coach,111,2),limit);
}
static unsigned char car35_talent_purchase(void *module,void **tree_ref,void **status_ref,int index,void **team_ref,void **coach_ref,unsigned char flag) {
    void *coach=coach_ref?*coach_ref:NULL;int role=car35_entrant_role(coach),before=car35_get(coach,111,2);
    unsigned char result=((unsigned char(*)(void*,void**,void**,int,void**,void**,unsigned char))(game_base+0x082e0ab0))(module,tree_ref,status_ref,index,team_ref,coach_ref,flag);
    if(role>=0)log_line("CAR35_ENTRANT_PURCHASE coach_row=%lu role=%d subtree_row=%lu node_index=%d native_success=%d points_before=%d points_after=%d",
      (unsigned long)car35_row(coach),role,(unsigned long)car35_row(tree_ref?*tree_ref:NULL),index,result,before,car35_get(coach,111,2));
    return result;
}
