#ifndef DRI_STAFF28_CORE_H
#define DRI_STAFF28_CORE_H
#include <stdint.h>
#define STAFF28_TEAMS 138
typedef struct {
    int32_t score[3];             /* HC, OC, DC. Raw native prestige score. */
    unsigned char present[3];    /* An absent coach is distinct from score 0. */
} Staff28Scores;
int staff28_side(int position);  /* 0 offense, 1 defense, -1 native specialist */
int64_t staff28_score(const Staff28Scores *scores,int side);
int staff28_rank_grade(unsigned better);
int staff28_compute(const Staff28Scores *scores,unsigned count,int grades[][2]);
float staff28_distance(float home_lat,float home_lon,float team_lat,float team_lon,
                       float (*cosine)(float));
#endif
