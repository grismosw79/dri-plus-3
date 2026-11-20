#include "pt28_core.h"
#include <string.h>
#include <math.h>
static int pt28_project(int ovr,int step){return ovr+step>99?99:ovr+step;}
double pt28_annual(int ovr,const int *peers,int count,int starters,int useful,int rotation){
 if(starters<=0)return 0.;
 int higher=0,equal=0,last_starter=-1,sorted[PT28_MAX_PLAYERS];
 if(count<0||count>PT28_MAX_PLAYERS||(!peers&&count))return -1.;
 for(int i=0;i<count;i++){
  if(peers[i]<0||peers[i]>99)return -1.;
  higher+=peers[i]>ovr;equal+=peers[i]==ovr;
  int j=i;while(j>0&&sorted[j-1]<peers[i]){sorted[j]=sorted[j-1];j--;}sorted[j]=peers[i];
 }
 if(count>=starters)last_starter=sorted[starters-1];
 double total=0.;
 for(int rank=higher+1;rank<=higher+equal+1;rank++){
  double score;
  if(rank<=starters)score=12.;
  else if(rank==starters+1 && last_starter-ovr<=4)score=9.;
  else if(rank<=useful){
   int width=useful-starters-1;if(width<1)width=1;
   double fraction=(double)(rank-starters-1)/width;
   if(fraction<0.)fraction=0.;
   if(fraction>1.)fraction=1.;
   score=(rotation?7.:4.)-2.*fraction;
  }else score=1.;
  total+=score;
 }
 return total/(equal+1);
}
double pt28_combine(double immediate,double following,int following_available){
 double bonus=following_available?.2*(following-immediate):0.;
 if(bonus<0.)bonus=0.;
 if(bonus>1.)bonus=1.;
 double score=immediate+bonus;return score>12.?12.:score;
}
int pt28_letter(double score){
 if(!isfinite(score)||score<0.||score>12.)return -1;
 int point=(int)floor(score+.5);if(point<0)point=0;if(point>12)point=12;return 12-point;
}
int pt28_compute(const Pt28Room *room,const Pt28Player *candidate,int step,int advance,int development,Pt28Result *out){
 if(!room||!candidate||!out||step<0||step>20||advance<0||advance>1||development<0||development>1||room->count<0||room->count>PT28_MAX_PLAYERS||
    room->starters<0||room->starters>22||room->useful<0||room->useful>44||candidate->ovr<0||candidate->ovr>99||candidate->year<0||candidate->year>3)return 0;
 memset(out,0,sizeof(*out));
 int peers[PT28_MAX_PLAYERS],future[PT28_MAX_PLAYERS];
 int year=candidate->year+((candidate->hs||(candidate->redshirt&&candidate->year<3))?0:advance);
 /* A last-year player still gets an opportunity grade for his requested job.
    Native PLV eligibility, not this grade function, decides graduation. */
 if(year>3)year=3;
 out->following_available=year<3;out->starters=room->starters;out->useful=room->useful;out->rotation=room->rotation;
 out->projected_ovr=pt28_project(candidate->ovr,candidate->develop?step*development:0);
 out->following_ovr=pt28_project(out->projected_ovr,step);
 for(int i=0;i<room->count;i++){
  const Pt28Player *p=&room->players[i];
  if(p->ovr<0||p->ovr>99||p->year<0||p->year>3||p->arrival_delay<0||p->arrival_delay>1)return 0;
  if(p->table==candidate->table&&p->id==candidate->id)continue;
  if(p->arrival_delay){
   /* A secured HS/JUCO player arriving after this season is absent now and
      enters next year's room at his supplied rating and entering class. */
   future[out->future_peer_count++]=p->ovr;continue;
  }
  int py=p->year+((p->hs||(p->redshirt&&p->year<3))?0:advance);
  if(py>3)continue;
  int rating=pt28_project(p->ovr,p->develop?step*development:0);
  peers[out->peer_count++]=rating;
  out->rank+=rating>out->projected_ovr;out->equals+=rating==out->projected_ovr;
  if(py<3)future[out->future_peer_count++]=pt28_project(rating,step);
 }
 out->rank++;
 out->immediate=pt28_annual(out->projected_ovr,peers,out->peer_count,room->starters,room->useful,room->rotation);
 out->following=out->following_available?pt28_annual(out->following_ovr,future,out->future_peer_count,room->starters,room->useful,room->rotation):out->immediate;
 if(out->immediate<0.||out->following<0.)return 0;
 out->score=pt28_combine(out->immediate,out->following,out->following_available);out->grade=pt28_letter(out->score);return out->grade>=0;
}
