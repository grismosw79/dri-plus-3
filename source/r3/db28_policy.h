#ifndef DRI_DB28_POLICY_H
#define DRI_DB28_POLICY_H

/* Native RecruitingMotivationType values, independently checked against the
   captured DealbreakerInfo schema. PlayingStyle and CoachPrestige retain their
   established requirement policies. Proximity's three FTC curves all use B. */
static inline int db28_shared_college_motivation(int motivation) {
    return motivation==2 || motivation==4 || motivation==7 ||
           motivation==9 || motivation==10;
}

/* Exact predicate in native RecruitRecord::IsTransfer (RVA 08629dd0).
   0 = HS; 1..3 = JUCO; 4..7 = transfer classes. */
static inline int db28_transfer_class(int recruiting_class) {
    return (unsigned)recruiting_class-4u <= 3u;
}

/* Ordinals A+..F. The four private rows use -100 for disabled anchors;
   Playing Time retains the original roster ladder, including F=40. */
static inline const int *db28_expected_college_curve(int motivation) {
    static const int brand_champ[13]={-100,-100,90,-100,85,80,-100,75,-100,70,0,-100,-100};
    static const int conference[13]={-100,-100,85,-100,80,-100,-100,-100,0,-100,-100,-100,-100};
    static const int pro[13]={-100,-100,90,-100,-100,82,-100,75,-100,70,0,-100,-100};
    static const int playing_time[13]={-1,95,90,85,80,75,70,65,60,55,50,45,40};
    switch(motivation){case 2:case 4:return brand_champ;case 7:return conference;
        case 9:return playing_time;case 10:return pro;default:return 0;}
}

/* Explicit lower-bound bands, never the native nearest-anchor interpolation.
   Reject a stale/foreign curve rather than silently running mismatched tuning. */
static inline int db28_floor_grade(int motivation,const int values[13],int ovr) {
    const int *expected=db28_expected_college_curve(motivation);
    if(!expected || !values || ovr<0 || ovr>99)return -1;
    int best=-1,grade=12;
    for(int i=0;i<13;i++) {
        if(values[i]!=expected[i])return -1;
        if(values[i]>=0 && values[i]<=ovr && values[i]>best) {
            best=values[i];grade=i;
        }
    }
    return grade;
}

/* Grade ordinals ascend as requirements weaken. Native adds coach allowances;
   preserve that addition rather than clamping the resulting requirement. */
static inline int db28_replace_base(int native_requirement,int original_base,
                                   int college_base,int *answer) {
    if(!answer || original_base<0 || original_base>12 ||
       college_base<0 || college_base>12)return 0;
    *answer=native_requirement-original_base+college_base;
    return 1;
}
#endif
