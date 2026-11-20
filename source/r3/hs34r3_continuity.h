#ifndef DRI_HS34R3_CONTINUITY_H
#define DRI_HS34R3_CONTINUITY_H
#include <stdint.h>
#include <stddef.h>
/* Optional sidecar, never a native save field or a saved pointer. A board
   fingerprint selects the exact observed native state, not just a week. */
typedef struct {uint32_t rid,pid;int32_t pos,slot,choice,order;} Hs34R3Entry;
typedef struct {
 uint32_t magic,version,league,year,type,week,tid,count;
 uint64_t board_hash,checksum;
 Hs34R3Entry entry[50];
} Hs34R3Disk;
static inline uint64_t hs34r3_hash(uint64_t h,uint64_t v) {
 for(int i=0;i<8;i++){h^=(unsigned char)v;h*=UINT64_C(1099511628211);v>>=8;}return h;
}
static inline uint64_t hs34r3_checksum(const Hs34R3Disk *d) {
 const unsigned char *b=(const unsigned char*)d;uint64_t h=UINT64_C(1469598103934665603);
 for(size_t i=0;i<sizeof(*d);i++)if(i<offsetof(Hs34R3Disk,checksum)||i>=offsetof(Hs34R3Disk,checksum)+8){h^=b[i];h*=UINT64_C(1099511628211);}
 return h;
}
static inline int hs34r3_disk_valid(const Hs34R3Disk *d,uint32_t league,uint32_t year,
 uint32_t type,uint32_t week,uint32_t tid,uint64_t hash) {
 if(d->magic!=UINT32_C(0x33505348)||d->version!=1||!league||d->league!=league||d->year!=year||
    d->tid!=tid||tid>=138||d->count>50||!hash||d->board_hash!=hash||d->type>6||d->week>60||
    d->type>type||(d->type==type&&d->week>week)||d->checksum!=hs34r3_checksum(d))return 0;
 for(unsigned i=0;i<d->count;i++) {
  const Hs34R3Entry *e=&d->entry[i];
  if(e->pos<0||e->pos>=22||e->slot/32!=e->pos||e->slot%32<1||e->slot%32>31||
     e->choice<1||e->choice>2||e->order<1||e->order>50)return 0;
  for(unsigned j=0;j<i;j++)if(d->entry[j].rid==e->rid||d->entry[j].order==e->order||
    (d->entry[j].slot==e->slot&&d->entry[j].choice==e->choice))return 0;
 }
 return 1;
}
#endif
