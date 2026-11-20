"""Production checkpoint file operations with fault-injected Win32 services.
Checks ordering, complete reads, atomic replacement, and handles; not live Windows.
"""
from pathlib import Path
import os,subprocess,json
S=Path(__file__).resolve().parents[1];O=Path(os.environ['DRI_TEST_OUTPUT_DIR']);O.mkdir(parents=True,exist_ok=True)
src=(S/'hs34r3_continuity.inc').read_text();body=src[src.index('static int hs34r3_path('):src.index('static int hs34r3_restore(')]
c=r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include "hs34r3_continuity.h"
typedef unsigned long DWORD;typedef int HANDLE;typedef struct{long long QuadPart;}LARGE_INTEGER;
#define INVALID_HANDLE_VALUE -1
#define ERROR_ALREADY_EXISTS 183
#define GENERIC_READ 1
#define GENERIC_WRITE 2
#define FILE_SHARE_READ 1
#define OPEN_EXISTING 1
#define CREATE_ALWAYS 2
#define FILE_ATTRIBUTE_NORMAL 0
#define MOVEFILE_REPLACE_EXISTING 1
#define MOVEFILE_WRITE_THROUGH 2
static int self_module,fail,handles,flush,renamed,deleted;static size_t filesize;static unsigned char final[sizeof(Hs34R3Disk)],temp[sizeof(Hs34R3Disk)];
static DWORD GetModuleFileNameW(int m,wchar_t *p,DWORD n){(void)m;assert(n>=32);if(fail==1)return 0;wcscpy(p,L"C:\\DRI\\Portal\\retention.dll");return wcslen(p);}
static int CreateDirectoryW(const wchar_t *p,void *a){(void)a;assert(!wcscmp(p,L"C:\\DRI\\Portal\\HS-Plan-Cache"));return fail!=2;}
static DWORD GetLastError(void){return 5;}
static DWORD GetCurrentProcessId(void){return 4242;}
static HANDLE CreateFileW(const wchar_t *p,int a,int share,void *sa,int kind,int flags,void *other){(void)sa;(void)flags;(void)other;
 assert(wcsstr(p,L"HS-Plan-Cache\\0000007b-2026-55-00000000000004d2.bin"));
 if(fail==3)return INVALID_HANDLE_VALUE;
 if(a==GENERIC_WRITE){assert(!share&&kind==CREATE_ALWAYS&&wcsstr(p,L".4242.tmp"));memset(temp,0,sizeof(temp));}
 else assert(share==FILE_SHARE_READ&&kind==OPEN_EXISTING);
 handles++;return a;}
static int GetFileSizeEx(HANDLE h,LARGE_INTEGER *s){assert(h==GENERIC_READ);s->QuadPart=filesize;return fail!=4;}
static int ReadFile(HANDLE h,void *p,DWORD n,DWORD *got,void *o){(void)o;assert(h==GENERIC_READ&&n==sizeof(final));memcpy(p,final,n);*got=fail==6?n-1:n;return fail!=5;}
static int WriteFile(HANDLE h,const void *p,DWORD n,DWORD *put,void *o){(void)o;assert(h==GENERIC_WRITE&&n==sizeof(temp));memcpy(temp,p,n);*put=fail==8?n-1:n;return fail!=7;}
static int CloseHandle(HANDLE h){assert(h==1||h==2);handles--;return 1;}
static int FlushFileBuffers(HANDLE h){assert(h==GENERIC_WRITE);flush++;return fail!=9;}
static int MoveFileExW(const wchar_t *a,const wchar_t *b,int flags){assert(!handles&&flush==1&&flags==3&&wcsstr(a,L".tmp")&&!wcsstr(b,L".tmp"));if(fail==10)return 0;memcpy(final,temp,sizeof(final));filesize=sizeof(final);renamed++;return 1;}
static int DeleteFileW(const wchar_t *p){assert(!handles&&wcsstr(p,L".tmp"));deleted++;return 1;}
'''+body+r'''
int main(void){
 Hs34R3Disk d={0},out;d.league=123;d.year=2026;d.tid=55;d.board_hash=1234;
 assert(hs34r3_write(&d)&&renamed==1&&!handles&&flush==1&&!deleted);
 assert(hs34r3_read(&out,&d)&&!memcmp(&out,&d,sizeof(d))&&!handles);
 for(int f=1;f<=10;f++){
  fail=f;flush=renamed=deleted=0;memset(final,0x42,sizeof(final));
  if(f<=3||f>=7){assert(!hs34r3_write(&d)&&!handles&&!renamed);for(size_t i=0;i<sizeof(final);i++)assert(final[i]==0x42);if(f>=7)assert(deleted==1);}
  if(f<=6)assert(!hs34r3_read(&out,&d)&&!handles);
 }
 fail=0;filesize=sizeof(d)-1;assert(!hs34r3_read(&out,&d)&&!handles);
 filesize=sizeof(d)+1;assert(!hs34r3_read(&out,&d)&&!handles);
 return 0;
}'''
(O/'cache_io.c').write_text(c)
subprocess.run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-I'+str(S),str(O/'cache_io.c'),'-o',str(O/'cache_io')],check=True)
subprocess.run([str(O/'cache_io')],check=True)
r={'passed':True,'production_file_functions_executed':True,'win32_services_modeled':True,'checks':['Exact board-key path','Only a complete flushed temp file replaces the checkpoint','Ten injected API/partial-transfer failures do not replace prior checkpoint','Short and oversized files rejected','All opened handles closed'],'live_game_verified':False}
(O/'test34_r3_cache_io_results.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r))
