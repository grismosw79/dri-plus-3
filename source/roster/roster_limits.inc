/* Native member numbers, in this build's order. The configured team limit is
   shared by Franchise, CutDayEval and TeamManager. Retain a record only for
   the duration of this check on the current dynasty's offseason thread. */
enum { RM_FRANCHISE_LIMITS=43, RM_RECRUIT_CUT_DAY=3, RM_CUT_DAY_LIMITS=24,
       RM_RECRUIT_TEAM_MANAGER=24, RM_TEAM_MANAGER_LIMITS=10,
       RM_SIZE_FIELDS=4, RM_SETTINGS_FIELDS=7, RM_DYNASTY_CEILING=90 };
typedef struct { int value[RM_SETTINGS_FIELDS]; } RMRosterLimits;
enum RMCapResult { RM_CAP_READY, RM_CAP_RAISED, RM_CAP_UNKNOWN, RM_CAP_LOW,
                   RM_CAP_REFUSED, RM_CAP_RESTORE_UNVERIFIED, RM_CAP_CACHE_UNKNOWN, RM_CAP_CACHE_REFUSED };
static const char *const rm_cap_names[]={"READY","RAISED","UNVERIFIED",
    "LIMIT_CHANGED","WRITE_REFUSED","RESTORE_UNVERIFIED","ENROLLMENT_CACHE_UNVERIFIED","ENROLLMENT_CACHE_REFUSED"};
enum RMCapPhase { RM_CAP_START, RM_CAP_ENROLL, RM_CAP_COMMIT };
static const char *const rm_cap_phases[]={"CYCLE_START","BEFORE_ENROLLMENT","BEFORE_FIRST_TRANSACTION"};

#include "../set_record_scalar.inc"
static int rm_write_one_limit(Rec record,unsigned member,int value){
 if(!record||member>=RM_SIZE_FIELDS||value<53||value>RM_DYNASTY_CEILING)return 0;
 __try { return set_scalar(record,member,2,value); }
 __except(EXCEPTION_EXECUTE_HANDLER) { return 0; }
}
static int rm_read_one_setting(Rec record,unsigned member,int *value){
 __try { return scalar(record,member,2,value); }
 __except(EXCEPTION_EXECUTE_HANDLER) { return 0; }
}
static int rm_read_roster_limits(Rec record,RMRosterLimits *limits){
 for(unsigned i=0;i<RM_SETTINGS_FIELDS;i++){
  if(!rm_read_one_setting(record,i,&limits->value[i]))return 0;
  if(i<RM_SIZE_FIELDS && (limits->value[i]<53 || limits->value[i]>116))return 0;
 }
 return 1;
}
static Rec rm_resolve_cap_record(Rec manager,Rec franchise){
 Rec primary=0,cut=0,team=0,cut_info=0,team_info=0;
 int ok=0,matches=0;
 __try {
  primary=reference(franchise,RM_FRANCHISE_LIMITS,&ok);if(!ok||!primary)__leave;
  cut=reference(manager,RM_RECRUIT_CUT_DAY,&ok);if(!ok||!cut)__leave;
  cut_info=reference(cut,RM_CUT_DAY_LIMITS,&ok);if(!ok||!cut_info||!pilot_same(primary,cut_info))__leave;
  team=reference(manager,RM_RECRUIT_TEAM_MANAGER,&ok);if(!ok||!team)__leave;
  team_info=reference(team,RM_TEAM_MANAGER_LIMITS,&ok);
  matches=ok&&team_info&&pilot_same(primary,team_info);
 } __except(EXCEPTION_EXECUTE_HANDLER) { matches=0; }
 release(team_info);release(team);release(cut_info);release(cut);
 if(matches)return primary;
 release(primary);return 0;
}
static enum RMCapResult rm_align_roster_limits(Rec record,int allow_write,RMRosterLimits *before,RMRosterLimits *after){
 if(!record||!rm_read_roster_limits(record,before))return RM_CAP_UNKNOWN;
 *after=*before;
 RMRosterLimits wanted=*before;
 unsigned pending=0,attempted=0;
 for(unsigned i=0;i<RM_SIZE_FIELDS;i++)if(before->value[i]<RM_DYNASTY_CEILING){
  wanted.value[i]=RM_DYNASTY_CEILING;pending|=1u<<i;
 }
 if(!pending)return RM_CAP_READY;
 if(!allow_write)return RM_CAP_LOW;
 for(unsigned i=0;i<RM_SIZE_FIELDS;i++)if(pending&(1u<<i)){
  attempted|=1u<<i;
  if(!rm_write_one_limit(record,i,wanted.value[i]))break;
 }
 /* Confirm all four limits together, including wider custom values. The
    three adjacent practice-squad fields must also keep their original values. */
 if(rm_read_roster_limits(record,after)&&!memcmp(after,&wanted,sizeof(wanted)))return RM_CAP_RAISED;
 for(unsigned i=0;i<RM_SIZE_FIELDS;i++)if(attempted&(1u<<i)){
  int actual;
  if(!rm_read_one_setting(record,i,&actual)||actual!=before->value[i])
   (void)rm_write_one_limit(record,i,before->value[i]);
 }
 return rm_read_roster_limits(record,after)&&!memcmp(before,after,sizeof(*after))?
        RM_CAP_REFUSED:RM_CAP_RESTORE_UNVERIFIED;
}
/* RecruitingManager copies RosterInfo.MaxRosterSize once at initialization.
   ENROLL_COMMITS uses that copy to determine how many returners to retire. */
typedef struct {
 unsigned char prefix[0xb98];
 volatile LONG64 enrollment_limit;
} RMEnrollmentManager;
static int rm_enrollment_target;
static unsigned rm_enrollment_checks,rm_enrollment_repairs;

static int rm_enrollment_layout_matches(void){
 static const unsigned char init[]={0x48,0x89,0x8e,0x98,0x0b,0x00,0x00};
 static const unsigned char space[]={0x48,0x8b,0x85,0x98,0x0b,0x00,0x00,0x48,0x2b,0xc2,0x48,0x3b,0xf0,0x76,0x18};
 unsigned char readback[sizeof(space)];
 return local_read(game_base+0x8194118,readback,sizeof(init))&&!memcmp(readback,init,sizeof(init))&&
        local_read(game_base+0x8167777,readback,sizeof(space))&&!memcmp(readback,space,sizeof(space));
}
static int rm_read_enrollment_limit(int *value){
 LONG64 current=0;
 if(!rm_manager||(rm_manager&7)||
    !local_read((U)&((RMEnrollmentManager*)rm_manager)->enrollment_limit,&current,sizeof(current))||
    current<53||current>116)return 0;
 *value=(int)current;return 1;
}
static int rm_sync_enrollment_limit(int target,int *actual){
 if(target<RM_DYNASTY_CEILING||target>116||!rm_read_enrollment_limit(actual))return 0;
 volatile LONG64 *slot=&((RMEnrollmentManager*)rm_manager)->enrollment_limit;
 for(int attempt=0;attempt<3;attempt++){
  if(*actual>=target)return 1;
  int previous=*actual;
  LONG64 observed;
  /* Change exactly the observed value. A concurrent wider ceiling wins. */
  __try {observed=InterlockedCompareExchange64(slot,target,previous);}
  __except(EXCEPTION_EXECUTE_HANDLER) {return 0;}
  if(rm_read_enrollment_limit(actual))continue;
  if(observed!=previous)return 0;
  /* A successful write lost its readback. Undo only that value before a
     clean handoff. A read failure alone cannot establish the final state. */
  __try {(void)InterlockedCompareExchange64(slot,previous,target);}
  __except(EXCEPTION_EXECUTE_HANDLER) {return -1;}
  if(!rm_read_enrollment_limit(actual))return -1;
  if(*actual>=target)return 1;
  return *actual==previous?0:-1;
 }
 return *actual>=target;
}
static int rm_restore_raised_limits(Rec record,const RMRosterLimits *before,RMRosterLimits *after){
 for(unsigned i=0;i<RM_SIZE_FIELDS;i++)if(before->value[i]<RM_DYNASTY_CEILING){
  int current;
  if(rm_read_one_setting(record,i,&current)&&current==RM_DYNASTY_CEILING)
   (void)rm_write_one_limit(record,i,before->value[i]);
 }
 return rm_read_roster_limits(record,after)&&!memcmp(before,after,sizeof(*before));
}
static int rm_require_roster_limits(enum RMCapPhase phase){
 RMRosterLimits before={{-1,-1,-1,-1,-1,-1,-1}},after=before;
 int cache_before=-1,cache_after=-1;
 if(phase==RM_CAP_START){
  rm_enrollment_target=0;rm_enrollment_checks=rm_enrollment_repairs=0;
 }
 Rec settings=rm_resolve_cap_record((Rec)rm_manager,rm_franchise);
 enum RMCapResult result=RM_CAP_UNKNOWN;
 if(settings){
  if(!rm_enrollment_layout_matches()||!rm_read_enrollment_limit(&cache_before)){
   (void)rm_read_roster_limits(settings,&before);after=before;result=RM_CAP_CACHE_UNKNOWN;
  }else{
   cache_after=cache_before;
   result=rm_align_roster_limits(settings,phase!=RM_CAP_COMMIT,&before,&after);
   if(result==RM_CAP_READY||result==RM_CAP_RAISED){
    int target=after.value[0];
    int synced=rm_sync_enrollment_limit(target,&cache_after);
    if(synced==1)rm_enrollment_target=target;
    else if(result==RM_CAP_RAISED&&!rm_restore_raised_limits(settings,&before,&after))
     result=RM_CAP_RESTORE_UNVERIFIED;
    else result=synced<0?RM_CAP_RESTORE_UNVERIFIED:RM_CAP_CACHE_REFUSED;
   }
  }
 }
 release(settings);
 log_line("RM18R22 ROSTER_LIMITS phase=%s result=%s before=%d,%d,%d,%d after=%d,%d,%d,%d active_before=%d active_after=%d",
    rm_cap_phases[phase],rm_cap_names[result],before.value[0],before.value[1],before.value[2],before.value[3],
    after.value[0],after.value[1],after.value[2],after.value[3],cache_before,cache_after);
 if(result==RM_CAP_READY||result==RM_CAP_RAISED)return 1;
 if(result==RM_CAP_RESTORE_UNVERIFIED){
  rm_abort("ROSTER_SETTINGS_RESTORE_UNVERIFIED; no custom roster transactions started");
 }else if(phase==RM_CAP_START){
  log_line("RM18R22 ROSTER_LIMITS_RETRY_BEFORE_ENROLLMENT");
 }else{
  rm_handoff_capacity_if_clean();
 }
 return 0;
}
static int rm_check_team_enrollment_limit(int team){
 int before=-1,after=-1;
 if(rm_enrollment_target>=RM_DYNASTY_CEILING&&rm_read_enrollment_limit(&before)){
  after=before;
  if(before>=rm_enrollment_target||rm_sync_enrollment_limit(rm_enrollment_target,&after)==1){
   if(before<rm_enrollment_target){
    rm_enrollment_repairs++;
    log_line("RM18R22 ROSTER_ENROLLMENT_CACHE team=%d target=%d active_before=%d active_after=%d",team,rm_enrollment_target,before,after);
   }
   rm_enrollment_checks++;
   return 1;
  }
 }
 log_line("RM18R22 ROSTER_ENROLLMENT_CACHE_UNAVAILABLE team=%d target=%d active_before=%d active_after=%d",team,rm_enrollment_target,before,after);
 rm_handoff_capacity_if_clean();
 if(!rm_fallback)rm_abort("ENROLLMENT_ROSTER_LIMIT_UNVERIFIED_AFTER_ROSTER_PROCESSING");
 return 0;
}
