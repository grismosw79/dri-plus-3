#ifndef TEST34_DIAGNOSTICS_H
#define TEST34_DIAGNOSTICS_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>
/* Pure bounded native-string decoding. Pointer access remains inside the
   caller's exception boundary; no allocator or game value is modified. */
static inline int hs34_reason_payload(const unsigned char data[16],const unsigned char **text,size_t *size) {
 if(data[15]&0x80) {
  uintptr_t pointer=0;uint32_t length=0,capacity=0;
  memcpy(&pointer,data,sizeof(pointer));memcpy(&length,data+8,4);memcpy(&capacity,data+12,4);
  capacity&=0x7fffffffU;if(!pointer || length>capacity)return 0;
  *text=(const unsigned char *)pointer;*size=length;
 } else {
  *text=data;*size=15; /* NUL-bounded; do not interpret the short tag as a length. */
 }
 return 1;
}
static inline void hs34_reason_text(char out[96],const unsigned char *text,size_t size) {
 size_t n=size<95?size:95;
 for(size_t i=0;i<n;i++) {
  unsigned c=text[i];if(!c){n=i;break;}
  out[i]=(c==','?';':c<32 || c==127?' ':c>126?'?':(char)c);
 }
 out[n]=0;
 if(!n)memcpy(out,"NATIVE_REJECTED_NO_REASON",sizeof("NATIVE_REJECTED_NO_REASON"));
}
enum {
 HS34_UNCLASSIFIED=0,HS34_DATA,HS34_USER,HS34_CLOSED_RACE,HS34_ALREADY_OWNED,
 HS34_ANNUAL,HS34_PHYSICAL,HS34_STAR_LIMIT,HS34_QB_LIMIT,HS34_EXCEPTION,
 HS34_ACADEMY,HS34_NATIVE,HS34_VIABILITY,HS34_PERMIT,HS34_SCORE,
 HS34_ACTION_COST,HS34_HOURS,HS34_BOARD,HS34_RESERVED,HS34_CLASS_TARGET,
 HS34_ROLE_OCCUPIED,HS34_BACKUP_ONLY,HS34_ACCEPTED,HS34_CLASS_FILLED,HS34_ROSTER_FULL,HS34_VACANCY_USED,HS34_NEXT_SCOPE,HS34_NIL
};
static inline const char *hs34_reason_label(int code) {
 static const char *const labels[]={"UNCLASSIFIED","DATA_UNAVAILABLE","USER_TEAM","CLOSED_RACE","ALREADY_ACQUIRED",
  "ANNUAL_INTAKE_FILLED","PHYSICAL_POSITION_ROOM","STAR_STACK_LIMIT","ELITE_QB_LIMIT","SHARED_EXCEPTION_ALLOWANCE",
  "ACADEMY_RESTRICTION","NATIVE_ELIGIBILITY","PURSUIT_NOT_CREDIBLE","OFFER_PERMISSION","NATIVE_SCORE_NONPOSITIVE",
  "NO_VALID_ACTION_COST","INSUFFICIENT_HOURS","BOARD_CAPACITY","ROLE_RESERVED","CLASS_TARGET_CAPACITY",
  "POSITION_ROLES_OCCUPIED","CONDITIONAL_BACKUP_ONLY","ADMISSIBLE_DESTINATION",
  "CLASS_TARGET_FILLED","PROJECTED_ROSTER_FULL","POSITION_VACANCY_EXTRA_USED","NEXT_SCOPE_RECHECK_REQUIRED","NIL_QUOTE_UNAFFORDABLE"};
 return code>=0 && (size_t)code<sizeof(labels)/sizeof(labels[0])?labels[code]:"UNKNOWN";
}
#endif
