#ifndef DRI_LOG30_POLICY_H
#define DRI_LOG30_POLICY_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
/* Bounded, observational suppression of unchanged candidate diagnostics only.
   Exact key and payload comparisons make hash collisions emit extra evidence,
   never hide a state change. No funding/commitment/ownership/fault event is
   eligible. PORTAL_ROLE is a decision diagnostic, not a funded-action witness.
   Scope is reset at evaluation/timing boundaries, not once a week. The three
   summary counters group decision, plan and defer rows without changing the
   historical summary layout. */
#define LOG30_SLOTS 8192
#define LOG30_KEY 96
#define LOG30_ROW 256
typedef struct { uint32_t epoch; char key[LOG30_KEY],line[LOG30_ROW]; } Log30Entry;
typedef struct { uint32_t epoch; uint64_t suppressed[3]; Log30Entry entries[LOG30_SLOTS]; } Log30State;
static int log30_kind(const char *s) {
 if(!strncmp(s,"HS15_DECISION,",14))return 0;
 if(!strncmp(s,"PORTAL_ROLE,",12))return 0;
 if(!strncmp(s,"HS28_PLAN,",10))return 1;
 if(!strncmp(s,"HS29_QUALITY_PLAN,",18))return 1;
 if(!strncmp(s,"HS28_PAIR_DEFER,",16))return 2;
 return -1;
}
static int log30_boundary(const char *s) {
 return !strncmp(s,"EVAL,",5) || !strncmp(s,"TEST29_PHASE_MS,",16) ||
        !strncmp(s,"FAULT,",6) || !strncmp(s,"DISABLED,",9) || !strncmp(s,"MODE,",5);
}
static void log30_reset(Log30State *s) {
 if(++s->epoch==0){memset(s->entries,0,sizeof(s->entries));s->epoch=1;}
 memset(s->suppressed,0,sizeof(s->suppressed));
}
static int log30_keep(Log30State *s,const char *line,size_t n,int deep) {
 int kind=log30_kind(line);if(deep || kind<0 || n>=LOG30_ROW)return 1;
 /* Quality-plan identity is event/team/recruit; its next value is OVR and
    belongs to the payload. Including it in the key could hide A->B->A changes.
    The other diagnostics include event/site/team/recruit or event/team/week/
    recruit and need four separators. */
 int key_commas=!strncmp(line,"HS29_QUALITY_PLAN,",18)?3:4;
 size_t keylen=0;int commas=0;uint32_t hash=2166136261u;
 for(;keylen<n;keylen++) {
  unsigned char c=(unsigned char)line[keylen];hash=(hash^c)*16777619u;
  if(c==',' && ++commas==key_commas){keylen++;break;}
 }
 if(commas!=key_commas || keylen>=LOG30_KEY)return 1;
 if(!s->epoch)s->epoch=1;
 Log30Entry *e=&s->entries[hash&(LOG30_SLOTS-1)];
 if(e->epoch==s->epoch && !strncmp(e->key,line,keylen) && !e->key[keylen] &&
    !strncmp(e->line,line,n) && !e->line[n]){s->suppressed[kind]++;return 0;}
 e->epoch=s->epoch;memcpy(e->key,line,keylen);e->key[keylen]=0;
 memcpy(e->line,line,n);e->line[n]=0;return 1;
}
#endif
