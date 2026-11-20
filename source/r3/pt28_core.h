#ifndef DRI_PT28_CORE_H
#define DRI_PT28_CORE_H
#include <stdint.h>
#define PT28_MAX_PLAYERS 384
typedef struct {
 uintptr_t table;
 uint32_t id;
 int pos,ovr,year,hs,develop,redshirt,arrival_delay,rostered;
} Pt28Player;
typedef struct {
 int starters,useful,rotation,count;
 Pt28Player players[PT28_MAX_PLAYERS];
} Pt28Room;
typedef struct {
 int grade,rank,equals,starters,useful,rotation,peer_count,future_peer_count;
 int projected_ovr,following_ovr,following_available;
 double immediate,following,score;
} Pt28Result;
/* A single scale for all classes/paths. Inputs describe legitimate jobs and
   the existing DLL's useful-depth horizon, never recruiting-admission caps. */
double pt28_annual(int ovr,const int *peers,int count,int starters,int useful,int rotation);
double pt28_combine(double immediate,double following,int following_available);
int pt28_letter(double score);
/* Class advancement is a caller-supplied lifecycle fact, not inferred from
   whether this record is wrapped as a Player or Recruit. 0=current season,
   1=next season. Development is separate because training and class rollover
   need not occur together. HS/JUCO preserves its supplied entering class. */
int pt28_compute(const Pt28Room *room,const Pt28Player *candidate,int step,int advance,int development,Pt28Result *out);
#endif
