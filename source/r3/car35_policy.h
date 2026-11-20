#ifndef CAR35_POLICY_H
#define CAR35_POLICY_H
#include <stdint.h>
/* T1 uses fixed role terms so creation and reconstruction are independent of
   status, previous employer, current performance, and process lifetime. */
static int car35_active_phase(int stage,int week,int active) {
    return stage==1 && week>=2 && week<=5 && active==1;
}
static int car35_contract_years(int role) { return role==0?5:(role==1||role==2)?2:0; }
/* Test35R1: Approved coordinator-to-HC floor. LetterGrade is ordered
   Aplus=0 through Cplus=6, C=7, ... F=12; Incomplete=13 is not a grade pass.
   Current HC status is authoritative: an HC's previous coordinator role does
   not make an HC-to-HC move a new promotion. Unassigned/unknown current roles
   may use the last recorded OC/DC role; no career history is fabricated. */
static int car35_promotion_source_role(int current,int previous) {
    return current>=0 && current<=2?current:previous;
}
static int car35_promotion_grade_at_floor(int destination,int current,int previous,int grade,int floor) {
    int source=car35_promotion_source_role(current,previous);
    if(floor<6 || floor>8)floor=6;
    return destination!=0 || (source!=1 && source!=2) || (grade>=0 && grade<=floor);
}
static int car35_promotion_grade_allowed(int destination,int current,int previous,int grade) {
    return car35_promotion_grade_at_floor(destination,current,previous,grade,6);
}
static int car35_merit_numerator(int native_score,int rank) {
    int q=13700+137*native_score;
    if(rank>=0 && rank<=137) q+=80*(137-2*rank);
    return q;
}
static int car35_merit_round(int q) { return (q+137)/274; }
/* No-history free agents stay in a separate 1..10 fallback tier. Their score
   represents level relative to that build's positive, non-user fallback pool,
   not absolute school fit. An equal-level pool receives the neutral score 5.
   Native fit remains a comparator tie-breaker; a zero native score is never
   passed here. Wider arithmetic also supports the entire signed level range. */
static int car35_fallback_numerator(int level,int minimum,int maximum) {
    if(maximum<=minimum)return 5*274;
    int64_t distance=(int64_t)level-minimum,range=(int64_t)maximum-minimum;
    if(distance<=0)return 274;
    if(distance>=range)return 10*274;
    return 274+(int)(2466*distance/range);
}
#endif
