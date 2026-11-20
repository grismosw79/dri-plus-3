#include "staff28_core.h"
#include <math.h>
#include <stddef.h>
int staff28_side(int position){
    if(position>=0&&position<=9)return 0;
    if(position>=10&&position<=18)return 1;
    return -1;
}
int64_t staff28_score(const Staff28Scores *scores,int side){
    if(!scores||side<0||side>1)return 0;
    int coordinator=side+1;
    return (scores->present[0]?(int64_t)scores->score[0]*75:0)+
           (scores->present[coordinator]?(int64_t)scores->score[coordinator]*25:0);
}
int staff28_rank_grade(unsigned better){
    static const unsigned cut[13]={5,10,15,25,35,45,55,65,75,90,105,120,250};
    for(int grade=0;grade<13;grade++)if(better<=cut[grade])return grade;
    return 13;
}
int staff28_compute(const Staff28Scores *scores,unsigned count,int grades[][2]){
    if(!scores||!grades||!count||count>STAFF28_TEAMS)return 0;
    int64_t values[STAFF28_TEAMS][2];
    for(unsigned i=0;i<count;i++){
        for(int k=0;k<3;k++)if(scores[i].present[k]>1||
           (scores[i].present[k]&&scores[i].score[k]<0))return 0;
        for(int side=0;side<2;side++)values[i][side]=staff28_score(&scores[i],side);
    }
    for(unsigned i=0;i<count;i++)for(int side=0;side<2;side++){
        unsigned better=0;
        for(unsigned j=0;j<count;j++)better+=values[j][side]>values[i][side];
        grades[i][side]=staff28_rank_grade(better);
    }
    return 1;
}
/* Native equirectangular approximation, with explicit float32 rounding at each
   operation. Runtime supplies the game's cosf; portable tests supply libc cosf. */
float staff28_distance(float home_lat,float home_lon,float team_lat,float team_lon,
                       float (*cosine)(float)){
    volatile float hrad=home_lat*0.01745329238474369f;
    volatile float trad=team_lat*0.01745329238474369f;
    volatile float sum=hrad+trad;
    volatile float mid=sum*0.5f;
    volatile float lon=home_lon-team_lon;
    volatile float lat=home_lat-team_lat;
    volatile float dx=cosine(mid)*lon;
    volatile float xx=dx*dx,yy=lat*lat;
    volatile float square=xx+yy;
    volatile float distance=sqrtf(square)*69.0f;
    return distance;
}
