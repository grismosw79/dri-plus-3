/* Exercises the actual launcher monitor with a deterministic Win32 boundary.
   This verifies control flow and API contracts; it does not display a real GUI. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

typedef void *HANDLE;
typedef uint32_t DWORD;
typedef uint16_t WORD;
typedef int BOOL;
typedef struct { DWORD dwLowDateTime, dwHighDateTime; } FILETIME;
typedef struct {
 DWORD dwFileAttributes;
 FILETIME ftCreationTime;
 wchar_t cFileName[260];
} WIN32_FIND_DATAW;
typedef struct { WORD wAttributes; } CONSOLE_SCREEN_BUFFER_INFO;
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)
#define GAME ((HANDLE)(uintptr_t)1)
#define SEARCH ((HANDLE)(uintptr_t)2)
#define LOGFILE ((HANDLE)(uintptr_t)3)
#define CONSOLE ((HANDLE)(uintptr_t)4)
#define FILE_ATTRIBUTE_DIRECTORY 16u
#define FILE_ATTRIBUTE_NORMAL 128u
#define GENERIC_READ 0x80000000u
#define FILE_SHARE_READ 1u
#define FILE_SHARE_WRITE 2u
#define FILE_SHARE_DELETE 4u
#define OPEN_EXISTING 3u
#define STD_OUTPUT_HANDLE ((DWORD)-11)
#define FOREGROUND_RED 4u
#define FOREGROUND_INTENSITY 8u
#define MB_OK 0u
#define MB_ICONERROR 16u
#define MB_SETFOREGROUND 65536u
#define MB_TOPMOST 262144u
#define CP_UTF8 65001u
#define WAIT_OBJECT_0 0u
#define WAIT_TIMEOUT 258u
#define WAIT_FAILED 0xffffffffu

typedef struct { const wchar_t *name; DWORD created, attributes; } FakeEntry;
typedef struct { const char *bytes; size_t size; int fail; } FakeRead;
typedef struct { DWORD result; const char *append; } FakeWait;
static FakeEntry entries[20];
static FakeRead reads[40];
static FakeWait waits[20];
static int entry_count, entry_cursor, read_count, read_cursor, wait_count, wait_cursor;
static int process_times_ok, open_ok, conversion_ok, console_info_ok;
static int find_closes, file_closes, opens, alerts, attributes, read_calls, wait_calls;
static WORD attribute_values[10];
static wchar_t seen_pattern[32768], opened_path[32768], alert_message[2048];
static DWORD alert_flags;
static const wchar_t *dll_path=L"C:\\DRI\\Portal\\DRI.Portal.Runtime.dll";
static const wchar_t *valid_name=L"DRI_PortalTest28R12_2026_10_25_111111_1234.csv";

static void add_entry(const wchar_t *name,DWORD created,DWORD flags) {
 assert(entry_count<(int)(sizeof(entries)/sizeof(entries[0])));
 entries[entry_count++]=(FakeEntry){name,created,flags};
}
static void add_bytes(const char *bytes,size_t size) {
 assert(read_count<(int)(sizeof(reads)/sizeof(reads[0])));
 reads[read_count++]=(FakeRead){bytes,size,0};
}
static void add_read(const char *bytes) { add_bytes(bytes,strlen(bytes)); }
static void add_wait(DWORD result,const char *append) {
 assert(wait_count<(int)(sizeof(waits)/sizeof(waits[0])));
 waits[wait_count++]=(FakeWait){result,append};
}
static void reset(void) {
 memset(entries,0,sizeof(entries));memset(reads,0,sizeof(reads));memset(waits,0,sizeof(waits));
 entry_count=entry_cursor=read_count=read_cursor=wait_count=wait_cursor=0;
 process_times_ok=open_ok=conversion_ok=console_info_ok=1;
 find_closes=file_closes=opens=alerts=attributes=read_calls=wait_calls=0;
 seen_pattern[0]=opened_path[0]=alert_message[0]=0;alert_flags=0;
 add_entry(valid_name,1001,0);
}
static BOOL GetProcessTimes(HANDLE game,FILETIME *start,FILETIME *end,FILETIME *kernel,FILETIME *user) {
 assert(game==GAME);(void)end;(void)kernel;(void)user;
 *start=(FILETIME){1000,0};return process_times_ok;
}
static int CompareFileTime(const FILETIME *a,const FILETIME *b) {
 uint64_t av=((uint64_t)a->dwHighDateTime<<32)|a->dwLowDateTime;
 uint64_t bv=((uint64_t)b->dwHighDateTime<<32)|b->dwLowDateTime;
 return av>bv?1:av<bv?-1:0;
}
static BOOL next_matching_entry(WIN32_FIND_DATAW *data) {
 const wchar_t *pattern=wcsrchr(seen_pattern,L'\\');pattern=pattern?pattern+1:seen_pattern;
 const wchar_t *star=wcschr(pattern,L'*');assert(star);
 size_t prefix=(size_t)(star-pattern),suffix=wcslen(star+1);
 while(entry_cursor<entry_count) {
  FakeEntry e=entries[entry_cursor++];size_t length=wcslen(e.name);
  if(length<prefix+suffix || wcsncmp(e.name,pattern,prefix) || wcscmp(e.name+length-suffix,star+1))continue;
  memset(data,0,sizeof(*data));data->dwFileAttributes=e.attributes;
  data->ftCreationTime=(FILETIME){e.created,0};wcscpy(data->cFileName,e.name);return 1;
 }
 return 0;
}
static HANDLE FindFirstFileW(const wchar_t *pattern,WIN32_FIND_DATAW *data) {
 wcscpy(seen_pattern,pattern);entry_cursor=0;
 return next_matching_entry(data)?SEARCH:INVALID_HANDLE_VALUE;
}
static BOOL FindNextFileW(HANDLE search,WIN32_FIND_DATAW *data) {
 assert(search==SEARCH);return next_matching_entry(data);
}
static BOOL FindClose(HANDLE search) {assert(search==SEARCH);find_closes++;return 1;}
static HANDLE CreateFileW(const wchar_t *path,DWORD access,DWORD sharing,void *security,DWORD disposition,DWORD flags,HANDLE template_file) {
 assert(access==GENERIC_READ && sharing==(FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE));
 assert(!security && disposition==OPEN_EXISTING && flags==FILE_ATTRIBUTE_NORMAL && !template_file);
 wcscpy(opened_path,path);opens++;return open_ok?LOGFILE:INVALID_HANDLE_VALUE;
}
static BOOL ReadFile(HANDLE file,void *buffer,DWORD capacity,DWORD *got,void *overlapped) {
 assert(file==LOGFILE && !overlapped);assert(++read_calls<100);
 *got=0;if(read_cursor==read_count)return 1;
 FakeRead r=reads[read_cursor++];if(r.fail)return 0;
 assert(r.size<=capacity);if(r.size)memcpy(buffer,r.bytes,r.size);*got=(DWORD)r.size;return 1;
}
static DWORD WaitForSingleObject(HANDLE game,DWORD timeout) {
 assert(game==GAME && timeout==1000);assert(++wait_calls<40);
 if(wait_cursor==wait_count)return WAIT_OBJECT_0;
 FakeWait w=waits[wait_cursor++];if(w.append)add_read(w.append);return w.result;
}
static BOOL CloseHandle(HANDLE handle) {assert(handle==LOGFILE);file_closes++;return 1;}
static HANDLE GetStdHandle(DWORD id) {assert(id==STD_OUTPUT_HANDLE);return CONSOLE;}
static BOOL GetConsoleScreenBufferInfo(HANDLE console,CONSOLE_SCREEN_BUFFER_INFO *state) {
 assert(console==CONSOLE);if(console_info_ok)state->wAttributes=7;return console_info_ok;
}
static BOOL SetConsoleTextAttribute(HANDLE console,WORD value) {
 assert(console==CONSOLE && attributes<10);attribute_values[attributes++]=value;return 1;
}
static int MessageBoxW(HANDLE owner,const wchar_t *message,const wchar_t *title,DWORD flags) {
 assert(!owner && !wcscmp(title,L"DRI+ Test28R12 - runtime stopped"));
 wcscpy(alert_message,message);alert_flags=flags;alerts++;return 1;
}
static int MultiByteToWideChar(unsigned code_page,DWORD flags,const char *bytes,int length,wchar_t *wide,int capacity) {
 assert(code_page==CP_UTF8 && !flags && length==-1);
 if(!conversion_ok)return 0;
 size_t size=strlen(bytes)+1;assert(size<=(size_t)capacity);
 for(size_t i=0;i<size;i++){assert((unsigned char)bytes[i]<128);wide[i]=(unsigned char)bytes[i];}
 return (int)size;
}

#include "launcher_monitor.inc"

static void assert_alert(int code,const wchar_t *contains,int opened) {
 int result=monitor_runtime(GAME,dll_path,1234);
 assert(result==code && alerts==1 && wcsstr(alert_message,contains));
 assert(alert_flags==(MB_OK|MB_ICONERROR|MB_SETFOREGROUND|MB_TOPMOST));
 assert(opens==opened && file_closes==(opened && open_ok));
 assert(attributes==(console_info_ok?2:1));
 assert(attribute_values[0]==(FOREGROUND_RED|FOREGROUND_INTENSITY));
 if(console_info_ok)assert(attribute_values[1]==7);
}
static void check_log_selection(void) {
 wchar_t path[32768];reset();entry_count=0;
 add_entry(L"DRI_PortalTest28R12_old_1234.csv",999,0);
 add_entry(L"DRI_PortalTest28R12_newer_1234.csv",1100,0);
 add_entry(L"DRI_PortalTest28R12_other_pid_4321.csv",5000,0);
 add_entry(L"DRI_PortalTest15_old_version_1234.csv",6000,0);
 add_entry(L"DRI_PortalTest28R7_previous_version_1234.csv",6500,0);
 add_entry(L"DRI_PortalTest28R12_directory_1234.csv",7000,FILE_ATTRIBUTE_DIRECTORY);
 add_entry(L"DRI_PortalTest28R12_older_1234.csv",1001,0);
 assert(find_runtime_log(GAME,dll_path,1234,path));
 assert(!wcscmp(seen_pattern,L"C:\\DRI\\Portal\\DRI_PortalTest28R12_*_1234.csv"));
 assert(!wcscmp(path,L"C:\\DRI\\Portal\\DRI_PortalTest28R12_newer_1234.csv"));
 assert(find_closes==1 && !opens);
 reset();entry_count=0;add_entry(L"DRI_PortalTest28R12_old_1234.csv",999,0);
 assert_alert(17,L"could not be found",0);assert(find_closes==1);
 reset();entry_count=0;add_entry(L"DRI_PortalTest28R12_other_pid_4321.csv",1100,0);
 assert_alert(17,L"could not be found",0);assert(find_closes==0);
 reset();process_times_ok=0;assert_alert(17,L"could not be found",0);assert(!find_closes && !seen_pattern[0]);
 reset();entry_count=0;assert_alert(17,L"could not be found",0);assert(!find_closes);
 reset();open_ok=0;assert_alert(17,L"could not be opened",1);assert(find_closes==1);
}
static void check_streaming_and_modes(void) {
 reset();add_read("READY,ACTIVE,game_base_0000000140000000\r\nPORTAL_SCORE,1,2,FAULT,4\r\n");
 assert(monitor_runtime(GAME,dll_path,1234)==0);assert(!alerts && file_closes==1 && wait_calls==1);
 const char *fault="FAULT,portal_HS_projection,behavior_disabled_restart_game\r\n";
 /* Exercise actual ReadFile/control-flow handling at every fault split. */
 for(size_t split=1;split<strlen(fault);split++) {
  reset();add_read("READY,ACTIVE,base\r\n");add_bytes(fault,split);add_bytes(fault+split,strlen(fault)-split);
  assert_alert(16,L"FAULT,portal_HS_projection,behavior_disabled_restart_game",1);
  assert(read_cursor==3 && wait_calls==0 && find_closes==1);
 }
 reset();add_read("READY,DIAGNOSTIC,base\nMODE,0\nDISABLED,behavior_passthrough_until_process_exit\r\n");
 add_read("MODE,");add_read("1\r\nPORTAL_SCORE,1,2,3\n");
 add_wait(WAIT_TIMEOUT,"FAULT,later_after_reactivation,behavior_disabled_restart_game\n");
 assert_alert(16,L"later_after_reactivation",1);assert(wait_calls==1 && read_cursor==4);
 reset();add_read("READY,ACTIVE,base\nDISABLED,behavior_passthrough_until_process_exit\n");
 assert(monitor_runtime(GAME,dll_path,1234)==0);assert(!alerts && file_closes==1 && !wait_calls);
 reset();add_read("READY,ACTIVE,base\nMODE,0\n");
 assert(monitor_runtime(GAME,dll_path,1234)==0);assert(!alerts && file_closes==1 && !wait_calls);
 reset();add_read("READY,ACTIVE,base\n");add_wait(WAIT_TIMEOUT,0);add_wait(WAIT_TIMEOUT,fault);
 assert_alert(16,L"portal_HS_projection",1);assert(wait_calls==2);
 reset();conversion_ok=0;add_read(fault);assert_alert(16,L"See the final FAULT row",1);
 reset();console_info_ok=0;add_read(fault);assert_alert(16,L"portal_HS_projection",1);
}
static void check_failures_and_exit(void) {
 reset();reads[read_count++]=(FakeRead){0,0,1};assert_alert(17,L"log monitoring failed",1);assert(!wait_calls);
 reset();add_read("READY,ACTIVE,base\n");add_wait(WAIT_FAILED,0);
 assert_alert(17,L"lost its game-process monitor",1);assert(wait_calls==1);
 reset();add_read("READY,ACTIVE,base\n");add_wait(WAIT_TIMEOUT,0);add_wait(WAIT_OBJECT_0,0);
 assert(monitor_runtime(GAME,dll_path,1234)==0);assert(!alerts && file_closes==1 && wait_calls==2);
 /* A final flush can happen after an EOF read but before process signalled.
    The monitor must drain that final content before declaring clean exit. */
 reset();add_read("READY,ACTIVE,base\n");
 add_wait(WAIT_OBJECT_0,"FAULT,last_flush_at_process_exit,behavior_disabled_restart_game\n");
 assert_alert(16,L"last_flush_at_process_exit",1);assert(wait_calls==1);
}
int main(void) {
 check_log_selection();check_streaming_and_modes();check_failures_and_exit();
 puts("PASS: real launcher monitor, PID/creation/version selection, file sharing, split streaming, alerts, modes, failures, handle cleanup, final flush at exit (mock Win32; no live GUI claim)");
 return 0;
}
