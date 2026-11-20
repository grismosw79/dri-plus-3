/* RM13R1: one guarded process-local native enrollment immediate.
   No player writes, detours, native calls, save edits, allocations or R3 rebuild.
   Activate at idle ENC with the validated RM13 90-limit save. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <stdint.h>
#include "native_guard.h"

static volatile LONG busy=0;
static int active=0;
static uintptr_t game_base=0;
static int match_guard(unsigned char immediate) {
    unsigned char b[ENROLL_SIZE]; SIZE_T got=0;
    if(!ReadProcessMemory(GetCurrentProcess(),(void*)(game_base+ENROLL_RVA),b,sizeof(b),&got)||got!=sizeof(b))return 0;
    for(unsigned i=0;i<sizeof(b);i++)if(b[i]!=(i==PATCH_OFFSET?immediate:enrollment_original[i]))return 0;
    return 1;
}
static DWORD change_guard(int enable) {
    HANDLE handles[512];DWORD count=0,suspended=0,result=0,old_protect=0;
    unsigned char before=active?0:4,after=enable?0:4;
    if(!match_guard(before))return 3;
    if(enable==active)return 0;
    DWORD pid=GetCurrentProcessId(),tid=GetCurrentThreadId();
    HANDLE snap=CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);
    if(snap==INVALID_HANDLE_VALUE)return 4;
    THREADENTRY32 t={0};t.dwSize=sizeof(t);
    BOOL more=Thread32First(snap,&t);
    if(!more){CloseHandle(snap);return 4;}
    while(more) {
        if(t.th32OwnerProcessID==pid && t.th32ThreadID!=tid) {
            if(count==512){result=5;break;}
            HANDLE h=OpenThread(THREAD_SUSPEND_RESUME|THREAD_GET_CONTEXT|THREAD_QUERY_INFORMATION,FALSE,t.th32ThreadID);
            if(!h){result=5;break;}
            handles[count++]=h;
        }
        t.dwSize=sizeof(t);more=Thread32Next(snap,&t);
    }
    if(!result && GetLastError()!=ERROR_NO_MORE_FILES)result=4;
    CloseHandle(snap);
    if(result)goto cleanup;
    if(!count){result=5;goto cleanup;}
    for(DWORD i=0;i<count;i++) {
        if(SuspendThread(handles[i])==(DWORD)-1){result=6;goto cleanup;}
        suspended++;
        CONTEXT ctx={0};ctx.ContextFlags=CONTEXT_CONTROL;
        if(!GetThreadContext(handles[i],&ctx)){result=7;goto cleanup;}
        if(ctx.Rip>=game_base+ENROLL_RVA && ctx.Rip<game_base+ENROLL_RVA+ENROLL_SIZE){result=7;goto cleanup;}
    }
    if(!match_guard(before)){result=3;goto cleanup;}
    unsigned char *target=(unsigned char*)(game_base+ENROLL_RVA+PATCH_OFFSET);
    if(!VirtualProtect(target,1,PAGE_EXECUTE_READWRITE,&old_protect)){result=8;goto cleanup;}
    *(volatile unsigned char*)target=after;
    int flushed=FlushInstructionCache(GetCurrentProcess(),target,1);
    int verified=match_guard(after);
    DWORD ignored=0;
    int restored=VirtualProtect(target,1,old_protect,&ignored);
    if(!flushed || !verified || !restored) {
        DWORD scratch=0;
        if(!VirtualProtect(target,1,PAGE_EXECUTE_READWRITE,&scratch)){result=12;goto cleanup;}
        *(volatile unsigned char*)target=before;
        int rollback_flush=FlushInstructionCache(GetCurrentProcess(),target,1);
        int rollback_verify=match_guard(before);
        int rollback_protect=VirtualProtect(target,1,old_protect,&scratch);
        result=rollback_flush&&rollback_verify&&rollback_protect?9:12;
    }else active=enable;
cleanup:
    while(suspended){suspended--;if(ResumeThread(handles[suspended])==(DWORD)-1)result=13;}
    for(DWORD i=0;i<count;i++)CloseHandle(handles[i]);
    return result;
}
static DWORD run(int enable) {
    if(InterlockedCompareExchange(&busy,1,0))return 10;
    DWORD result=0;game_base=(uintptr_t)GetModuleHandleW(0);
    if(!game_base)result=2;else result=change_guard(enable);
    InterlockedExchange(&busy,0);return result;
}
__declspec(dllexport) DWORD WINAPI DriInitialize(void *unused){(void)unused;return run(1);}
__declspec(dllexport) DWORD WINAPI DriDisable(void *unused){(void)unused;return run(0);}
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID reserved){(void)instance;(void)reason;(void)reserved;return TRUE;}

int Rm18ReservePreflight(void){game_base=(uintptr_t)GetModuleHandleW(0);return match_guard(4);}
