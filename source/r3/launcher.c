#ifndef UNICODE
#define UNICODE 1
#endif
#ifndef _UNICODE
#define _UNICODE 1
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <bcrypt.h>
#include <stdint.h>
#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include "dll_hash.h"
#include "launcher_monitor.inc"

static const char GAME_HASH[]="8247e6dcc68b78672ac1c79e2deb41900925e6303a71c1e0ace02ef8602b3be2";
static const char GAME_HASH_MOD_MANAGER[]="99ac5da70511c0c8ec5f8677d0e847e6443e1bfa56b22842228edae4723387f8";
static int supported_game_hash(const char *digest) {
    return digest && (!strcmp(digest,GAME_HASH) || !strcmp(digest,GAME_HASH_MOD_MANAGER));
}
static const wchar_t DLL_NAME[]=L"DRIChallengerRetention.dll";
static int hash_file(const wchar_t *path,char out[65]) {
    HANDLE f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
    if(f==INVALID_HANDLE_VALUE) return 0;
    BCRYPT_ALG_HANDLE alg=0;BCRYPT_HASH_HANDLE hash=0;unsigned char digest[32];DWORD n;int ok=0;
    unsigned char *buffer=HeapAlloc(GetProcessHeap(),0,1024*1024);
    if(!buffer) goto done;
    if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,0,0)<0) goto done;
    if(BCryptCreateHash(alg,&hash,0,0,0,0,0)<0) goto done;
    for(;;) { if(!ReadFile(f,buffer,1024*1024,&n,0)) goto done;if(!n) break;
        if(BCryptHashData(hash,buffer,n,0)<0) goto done; }
    if(BCryptFinishHash(hash,digest,32,0)<0) goto done;
    for(int j=0;j<32;j++) sprintf(out+j*2,"%02x",digest[j]);out[64]=0;ok=1;
done:
    if(hash) BCryptDestroyHash(hash);if(alg) BCryptCloseAlgorithmProvider(alg,0);
    if(buffer) HeapFree(GetProcessHeap(),0,buffer);CloseHandle(f);return ok;
}
static void car35_receipt(const wchar_t *dll_path,DWORD pid,const char *game_hash) {
    wchar_t receipt[32768];size_t n=wcslen(dll_path);while(n && dll_path[n-1]!=L'\\')n--;
    if(n+128>=32768)return;memcpy(receipt,dll_path,n*sizeof(wchar_t));
    SYSTEMTIME t;GetSystemTime(&t);
    swprintf(receipt+n,32768-n,L"DRI_Test36R3_Receipt_%04u%02u%02u_%02u%02u%02u_%lu.txt",t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,(unsigned long)pid);
    FILE *f=_wfopen(receipt,L"wb");if(!f)return;
    fprintf(f,"build=Test36R3_Portal_2026_11_06\nlauncher_revision=Test36R3_Portal\npaired_FTC=FTC-Test36\naccepted_baseline=Test34R4_FTC-Master-R03_release-control-v60\ngame_sha256=%s\ndll_sha256=%s\n",game_hash,DRI_DLL_HASH);
    fclose(f);
}
static int find_module(DWORD pid,const wchar_t *name,MODULEENTRY32W *out) {
    HANDLE s=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid);
    if(s==INVALID_HANDLE_VALUE) return 0;
    MODULEENTRY32W m={0};m.dwSize=sizeof(m);int found=0;
    if(Module32FirstW(s,&m)) do {if(!_wcsicmp(m.szModule,name)){*out=m;found=1;break;}}while(Module32NextW(s,&m));
    CloseHandle(s);return found;
}
static DWORD find_game(void) {
    HANDLE s=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);if(s==INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W p={0};p.dwSize=sizeof(p);DWORD pid=0;
    if(Process32FirstW(s,&p)) do {if(!_wcsicmp(p.szExeFile,L"CollegeFB27.exe")) {
        if(pid) {puts("More than one CollegeFB27 process is open. Close the extra instance.");pid=0;break;}pid=p.th32ProcessID;
    }}while(Process32NextW(s,&p));
    CloseHandle(s);return pid;
}
static unsigned char *at_rva(unsigned char *b,size_t size,DWORD rva,size_t bytes) {
    if(size<sizeof(IMAGE_DOS_HEADER)) return 0;
    IMAGE_DOS_HEADER *d=(void *)b;
    if(d->e_magic!=IMAGE_DOS_SIGNATURE || d->e_lfanew<0 || (size_t)d->e_lfanew+sizeof(IMAGE_NT_HEADERS64)>size) return 0;
    IMAGE_NT_HEADERS64 *n=(void *)(b+d->e_lfanew);
    if(n->Signature!=IMAGE_NT_SIGNATURE || n->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC) return 0;
    if(rva<n->OptionalHeader.SizeOfHeaders && rva<=size && bytes<=size-rva) return b+rva;
    IMAGE_SECTION_HEADER *s=IMAGE_FIRST_SECTION(n);
    if((unsigned char *)(s+n->FileHeader.NumberOfSections)>b+size) return 0;
    for(int j=0;j<n->FileHeader.NumberOfSections;j++) if(rva>=s[j].VirtualAddress) {
        uint64_t delta=(uint64_t)rva-s[j].VirtualAddress;
        uint64_t offset=(uint64_t)s[j].PointerToRawData+delta;
        if(delta<=s[j].SizeOfRawData && bytes<=s[j].SizeOfRawData-delta && offset<=size && bytes<=size-offset) return b+offset;
    }
    return 0;
}
static DWORD export_rva(const wchar_t *path,const char *wanted) {
    HANDLE f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
    if(f==INVALID_HANDLE_VALUE) return 0;
    DWORD size=GetFileSize(f,0), got=0,result=0;
    if(size==INVALID_FILE_SIZE || size<512 || size>16*1024*1024) {CloseHandle(f);return 0;}
    unsigned char *b=HeapAlloc(GetProcessHeap(),0,size);
    if(!b || !ReadFile(f,b,size,&got,0) || got!=size) goto done;
    IMAGE_DOS_HEADER *d=(void *)b;
    if(d->e_magic!=IMAGE_DOS_SIGNATURE || d->e_lfanew<0 || (size_t)d->e_lfanew+sizeof(IMAGE_NT_HEADERS64)>size) goto done;
    IMAGE_NT_HEADERS64 *n=(void *)(b+d->e_lfanew);
    IMAGE_DATA_DIRECTORY dir=n->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    IMAGE_EXPORT_DIRECTORY *e=(void *)at_rva(b,size,dir.VirtualAddress,sizeof(*e));if(!e) goto done;
    if(e->NumberOfNames>1000 || e->NumberOfFunctions>1000) goto done;
    DWORD *names=(void *)at_rva(b,size,e->AddressOfNames,e->NumberOfNames*4);
    WORD *ord=(void *)at_rva(b,size,e->AddressOfNameOrdinals,e->NumberOfNames*2);
    DWORD *functions=(void *)at_rva(b,size,e->AddressOfFunctions,e->NumberOfFunctions*4);
    if(!names || !ord || !functions) goto done;
    for(DWORD j=0;j<e->NumberOfNames;j++) {
        char *name=(void *)at_rva(b,size,names[j],strlen(wanted)+1);
        if(name && !strcmp(name,wanted) && ord[j]<e->NumberOfFunctions) {
            DWORD r=functions[ord[j]];
            if(r<dir.VirtualAddress || r>=dir.VirtualAddress+dir.Size) result=r;
        }
    }
done:
    if(b) HeapFree(GetProcessHeap(),0,b);CloseHandle(f);return result;
}
static int run_remote(HANDLE process,uintptr_t address,void *arg,DWORD *code) {
    HANDLE thread=CreateRemoteThread(process,0,0,(LPTHREAD_START_ROUTINE)address,arg,0,0);
    if(!thread) {printf("Windows refused the runtime operation (error %lu). No bypass is attempted.\n",GetLastError());return 0;}
    DWORD wait=WaitForSingleObject(thread,30000);
    if(wait!=WAIT_OBJECT_0) {
        puts("The operation did not finish within 30 seconds. Do not start another copy. Close and restart the game before retrying.");
        CloseHandle(thread);return -1;
    }
    int ok=GetExitCodeThread(thread,code);CloseHandle(thread);return ok?1:0;
}
int wmain(int argc,wchar_t **argv) {
    puts("DRI+ Test36R3 Launcher - End-of-season NIL / Players Leaving / recruiting");
    puts("Experimental build. Use the disposable offline Dynasty and keep its starting save.");
    if(argc!=2 || (wcscmp(argv[1],L"test") && wcscmp(argv[1],L"diagnostic") && wcscmp(argv[1],L"disable"))) {
        puts("Usage: DRIRetentionTest.exe diagnostic | test | disable");return 2;
    }
    int disable=!wcscmp(argv[1],L"disable"), active=!wcscmp(argv[1],L"test");
    DWORD pid=find_game();if(!pid) {puts("Open College Football 27 to the main menu. Activate Test36R3 before loading a separate Test36R3 dynasty branch.");return 3;}
    MODULEENTRY32W game,dll;
    if(!find_module(pid,L"CollegeFB27.exe",&game)) {puts("Cannot read the game's module list. Run the game and launcher with matching Windows permissions.");return 4;}
    wchar_t path[32768];DWORD n=GetModuleFileNameW(0,path,32000);
    if(!n || n>=32000) return 5;
    while(n && path[n-1]!=L'\\') n--;
    if(n+wcslen(DLL_NAME)+1>=32768) return 5;
    wcscpy(path+n,DLL_NAME);
    char digest[65],game_digest[65];puts("Checking the game build and test DLL...");
    if(!hash_file(game.szExePath,digest) || !supported_game_hash(digest)) {
        puts("This game executable does not match either inspected October 29 executable captured October 31. Nothing was patched.");return 6;
    }
    memcpy(game_digest,digest,65);
    if(!hash_file(path,digest) || strcmp(digest,DRI_DLL_HASH)) {puts("The test DLL is missing or does not match this launcher. Extract the complete ZIP again.");return 7;}
    car35_receipt(path,pid,game_digest);
    int loaded=find_module(pid,DLL_NAME,&dll);
    if(loaded && _wcsicmp(path,dll.szExePath)) {puts("A retention DLL from a different folder is already loaded. Restart the game before using this package.");return 8;}
    if(disable && !loaded) {puts("No retention DLL is loaded in this game session.");return 0;}
    HANDLE process=OpenProcess(SYNCHRONIZE|PROCESS_CREATE_THREAD|PROCESS_QUERY_INFORMATION|PROCESS_VM_OPERATION|PROCESS_VM_READ|PROCESS_VM_WRITE,FALSE,pid);
    if(!process) {printf("Cannot open the test process (Windows error %lu). No changes made.\n",GetLastError());return 9;}
    int result=10;DWORD code=0;
    if(!loaded) {
        FARPROC local=GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"LoadLibraryW");
        MEMORY_BASIC_INFORMATION mbi;wchar_t hostpath[32768];MODULEENTRY32W host;
        if(!local || !VirtualQuery((void *)local,&mbi,sizeof(mbi)) || !GetModuleFileNameW(mbi.AllocationBase,hostpath,32768)) goto done;
        wchar_t *base=wcsrchr(hostpath,L'\\');base=base?base+1:hostpath;
        if(!find_module(pid,base,&host)) goto done;
        uintptr_t remote_load=(uintptr_t)host.modBaseAddr+((uintptr_t)local-(uintptr_t)mbi.AllocationBase);
        SIZE_T bytes=(wcslen(path)+1)*sizeof(wchar_t),written=0;
        void *remote_path=VirtualAllocEx(process,0,bytes,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
        if(!remote_path) goto done;
        if(!WriteProcessMemory(process,remote_path,path,bytes,&written) || written!=bytes) {VirtualFreeEx(process,remote_path,0,MEM_RELEASE);goto done;}
        int run=run_remote(process,remote_load,remote_path,&code);
        /* Never free an argument that a timed-out loader thread could still use. */
        if(run!=-1) VirtualFreeEx(process,remote_path,0,MEM_RELEASE);
        if(run!=1 || !find_module(pid,DLL_NAME,&dll)) {puts("Windows did not load the test DLL. Restart the game before retrying.");goto done;}
    }
    DWORD rva=export_rva(path,disable?"DriDisable":"DriInitialize");if(!rva) goto done;
    if(run_remote(process,(uintptr_t)dll.modBaseAddr+rva,(void *)(uintptr_t)active,&code)!=1) goto done;
    if(code!=0) {
        printf("The runtime refused to activate (code %lu). Check the CSV in this folder.\n",code);
        puts("Code 13: build/signature mismatch. Code 15: installation blocked or a thread was at a hook site.");
        puts("Restart the game before another attempt.");goto done;
    }
    if(disable) puts("Behavior disabled. Hooks pass through; close the game to unload the DLL completely.");
    else if(active) puts("ACTIVE TEST READY. Follow the Test36R3 checkpoints in READ-ME-FIRST.txt. Keep separate saves and return them with this CSV.");
    else puts("DIAGNOSTIC READY. Runtime behavior is observational. Advance the agreed checkpoint and return the CSV from this folder.");
    puts("Restarting the game ends this session's runtime. Launch it again only when you want another test.");result=0;
    if(active)result=monitor_runtime(process,path,pid);
done:
    CloseHandle(process);if(result) puts("Keep the message and CSV. Close the game before retrying.");return result;
}
