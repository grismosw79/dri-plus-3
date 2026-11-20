/* Test28 college requirement alignment.
   Include after retention/portal record helpers; route from DriEpsRequired
   inside its existing SEH boundary. This module does not mutate tuning.

   Native 0842f5d0 uses field4/OVR for Player, field6/stars for transfer Recruit,
   field5/stars for HS/JUCO, then adds the position-specific coach allowance.
   Native curve lookup selects the nearest anchor, not a lower-bound band.
   Read and validate the Test28 field4 policy, then apply explicit floor bands.
 */
#include "db28_policy.h"

static volatile LONG db28_requirement_samples[16];

static int db28_curve_grade(Rec info,int field_id,int value) {
    Rec curve=reference(info,field_id);
    if(!curve){portal_fault("DB28_requirement_curve");return -1;}
    Wrap w={game_base+0x0bd2ee10,curve,schema(curve),0};
    int grade=((int(*)(Wrap*,int))(game_base+0x0860ddd0))(&w,value);
    release(curve);
    return grade;
}

static int db28_college_grade(Rec info,int motivation,int ovr) {
    Rec curve=reference(info,4);
    if(!curve){portal_fault("DB28_college_curve");return -1;}
    int values[13];
    for(int i=0;i<13;i++)values[i]=(int)scalar(curve,i,0x100);
    release(curve);
    int grade=db28_floor_grade(motivation,values,ovr);
    if(grade<0)portal_fault("DB28_college_curve_mismatch");
    return grade;
}

/* Returns 1 when a verified college requirement was handled, 0 for native
   passthrough or fault. Both roster Player and transfer Recruit use floor bands.
   Do not infer transfer status from team ownership, class year, or PrevTeam. */
static int db28_required(U eval,Rec info,Rec person,Rec team,
                        int native_requirement,int *answer) {
    if(mode!=1 || faulted || !eval || !info || !person || !team || !answer)return 0;
    int motivation=(int)scalar(info,3,0x108);
    if(!db28_shared_college_motivation(motivation))return 0;
    U person_schema=schema(person);
    int is_player=((unsigned char(*)(U,U))(game_base+0x08059c60))
                  (person_schema,q(eval+0x60));
    int recruit_class=-1;
    Rec player=person;
    if(!is_player) {
        int is_recruit=((unsigned char(*)(U,U))(game_base+0x08059c60))
                       (person_schema,q(eval+0x68));
        if(!is_recruit)return 0;
        recruit_class=(int)scalar(person,2,0x108);
        if(!db28_transfer_class(recruit_class))return 0;
        player=reference(person,5);
    }
    if(!player){portal_fault("DB28_requirement_player");return 0;}
    int ovr=(int)scalar(player,191,0x100);
    int stars=(int)scalar(player,231,0x108);
    uint32_t player_row=row(player);
    if(!is_player)release(player);
    if(ovr<0 || ovr>99 || (!is_player && (stars<0 || stars>4))){
        portal_fault("DB28_requirement_quality");return 0;
    }
    int original=db28_curve_grade(info,is_player?4:6,is_player?ovr:stars);
    int college=db28_college_grade(info,motivation,ovr);
    if(!db28_replace_base(native_requirement,original,college,answer)){
        portal_fault("DB28_requirement_value");return 0;
    }
    /* Bounded diagnostic samples; eligibility calls can number in millions.
       Sampling never changes policy or replenishes recruiting allowances. */
    LONG sample=InterlockedIncrement(&db28_requirement_samples[motivation]);
    if(sample<=8)log_line("DB28_REQUIREMENT_SAMPLE,%d,%ld,%u,%d,%d,%d,%d,%d,%d,%d",
        motivation,(long)sample,player_row,(int)scalar(team,383,0x100),
        recruit_class,ovr,stars,original,college,*answer);
    return !faulted && mode==1;
}
