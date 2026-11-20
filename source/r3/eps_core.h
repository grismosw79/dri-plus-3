#ifndef DRI_EPS_CORE_H
#define DRI_EPS_CORE_H
#include "eps_config.h"
typedef struct {
 int team,opponent,game;
 double pass_yards,pass_attempts,rush_yards,rush_attempts,sacks;
 double group[EPS_GROUPS][EPS_STATS];
} EpsRow;
typedef struct {
 double value,observed,numerator,denominator,prior_exposure,prior_value;
} EpsMetric;
typedef struct {double score,rank;int grade,status;} EpsGrade;
/* status 0 provisional, 1 supported, 2 prior final carried. */
typedef struct {
 int year,rows,teams;
 double selected_prior_games[4];
 double exposure[EPS_TEAMS][EPS_EXPOSURES];
 EpsMetric metric[EPS_TEAMS][EPS_METRICS];
 EpsGrade grade[EPS_TEAMS][EPS_PROFILES];
} EpsOutput;
int eps_compute(const EpsRow *rows,int count,int teams,int year,const EpsOutput *previous,EpsOutput *out);
int eps_profile(int archetype,int position);
int eps_grade_pass(const EpsGrade *g,int requirement,int allowance);
#endif
