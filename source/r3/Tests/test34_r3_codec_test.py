"""Bounded checkpoint format and annual recovery capacity matrix; no native writes."""
from pathlib import Path
import os,subprocess,json
S=Path(__file__).resolve().parents[1];O=Path(os.environ['DRI_TEST_OUTPUT_DIR']);O.mkdir(parents=True,exist_ok=True)
c=r'''
#include <assert.h>
#include <string.h>
#include "hs34r3_continuity.h"
#include "class34_policy.h"
#include "class34r2_plan.h"
static int valid(Hs34R3Disk *d){d->checksum=hs34r3_checksum(d);return hs34r3_disk_valid(d,123,2026,1,9,55,1234);}
int main(void){
 Hs34R3Disk d={0},good;
 d.magic=0x33505348;d.version=1;d.league=123;d.year=2026;d.type=1;d.week=9;d.tid=55;d.board_hash=1234;
 d.count=2;d.entry[0]=(Hs34R3Entry){100,200,1,33,1,1};d.entry[1]=(Hs34R3Entry){101,201,1,33,2,2};assert(valid(&d));good=d;
 for(size_t i=0;i<sizeof(d);i++){d=good;((unsigned char*)&d)[i]^=1;assert(!hs34r3_disk_valid(&d,123,2026,1,9,55,1234));}
 #define NO(expr) do{d=good;expr;assert(!valid(&d));}while(0)
 NO(d.magic=0);NO(d.version=2);NO(d.league=124);NO(d.year=2027);NO(d.type=2);NO(d.week=10);NO(d.tid=56);NO(d.board_hash=999);NO(d.count=51);
 NO(d.entry[1].rid=100);NO(d.entry[1].order=1);NO(d.entry[1].choice=1);NO(d.entry[0].pos=-1);NO(d.entry[0].pos=22);NO(d.entry[0].slot=0);NO(d.entry[0].slot=-33);NO(d.entry[0].slot=65);NO(d.entry[0].choice=3);NO(d.entry[0].order=0);NO(d.entry[0].order=51);
 d=good;d.week=8;assert(valid(&d));assert(!hs34r3_disk_valid(&d,0,2026,1,9,55,1234));
 /* Full legitimate board bound: 25 pairs with unique order and identities. */
 d=good;d.count=50;for(int i=0;i<50;i++)d.entry[i]=(Hs34R3Entry){100+i,200+i,1,33+i/2,1+i%2,1+i};assert(valid(&d));
 int ideal[22]={4,5,4,6,5,3,3,3,3,3,4,4,6,2,4,4,8,4,4,2,2,0};
 int annual[22],hs[22];
 for(unsigned tid=0;tid<138;tid++)for(int missing=0;missing<21;missing++)for(int extra=1;extra<=18;extra++)if(extra!=missing){
  int base=class31_annual(ideal,tid,2026,annual);if(!annual[missing])continue;
  memcpy(hs,annual,sizeof(hs));hs[missing]--;hs[extra]+=2;
  int secured=0;for(int p=0;p<22;p++)secured+=hs[p];
  assert(base-secured<=0);
  int target=class34r2_capacity(1,base,class34r2_secured_extra(1,annual,hs),0);
  assert(target-secured==(base<=23?1:25-secured)); /* one ordinary vacancy survives the extras */
  hs[missing]++;secured++;assert(target-secured==(base<=23?0:25-secured));
 }
 return 0;
}'''
(O/'checkpoint.c').write_text(c)
subprocess.run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror','-I'+str(S),str(O/'checkpoint.c'),'-o',str(O/'checkpoint')],check=True)
subprocess.run([str(O/'checkpoint')],check=True)
r={'passed':True,'checks':['Every byte corruption rejected','Foreign league/year/team/board and future state rejected','All entry bounds and duplicate roles rejected','Full50-entry checkpoint accepted','138-school ordinary recovery survives two extras at every eligible other position','Completed ordinary intake stops recovery'],'live_game_verified':False}
(O/'test34_r3_codec_results.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r))
