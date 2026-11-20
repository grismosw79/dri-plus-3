/* Exercises the shipped monitor code through deterministic Win32 fixtures.
   No real Windows GUI or game process is used. */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
typedef void *HANDLE;
typedef uint32_t DWORD;
typedef struct {DWORD dwLowDateTime,dwHighDateTime;} FILETIME;
typedef struct {DWORD dwFileAttributes;FILETIME ftCreationTime;wchar_t cFileName[260];} WIN32_FIND_DATAW;
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)
#define GAME ((HANDLE)(uintptr_t)1)
#define SEARCH ((HANDLE)(uintptr_t)2)
#define FILE_ATTRIBUTE_DIRECTORY 16u
#define FILE_ATTRIBUTE_NORMAL 128u
#define GENERIC_READ 0x80000000u
#define FILE_SHARE_READ 1u
#define FILE_SHARE_WRITE 2u
#define FILE_SHARE_DELETE 4u
#define OPEN_EXISTING 3u
#define MB_OK 0u
#define MB_ICONERROR 16u
#define MB_SETFOREGROUND 65536u
#define MB_TOPMOST 262144u
#define WAIT_OBJECT_0 0u
#define WAIT_TIMEOUT 258u
#define WAIT_FAILED 0xffffffffu
static const wchar_t *names[]={L"RM18R22_Recruiting_current_1234.csv",L"RM18R22_Positions_Development_current_1234.jsonl",L"RM18R22_Roster_current_1234.jsonl"};
static int missing,open_fail,read_fail,wait_fail,opened,closed,alerts,waits,search_kind;
static const char *chunks[3][8],*exit_append;
static unsigned counts[3],cursor[3];
static wchar_t message[2048];
static void reset(void){missing=open_fail=read_fail=-1;wait_fail=opened=closed=alerts=waits=0;memset(chunks,0,sizeof(chunks));memset(counts,0,sizeof(counts));memset(cursor,0,sizeof(cursor));exit_append=0;message[0]=0;}
static void add(int file,const char *s){assert(counts[file]<8);chunks[file][counts[file]++]=s;}
static int GetProcessTimes(HANDLE h,FILETIME *s,FILETIME *a,FILETIME *b,FILETIME *c){(void)a;(void)b;(void)c;assert(h==GAME);*s=(FILETIME){100,0};return 1;}
static int CompareFileTime(const FILETIME *a,const FILETIME *b){return a->dwLowDateTime>b->dwLowDateTime?1:a->dwLowDateTime<b->dwLowDateTime?-1:0;}
static HANDLE FindFirstFileW(const wchar_t *pattern,WIN32_FIND_DATAW *out){
 search_kind=wcsstr(pattern,L"Recruiting")?0:wcsstr(pattern,L"Positions_Development")?1:2;
 assert(wcsstr(pattern,L"RM18R22_")&&wcsstr(pattern,L"_1234."));
 if(missing==search_kind)return INVALID_HANDLE_VALUE;
 memset(out,0,sizeof(*out));out->ftCreationTime.dwLowDateTime=101;wcscpy(out->cFileName,names[search_kind]);return SEARCH;
}
static int FindNextFileW(HANDLE h,WIN32_FIND_DATAW *out){(void)out;assert(h==SEARCH);return 0;}
static int FindClose(HANDLE h){assert(h==SEARCH);return 1;}
static HANDLE CreateFileW(const wchar_t *p,DWORD a,DWORD share,void *s,DWORD d,DWORD f,HANDLE t){
 assert(a==GENERIC_READ&&share==(FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE)&&!s&&d==OPEN_EXISTING&&f==FILE_ATTRIBUTE_NORMAL&&!t);
 int k=wcsstr(p,L"Recruiting")?0:wcsstr(p,L"Positions_Development")?1:2;
 if(open_fail==k)return INVALID_HANDLE_VALUE;opened++;return (HANDLE)(uintptr_t)(10+k);
}
static int ReadFile(HANDLE h,void *out,DWORD capacity,DWORD *got,void *o){
 int k=(int)(uintptr_t)h-10;assert(k>=0&&k<3&&!o);*got=0;if(read_fail==k)return 0;
 if(cursor[k]==counts[k])return 1;const char *s=chunks[k][cursor[k]++];size_t n=strlen(s);assert(n<=capacity);memcpy(out,s,n);*got=(DWORD)n;return 1;
}
static DWORD WaitForSingleObject(HANDLE h,DWORD ms){assert(h==GAME&&ms==1000&&++waits<10);if(exit_append){add(0,exit_append);exit_append=0;}return wait_fail?WAIT_FAILED:WAIT_OBJECT_0;}
static int CloseHandle(HANDLE h){assert((uintptr_t)h>=10&&(uintptr_t)h<=12);closed++;return 1;}
static int MessageBoxW(HANDLE owner,const wchar_t *text,const wchar_t *title,DWORD flags){
 assert(!owner&&!wcscmp(title,L"DRI+ RM18R22 | Runtime stopped")&&flags==(MB_OK|MB_ICONERROR|MB_SETFOREGROUND|MB_TOPMOST));
 assert(wcslen(text)<450);wcscpy(message,text);alerts++;return 1;
}
#include "../source/launcher_monitor.inc"
static int checks;
static void run(int expected,const wchar_t *code){
 int result=monitor_runtime(GAME,L"C:\\DRI\\DRIPlus3-RM18R22.dll",1234);
 assert(result==expected&&opened==closed);
 if(expected){assert(alerts==1&&wcsstr(message,code)&&wcsstr(message,L"COLLECT-DRI-DIAGNOSTICS.cmd")&&wcsstr(message,L"last good dynasty save")&&!wcsstr(message,L"behavior_disabled"));}
 else assert(!alerts);
 checks++;
}
int main(void){
 reset();add(0,"READY,ACTIVE,base\n");run(0,0);
 const char *fault="FAULT,PP28_refresh,0xc0000005,behavior_disabled_restart_game\n";
 char a[100],b[100];
 for(size_t split=1;split<strlen(fault);split++){
  reset();memcpy(a,fault,split);a[split]=0;strcpy(b,fault+split);add(0,a);add(0,b);run(16,L"PP28_refresh / 0xc0000005");
 }
 reset();add(0,"MODE,0\nMODE,1\n");exit_append=fault;run(16,L"PP28_refresh / 0xc0000005");assert(waits==1);
 reset();add(0,"DISABLED,intentional\n");run(0,0);assert(waits==0);
 reset();add(0,"CAR35_GENERATION_SUMMARY,fixture\n");run(0,0);
 const wchar_t *missing_code[]={L"MONITOR_CSV_NOT_FOUND",L"MONITOR_DEVELOPMENT_LOG_OPEN_FAILED",L"MONITOR_ROSTER_LOG_OPEN_FAILED"};
 const wchar_t *open_code[]={L"MONITOR_CSV_OPEN_FAILED",L"MONITOR_DEVELOPMENT_LOG_OPEN_FAILED",L"MONITOR_ROSTER_LOG_OPEN_FAILED"};
 const wchar_t *read_code[]={L"MONITOR_CSV_READ_FAILED",L"MONITOR_DEVELOPMENT_READ_FAILED",L"MONITOR_ROSTER_READ_FAILED"};
 for(int i=0;i<3;i++){
  reset();missing=i;run(17,missing_code[i]);
  reset();open_fail=i;run(17,open_code[i]);
  reset();read_fail=i;run(17,read_code[i]);
 }
 reset();wait_fail=1;run(17,L"MONITOR_PROCESS_WAIT_FAILED");
 reset();add(1,"{\"event\":\"STATUS\",\"message\":\"CAPTURE_FAILED fixture details\"}\n");run(16,L"DEVELOPMENT_PROCESSING_FAILED");
 reset();add(2,"{\"event\":\"STATUS\",\"message\":\"RM17R3 FAILED_DISCARD_BRANCH fixture details\"}\n");run(16,L"ROSTER_PROCESSING_FAILED");
 char code[192];dri_fault_code("FAULT,bad\ncode,0xc0000005",code);assert(!strcmp(code,"RUNTIME_FAULT"));
 dri_fault_code("FAULT,portal_HS30_preference_identity,behavior_disabled",code);assert(!strcmp(code,"portal_HS30_preference_identity"));
 printf("PASS %d actual release-monitor scenarios plus malformed/identity code checks; mocked Win32, no GUI claim\n",checks);
 return 0;
}
