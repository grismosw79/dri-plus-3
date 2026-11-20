/* RM15 native flow observer. NO roster, recruiting, finance, or development writes. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <wchar.h>

typedef uintptr_t U;
typedef void *Rec;
static U game_base;
/* Resolve before suspending threads: no loader calls inside verify_build. */
static U accepted_r3_base;
static HMODULE self_module;
static volatile LONG enabled, installed, capture_fault;
static LONG64 sequence;
static DWORD tls_index=TLS_OUT_OF_INDEXES;
static HANDLE log_file=INVALID_HANDLE_VALUE;
static SRWLOCK log_lock=SRWLOCK_INIT;
#include "native_profile.h"
#include "../log_io.h"
extern void HookEpsGrade(void);
extern int DriEpsGrade(U,Rec*,int,Rec*);
extern U ResumeEpsGrade;
#include "r3_compat.h"
typedef struct { unsigned kind,rva,index,member;uint64_t expected,actual; } RMVerifyFailure;
static RMVerifyFailure verify_failure;
static int guard_failure(unsigned kind,unsigned rva,unsigned index,unsigned member,uint64_t expected,uint64_t actual) {
    verify_failure=(RMVerifyFailure){kind,rva,index,member,expected,actual};return 0;
}
static int local_read(U address,void *out,size_t n) {
    SIZE_T got=0;return address && ReadProcessMemory(GetCurrentProcess(),(void*)address,out,n,&got) && got==n;
}
static int code_pointer(U p) { return p>=game_base+0x1000 && p<game_base+0xa3b5000; }
static uint64_t hash_bytes(const void *p,size_t n) {
    const unsigned char *b=p;uint64_t h=UINT64_C(14695981039346656037);
    for(size_t i=0;i<n;i++)h=(h^b[i])*UINT64_C(1099511628211);return h;
}
static void emit(const char *body) {
    if(log_file==INVALID_HANDLE_VALUE)return;
    char line[32768];DWORD wrote=0;
    AcquireSRWLockExclusive(&log_lock);
    LONG64 seq=InterlockedIncrement64(&sequence);
    int n=snprintf(line,sizeof(line),"{\"seq\":%lld,\"thread\":%lu,%s}\r\n",(long long)seq,(unsigned long)GetCurrentThreadId(),body);
    if(n<0 || n>=(int)sizeof(line) || !WriteFile(log_file,line,(DWORD)n,&wrote,0) || wrote!=(DWORD)n)InterlockedExchange(&capture_fault,1);
    ReleaseSRWLockExclusive(&log_lock);
}
static __declspec(noinline) int logging_team(void) {
    return tls_index==TLS_OUT_OF_INDEXES ? -1 : (int)(intptr_t)TlsGetValue(tls_index)-1;
}
static __declspec(noinline) void log_line(const char *fmt,...) {
    char raw[1600],body[3400];va_list args;va_start(args,fmt);vsnprintf(raw,sizeof(raw),fmt,args);va_end(args);
    size_t k=(size_t)snprintf(body,sizeof(body),"\"event\":\"STATUS\",\"team\":%d,\"message\":\"",logging_team());
    for(size_t i=0;raw[i] && k+3<sizeof(body);i++) {
        unsigned char c=(unsigned char)raw[i];if(c=='"' || c=='\\')body[k++]='\\';
        body[k++]=c<32?' ':c;
    }
    body[k++]='"';body[k]=0;emit(body);
}
static void capture_error(const char *where) {
    InterlockedExchange(&capture_fault,1);log_line("CAPTURE_FAILED %s; roster failure route will determine whether native continuation is permitted",where);
}
static int verify_build(void) {
    unsigned char b[32];verify_failure=(RMVerifyFailure){0};
    for(size_t i=0;i<sizeof(code_guards)/sizeof(code_guards[0]);i++) {
        const RMCodeGuard *g=&code_guards[i];
        if(!local_read(game_base+g->rva,b,g->size))return guard_failure(1,g->rva,(unsigned)i,0,g->hash,0);
        uint64_t expected=g->hash,actual=hash_bytes(b,g->size);
        if(g->rva==RM_R3_GRADE_RVA) {
            if(!accepted_r3_base){if(actual!=expected)return guard_failure(2,g->rva,(unsigned)i,0,expected,actual);continue;}
            unsigned char patched[32];memcpy(patched,rm_r3_native_grade,sizeof(patched));
            patched[0]=0xff;patched[1]=0x25;memset(patched+2,0,4);
            U target=(U)&HookEpsGrade;memcpy(patched+6,&target,8);
            memset(patched+14,0x90,RM_R3_GRADE_PATCH_SIZE-14);
            expected=hash_bytes(patched,sizeof(patched));
            if(g->size!=sizeof(patched) || memcmp(b,patched,sizeof(patched)))return guard_failure(3,g->rva,(unsigned)i,0,expected,actual);
            unsigned char owned[28]={0xe9};
            int64_t jump=(int64_t)(U)&DriEpsGrade-(int64_t)(target+5);
            int64_t resume=(int64_t)(U)&ResumeEpsGrade-(int64_t)(target+28);
            if(jump<INT32_MIN||jump>INT32_MAX||resume<INT32_MIN||resume>INT32_MAX)return guard_failure(4,g->rva,(unsigned)i,0,0,0);
            int32_t j=(int32_t)jump,r=(int32_t)resume;memcpy(owned+1,&j,4);memcpy(owned+5,rm_r3_native_grade,17);owned[22]=0xff;owned[23]=0x25;memcpy(owned+24,&r,4);
            expected=hash_bytes(owned,sizeof(owned));
            if(!local_read(target,b,sizeof(owned)))return guard_failure(4,g->rva,(unsigned)i,0,expected,0);
            actual=hash_bytes(b,sizeof(owned));
            if(memcmp(b,owned,sizeof(owned))||ResumeEpsGrade!=game_base+RM_R3_GRADE_RVA+RM_R3_GRADE_PATCH_SIZE)return guard_failure(5,g->rva,(unsigned)i,0,expected,actual);
        } else if(actual!=expected)return guard_failure(6,g->rva,(unsigned)i,0,expected,actual);
    }
    for(size_t i=0;i<sizeof(table_guards)/sizeof(table_guards[0]);i++) {
        const RMTableGuard *g=&table_guards[i];uint64_t h=UINT64_C(14695981039346656037);
        for(unsigned j=0;j<g->count;j++) {
            uint64_t pair[2]={0};
            if(!local_read(game_base+g->rva+j*16,pair,sizeof(pair)))return guard_failure(7,g->rva,(unsigned)i,j,g->hash,0);
            if(pair[0]!=j || pair[1]<game_base)return guard_failure(8,g->rva,(unsigned)i,j,j,pair[0]);
            pair[1]-=game_base;
            for(size_t k=0;k<sizeof(pair);k++)h=(h^((unsigned char*)pair)[k])*UINT64_C(1099511628211);
        }
        if(h!=g->hash)return guard_failure(9,g->rva,(unsigned)i,0,g->hash,h);
    }
    return 1;
}
/* Called only after threads are running, never from frozen guard checks. */
static void log_verify_failure(void) {
    if(verify_failure.kind)log_line("VERIFY_FAILURE kind=%u module=%s rva=0x%08x index=%u member=%u expected=0x%llx actual=0x%llx",
        verify_failure.kind,(verify_failure.kind==4 || verify_failure.kind==5)?"accepted_R3":"game",
        verify_failure.rva,verify_failure.index,verify_failure.member,
        (unsigned long long)verify_failure.expected,(unsigned long long)verify_failure.actual);
}

/* Stored fields only: no speculative method invocation. Reference getters own
   one reference, which is released on the same game thread. */
static int field_context(Rec r,unsigned index,U *data,uint32_t *row,U *field) {
    U vt,getter,type,cache;int16_t count;
    if(!r || !local_read((U)r,&vt,8) || !local_read(vt+0x38,&getter,8) || !code_pointer(getter))return 0;
    type=((U(*)(Rec))getter)(r);
    if(!type || !local_read(type+0xd0,&cache,8) || !cache || !local_read(type+0xd8,&count,2) || count<=0 || index>=(unsigned)count)return 0;
    *field=((U(*)(U,unsigned))(game_base+0x8055d60))(type,index);
    return *field!=0xffffffffu && local_read((U)r+0x10,data,8) && *data && local_read((U)r+0x18,row,4);
}
static __declspec(noinline) int scalar(Rec r,unsigned index,unsigned kind,int *value) {
    U data,field,vt,fn;uint32_t row;
    if(!field_context(r,index,&data,&row,&field) || !local_read(data,&vt,8) ||
       !local_read(vt+(kind==1?0xf8:kind==2?0x100:0x108),&fn,8) || !code_pointer(fn))return 0;
    *value=kind==1?((unsigned char(*)(U,uint32_t,U))fn)(data,row,field)!=0:((int(*)(U,uint32_t,U))fn)(data,row,field);
    return 1;
}
static __declspec(noinline) void release(Rec r) {
    U vt,fn;if(!r)return;
    if(local_read((U)r,&vt,8) && local_read(vt+8,&fn,8) && code_pointer(fn))((void(*)(Rec))fn)(r);
    else capture_error("reference release");
}
static __declspec(noinline) Rec reference(Rec r,unsigned index,int *ok) {
    U data,field,vt,fn;uint32_t row;Rec out=0;*ok=0;
    if(!field_context(r,index,&data,&row,&field) || !local_read(data,&vt,8) || !local_read(vt+0x128,&fn,8) || !code_pointer(fn))return 0;
    ((void(*)(U,Rec*,uint32_t,U))fn)(data,&out,row,field);*ok=1;return out;
}
static __declspec(noinline) Rec array_field(Rec r,unsigned index,int *ok) {
    Rec raw=reference(r,index,ok),temp=0,out=0;U vt,fn;
    if(!*ok || !raw)return raw;
    if(!local_read((U)raw,&vt,8) || !local_read(vt+0x28,&fn,8) || !code_pointer(fn)){*ok=0;release(raw);return 0;}
    Rec *result=((Rec*(*)(Rec,Rec*,unsigned))fn)(raw,&temp,0x41525259);
    if(result){out=*result;*result=0;}else *ok=0;
    release(temp);release(raw);return out;
}
static __declspec(noinline) U array_count(Rec a) { return a?((U(*)(Rec))(game_base+0x37f4130))(a):0; }
static __declspec(noinline) Rec array_at(Rec a,U index,int *ok) {
    U data,vt,fn;uint32_t row;Rec out=0;*ok=0;
    if(!a || !local_read((U)a+0x10,&data,8) || !local_read((U)a+0x18,&row,4) ||
       !local_read(data,&vt,8) || !local_read(vt+0x128,&fn,8) || !code_pointer(fn))return 0;
    ((void(*)(U,Rec*,uint32_t,U))fn)(data,&out,row,index);*ok=1;return out;
}
static int identity(Rec r,U *store,uint32_t *row) {
    return r && local_read((U)r+0x10,store,8) && local_read((U)r+0x18,row,4);
}
static int append(char *b,size_t cap,size_t *len,const char *fmt,...) {
    if(*len>=cap)return 0;va_list args;va_start(args,fmt);int n=vsnprintf(b+*len,cap-*len,fmt,args);va_end(args);
    if(n<0 || (size_t)n>=cap-*len)return 0;*len+=(size_t)n;return 1;
}

__declspec(dllexport) const struct {char tag[24];U p[3];} rm18_owned_eps={"RM18R22-OWNED-EPS",{(U)&HookEpsGrade,(U)&DriEpsGrade,(U)&ResumeEpsGrade}};
