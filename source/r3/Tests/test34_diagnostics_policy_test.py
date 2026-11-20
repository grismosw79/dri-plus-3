"""Native reason layout, bounded capture and CSV sanitation controls."""
from pathlib import Path
import os, subprocess, tempfile, json
S=Path(__file__).resolve().parent.parent
code=r'''
#include <assert.h>
#include <stdio.h>
#include "test34_diagnostics.h"
int main(void){
 unsigned char data[16]={0};const unsigned char *p=0;size_t n=0;char text[96];
 data[15]=15;assert(hs34_reason_payload(data,&p,&n));hs34_reason_text(text,p,n);
 assert(!strcmp(text,"NATIVE_REJECTED_NO_REASON"));
 memcpy(data,"Playing,Time\n",13);assert(hs34_reason_payload(data,&p,&n));hs34_reason_text(text,p,n);
 assert(!strcmp(text,"Playing;Time "));
 memset(data,'X',15);data[15]=0;assert(hs34_reason_payload(data,&p,&n));hs34_reason_text(text,p,n);assert(strlen(text)==15);
 unsigned char heap[200];memset(heap,'Q',sizeof(heap));uintptr_t pointer=(uintptr_t)heap;
 uint32_t len=200,cap=0x80000000U|200;memcpy(data,&pointer,8);memcpy(data+8,&len,4);memcpy(data+12,&cap,4);
 assert(hs34_reason_payload(data,&p,&n));assert(p==heap && n==200);hs34_reason_text(text,p,n);assert(strlen(text)==95 && text[95]==0);
 len=201;memcpy(data+8,&len,4);assert(!hs34_reason_payload(data,&p,&n));
 pointer=0;memcpy(data,&pointer,8);assert(!hs34_reason_payload(data,&p,&n));
 assert(!strcmp(hs34_reason_label(HS34_ANNUAL),"ANNUAL_INTAKE_FILLED"));
 assert(!strcmp(hs34_reason_label(HS34_NATIVE),"NATIVE_ELIGIBILITY"));
 assert(!strcmp(hs34_reason_label(HS34_NEXT_SCOPE),"NEXT_SCOPE_RECHECK_REQUIRED"));
 assert(strcmp(hs34_reason_label(HS34_NEXT_SCOPE),hs34_reason_label(HS34_ACCEPTED)));
 assert(!strcmp(hs34_reason_label(999),"UNKNOWN"));
 puts("PASS bounded reason layouts and diagnostics labels");return 0;
}
'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t)/'check.c';p.write_text(code);exe=Path(t)/'check'
 subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-I'+str(S),str(p),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
out=Path(os.environ.get('DRI_TEST_OUTPUT_DIR','.'))
(out/'test34_diagnostics_policy_results.json').write_text(json.dumps({'passed':True,'checks':['empty inline reason','SSO non-length tag and CSV sanitation','full inline15','heap bounded95','invalid heap bounds','reason labels']},indent=2)+'\n')
