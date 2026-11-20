#ifndef DRI_EPS_SNAPSHOT_H
#define DRI_EPS_SNAPSHOT_H
#include "eps_core.h"
#include <stdint.h>
#include <stddef.h>
#define EPS_DISK_MAGIC UINT64_C(0x4550533154455354)
#define EPS_DISK_VERSION 1u
typedef struct {
 uint64_t magic,signature,output_hash,cache_hash;
 uint32_t league,year,index,final,bytes,version;
 EpsOutput output;
} EpsDisk;
uint64_t eps_hash(const void*,size_t);
int eps_disk_valid(const EpsDisk*,unsigned,int,int,uint64_t,int);
#endif
