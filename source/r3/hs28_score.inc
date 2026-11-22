/* Test31: retained native board scoring at081733f5 supplies the school cache.
   A null cache instead reads aggregate team need and can disagree in sign
   with the real candidate/retained-board scorer. Preserve the actual native
   selection0/HS0 ABI and use the same inline per-position school context. */
static int market29_portal_school_cache(Rec team,U eval,U cache[67]);
typedef struct {
 U eval,scorer,franchise_table,franchise_handle,team_table;
 uint32_t franchise_row,team_row,year,week,type;
} Hs31ScoreScope;
typedef struct {Hs31ScoreScope scope;U generation;int valid;U values[67];} Hs31SchoolScore;
static Hs31SchoolScore hs31_school_scores[138];
static SRWLOCK hs31_score_lock=SRWLOCK_INIT;
static void hs31_native_score_invalidate(Rec team) {
 unsigned tid=team?scalar(team,383,0x100):138;
 AcquireSRWLockExclusive(&hs31_score_lock);
 if(team){if(tid<138){hs31_school_scores[tid].valid=0;hs31_school_scores[tid].generation++;}}
 else for(int i=0;i<138;i++){hs31_school_scores[i].valid=0;hs31_school_scores[i].generation++;}
 ReleaseSRWLockExclusive(&hs31_score_lock);
}
/* The captured context contains three inline integer vectors. Rebase only
   their nine verified header pointers when copying it; retain no game object
   or stack address in the persistent numeric cache. Bounds failures reject
   the forecast rather than falling back to the mismatched aggregate scorer. */
static int hs31_school_copy(U to[67],const U from[67]) {
 static const unsigned at[3]={0,0xa8,0x150},inline_at[3]={0x18,0xc0,0x168},end[3]={0xa8,0x150,0x218};
 U base=(U)from;
 for(int v=0;v<3;v++)for(int k=0;k<3;k++) {
  U p=from[at[v]/8+k];
  if(p<base+inline_at[v] || p>base+end[v] || ((p-base)&3) || (!k && p!=base+inline_at[v]))return 0;
 }
 memcpy(to,from,67*sizeof(U));
 for(int v=0;v<3;v++)for(int k=0;k<3;k++)to[at[v]/8+k]=(U)to+(from[at[v]/8+k]-base);
 return 1;
}
static int hs31_school_context(Rec team,U eval,const Hs31ScoreScope *scope,U cache[67]) {
 unsigned tid=scalar(team,383,0x100);if(tid>=138)return 0;
 U generation;int found=0,ok=1;
 AcquireSRWLockExclusive(&hs31_score_lock);
 Hs31SchoolScore *s=&hs31_school_scores[tid];generation=s->generation;
 if(s->valid && !memcmp(&s->scope,scope,sizeof(*scope))){found=1;ok=hs31_school_copy(cache,s->values);}
 ReleaseSRWLockExclusive(&hs31_score_lock);
 if(found)return ok;
 if(!market29_portal_school_cache(team,eval,cache))return 0;
 AcquireSRWLockExclusive(&hs31_score_lock);
 if(s->generation==generation) {
  ok=hs31_school_copy(s->values,cache);s->scope=*scope;s->valid=ok;
 }
 ReleaseSRWLockExclusive(&hs31_score_lock);return ok;
}
static int hs28_native_score(Rec recruit,Rec team,U eval,int *out) {
 if(!recruit || !team || !eval || !out)return 0;
 U scorer=q(eval+0xf8);if(!scorer)return 0;
 Rec f=context_ref(eval,6),season=0;uint32_t year=0,week=0,type=0;
 if(!cache_season_get(eval,f,&year,&week,&type)) { /* cache stack: the season memo of the window */
  season=f?reference(f,44):0;
  if(!season){release(f);return 0;}
  year=scalar(season,2,0x100);week=scalar(season,4,0x100);type=scalar(season,5,0x108);
  cache_season_check(eval,f,year,week,type,"HS_NATIVE_SCORE");
 }
 Hs31ScoreScope scope;memset(&scope,0,sizeof(scope));scope.eval=eval;scope.scorer=scorer;
 scope.franchise_table=q((U)f+0x10);scope.franchise_handle=(U)f;scope.franchise_row=row(f);
 scope.team_table=q((U)team+0x10);scope.team_row=row(team);
 scope.year=year;scope.week=week;scope.type=type;
 release(season);release(f);if(scope.week>63)return 0;
 U cache[67];if(!hs31_school_context(team,eval,&scope,cache))return 0;
 struct {U rec;int32_t values[10];} candidate={(U)recruit,{0}};
 _Static_assert(sizeof(candidate)==48,"HS native score candidate ABI");
 ((void(*)(U,unsigned char,unsigned char,Rec*,U*,void*,int))
  (game_base+0x081858a0))(scorer,0,0,&team,cache,&candidate,(int)scope.week);
 if(candidate.rec!=(U)recruit)return 0;
 *out=candidate.values[8];return 1;
}
#ifdef DRI_EMULATION_TEST
__declspec(dllexport) int DriTestHsNativeScore(Rec r,Rec t,U e,int *out){return hs28_native_score(r,t,e,out);}
__declspec(dllexport) void DriTestHs31ScoreInvalidate(Rec t){hs31_native_score_invalidate(t);}
#endif
