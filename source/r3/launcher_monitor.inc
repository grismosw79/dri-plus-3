/* Runs in the launcher process only. No extra threads, hooks, or UI are added
   to the game. The active launcher must remain open to deliver this alert. */
#include "runtime_log_monitor.h"
#include "car35_generation_monitor.h"
static int find_runtime_log(HANDLE game,const wchar_t *dll_path,DWORD pid,wchar_t out[32768]) {
 wchar_t pattern[32768];size_t n=wcslen(dll_path);
 while(n && dll_path[n-1]!=L'\\')n--;
 if(n+128>=32768)return 0;
 memcpy(pattern,dll_path,n*sizeof(wchar_t));
 swprintf(pattern+n,32768-n,L"DRI_PortalTest36R3_*_%lu.csv",(unsigned long)pid);
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
static void runtime_monitor_alert(const wchar_t *message) {
 HANDLE console=GetStdHandle(STD_OUTPUT_HANDLE);CONSOLE_SCREEN_BUFFER_INFO state;
 int restore=GetConsoleScreenBufferInfo(console,&state)!=0;
 SetConsoleTextAttribute(console,FOREGROUND_RED|FOREGROUND_INTENSITY);
 fprintf(stderr,"\n%ls\n",message);fflush(stderr);
 if(restore)SetConsoleTextAttribute(console,state.wAttributes);
 MessageBoxW(0,message,L"DRI+ Test36R3 - runtime stopped",MB_OK|MB_ICONERROR|MB_SETFOREGROUND|MB_TOPMOST);
}
static int monitor_runtime(HANDLE game,const wchar_t *dll_path,DWORD pid) {
 wchar_t path[32768];
 if(!find_runtime_log(game,dll_path,pid,path)) {
  runtime_monitor_alert(L"The runtime activated, but its current Test36R3 CSV could not be found.\n\nClose and restart the game before testing. Keep the launcher message so this can be checked.");return 17;
 }
 HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
 if(file==INVALID_HANDLE_VALUE) {
  runtime_monitor_alert(L"The runtime activated, but its Test36R3 CSV could not be opened for monitoring.\n\nClose and restart the game before testing. Keep the CSV and launcher message.");return 17;
 }
 printf("Monitoring: %ls\n",path);
 puts("Keep this window open during the test. It will alert you if the runtime stops.");fflush(stdout);
 DriLogMonitor state={0};Car35GenerationMonitor generation={0};char buffer[32768];int result=0,process_exited=0;
 for(;;) {
  DWORD got=0;
  if(!ReadFile(file,buffer,sizeof(buffer),&got,0)) {
   runtime_monitor_alert(L"Test36R3 log monitoring failed.\n\nStop advancing this test, retain its CSV and checkpoints, and close the game before retrying.");result=17;break;
  }
  if(got) {
   dri_log_monitor_feed(&state,buffer,got);
   if(state.fault) {
    wchar_t reason[512],message[1024];
    if(!MultiByteToWideChar(CP_UTF8,0,state.reason,-1,reason,512))wcscpy(reason,L"See the final FAULT row in the CSV.");
    swprintf(message,1024,L"Test36R3 reported a runtime failure.\n\n%ls\n\nStop this test and keep the CSV and last good save. Close the game before retrying. Keep the last good checkpoint for review.",reason);
    runtime_monitor_alert(message);result=16;break;
   }
   if(car35_generation_feed(&generation,buffer,got)) {
    puts("\nCOACH GENERATION BOUNDARY CAPTURED. Finish the current advance, then keep a separate GEN-POST save and this runtime folder. Preserve the GEN-PRE checkpoint and its matching cache pair.\n");fflush(stdout);
   }
   continue;
  }
  /* Inspect intentional mode changes only after draining the existing log:
     a later MODE,1 can reactivate an earlier diagnostic/disabled session. */
  if(state.stopped){puts("The runtime was intentionally disabled or switched to diagnostic mode. Active-test monitoring has ended.");break;}
  if(process_exited){puts("Game closed. Keep the CSV with your saved checkpoints.");break;}
  DWORD status=WaitForSingleObject(game,1000);
  /* Drain once more after process exit: its final row may have arrived
     between the previous EOF and the signaled process handle. */
  if(status==WAIT_OBJECT_0){process_exited=1;continue;}
  if(status!=WAIT_TIMEOUT) {
   runtime_monitor_alert(L"The launcher lost its game-process monitor.\n\nStop advancing this test and keep the CSV. Close the game before retrying.");result=17;break;
  }
 }
 CloseHandle(file);return result;
}
