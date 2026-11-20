/* Runs in the launcher process only. No extra threads, hooks, or UI are added
   to the game. The active launcher must remain open to deliver this alert. */
#include "runtime_log_monitor.h"
#include "failure_dialog.h"
static int find_runtime_log(HANDLE game,const wchar_t *dll_path,DWORD pid,const wchar_t *kind,const wchar_t *extension,wchar_t out[32768]) {
 wchar_t pattern[32768];size_t n=wcslen(dll_path);
 while(n && dll_path[n-1]!=L'\\')n--;
 if(n+128>=32768)return 0;
 memcpy(pattern,dll_path,n*sizeof(wchar_t));
 swprintf(pattern+n,32768-n,L"RM18R22_%ls_*_%lu.%ls",kind,(unsigned long)pid,extension);
 FILETIME started,ended,kernel,user;
 if(!GetProcessTimes(game,&started,&ended,&kernel,&user))return 0;
 WIN32_FIND_DATAW data;HANDLE search=FindFirstFileW(pattern,&data);
 if(search==INVALID_HANDLE_VALUE)return 0;
 FILETIME newest={0};int found=0;
 do {
  if((data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) ||
     CompareFileTime(&data.ftCreationTime,&started)<0)continue;
  if(found && CompareFileTime(&data.ftCreationTime,&newest)<=0)continue;
  size_t length=wcslen(data.cFileName);if(n+length>=32768)continue;
  memcpy(out,dll_path,n*sizeof(wchar_t));wcscpy(out+n,data.cFileName);
  newest=data.ftCreationTime;found=1;
 }while(FindNextFileW(search,&data));
 FindClose(search);return found;
}
static void runtime_monitor_alert(const wchar_t *dll_path,DWORD pid,const char *code) {
 wchar_t message[33792];
 dri_failure_message(dll_path,(unsigned long)pid,code,message,sizeof(message)/sizeof(message[0]));
 fprintf(stderr,"\n%ls\n",message);fflush(stderr);
 MessageBoxW(0,message,L"DRI+ RM18R22 | Runtime stopped",MB_OK|MB_ICONERROR|MB_SETFOREGROUND|MB_TOPMOST);
}
static int monitor_runtime(HANDLE game,const wchar_t *dll_path,DWORD pid) {
 wchar_t path[32768];
 if(!find_runtime_log(game,dll_path,pid,L"Recruiting",L"csv",path)) {
  runtime_monitor_alert(dll_path,pid,"MONITOR_CSV_NOT_FOUND");return 17;
 }
 HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
 if(file==INVALID_HANDLE_VALUE) {
  runtime_monitor_alert(dll_path,pid,"MONITOR_CSV_OPEN_FAILED");return 17;
 }
 printf("Monitoring: %ls\n",path);
 HANDLE prep_files[2]={INVALID_HANDLE_VALUE,INVALID_HANDLE_VALUE};
 DriPrepMonitor prep[2]={{0},{0}};
 const wchar_t *prep_kinds[2]={L"Positions_Development",L"Roster"};
 for(unsigned k=0;k<2;k++) {
  if(find_runtime_log(game,dll_path,pid,prep_kinds[k],L"jsonl",path))
   prep_files[k]=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
  if(prep_files[k]==INVALID_HANDLE_VALUE) {
   for(unsigned j=0;j<k;j++)CloseHandle(prep_files[j]);CloseHandle(file);
   runtime_monitor_alert(dll_path,pid,k?"MONITOR_ROSTER_LOG_OPEN_FAILED":"MONITOR_DEVELOPMENT_LOG_OPEN_FAILED");return 17;
  }
  printf("Monitoring: %ls\n",path);
 }
 puts("DRI_MONITOR_READY: all session logs are open; runtime monitoring is active.");fflush(stdout);
 DriLogMonitor state={0};char buffer[32768];int result=0,process_exited=0;
 for(;;) {
  DWORD prep_bytes=0;
  for(unsigned k=0;k<2;k++) {
   DWORD got=0;
   if(!ReadFile(prep_files[k],buffer,sizeof(buffer),&got,0)) {
    runtime_monitor_alert(dll_path,pid,k?"MONITOR_ROSTER_READ_FAILED":"MONITOR_DEVELOPMENT_READ_FAILED");result=17;goto finished;
   }
   prep_bytes+=got;dri_prep_monitor_feed(&prep[k],buffer,got);
   if(prep[k].fault) {
    /* Preserve the complete component status row in redirected launcher output. */
    fprintf(stderr,"%s\n",prep[k].reason);fflush(stderr);
    runtime_monitor_alert(dll_path,pid,k?"ROSTER_PROCESSING_FAILED":"DEVELOPMENT_PROCESSING_FAILED");
    result=16;goto finished;
   }
  }
  DWORD got=0;
  if(!ReadFile(file,buffer,sizeof(buffer),&got,0)) {
   runtime_monitor_alert(dll_path,pid,"MONITOR_CSV_READ_FAILED");result=17;break;
  }
  if(got) {
   dri_log_monitor_feed(&state,buffer,got);
   if(state.fault) {
    char code[192];dri_fault_code(state.reason,code);
    fprintf(stderr,"%s\n",state.reason);fflush(stderr);
    runtime_monitor_alert(dll_path,pid,code);result=16;break;
   }
   continue;
  }
  /* Inspect intentional mode changes only after draining the existing log:
     a later MODE,1 can reactivate an earlier diagnostic/disabled session. */
  if(prep_bytes)continue;
  if(state.stopped){puts("The runtime was intentionally disabled or switched to diagnostic mode. Monitoring has ended.");break;}
  if(process_exited){puts("Game closed. Session diagnostics retained.");break;}
  DWORD status=WaitForSingleObject(game,1000);
  /* Drain once more after process exit: its final row may have arrived
     between the previous EOF and the signaled process handle. */
  if(status==WAIT_OBJECT_0){process_exited=1;continue;}
  if(status!=WAIT_TIMEOUT) {
   runtime_monitor_alert(dll_path,pid,"MONITOR_PROCESS_WAIT_FAILED");result=17;break;
  }
 }
 finished:
 for(unsigned k=0;k<2;k++)CloseHandle(prep_files[k]);
 CloseHandle(file);return result;
}
