/* T3 Steam compatibility TEST monitor. Native gameplay DLL and its guards are unchanged. */
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

#include "game_identity.h"
static const wchar_t DLL_NAME[]=L"DRIPlus3-RM18R22.dll";
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
    puts("DRI+ 3.0 RM18R22 TU1117 T3 - Steam compatibility TEST monitor");
    puts("Launch through Frosty with your current DRI 3.0 mod profile. Activate once at the game's main menu before loading your prepared dynasty.");
    puts("Wait for RM18R22 READY and the monitoring messages. Keep this window open while playing.");
    int disable=argc>1 && !_wcsicmp(argv[1],L"disable");
    DWORD pid=find_game();if(!pid){puts("Open exactly one CollegeFB27 game process first.");return 3;}
    MODULEENTRY32W game,dll;
    if(!find_module(pid,L"CollegeFB27.exe",&game)){puts("Game module unavailable.");return 4;}
    wchar_t path[32768];DWORD n=GetModuleFileNameW(0,path,32000);
    if(!n || n>=32000)return 5;while(n && path[n-1]!=L'\\')n--;
    if(n+wcslen(DLL_NAME)+1>=32768)return 5;wcscpy(path+n,DLL_NAME);
    if(!disable){
        const wchar_t *folders[]={L"HC-Hire-Cache",L"HS-Plan-Cache",L"EPS1-Cache"};
        for(unsigned j=0;j<3;j++){
            wchar_t cache[32768];wcscpy(cache,path);wchar_t *end=wcsrchr(cache,L'\\');if(!end)return 5;
            wcscpy(end+1,folders[j]);DWORD attr=GetFileAttributesW(cache);
            if(attr==INVALID_FILE_ATTRIBUTES||!(attr&FILE_ATTRIBUTE_DIRECTORY)){puts("Required runtime cache folders are missing. Re-extract the complete release and relaunch through Frosty.");return 5;}
        }
    }
    char digest[65],game_digest[65];puts("Checking supported executable and combined RM18R22 DLL...");
    if(!hash_file(game.szExePath,digest)) {
        puts("Could not read or hash the game executable. No patch was installed.");return 6;
    }
    printf("Game SHA-256: %s\n",digest);fflush(stdout);
    if(!supported_game_hash(digest)) {
        puts("Unsupported game executable. No patch was installed.");return 6;
    }
    if(!strcmp(digest,GAME_HASH_STEAM_MMC)) {
        puts("Steam/MMC test fingerprint accepted. Native compatibility checks remain required; live qualification is pending.");
    }
    memcpy(game_digest,digest,65);
    if(!hash_file(path,digest) || strcmp(digest,DRI_DLL_HASH)){puts("RM18R22 DLL/launcher mismatch. Re-extract the complete R22 release and restart Frosty.");return 7;}
    int loaded=find_module(pid,DLL_NAME,&dll);
    if(loaded && _wcsicmp(path,dll.szExePath)){puts("Another RM18R22 folder is loaded. Restart the game before switching folders.");return 8;}
    if(disable && !loaded){puts("RM18R22 is not loaded.");return 0;}
    HANDLE process=OpenProcess(SYNCHRONIZE|PROCESS_CREATE_THREAD|PROCESS_QUERY_INFORMATION|PROCESS_VM_OPERATION|PROCESS_VM_READ|PROCESS_VM_WRITE,FALSE,pid);
    if(!process){printf("Cannot open game process (Windows error %lu). No bypass is attempted.\n",GetLastError());return 9;}
    int result=10;DWORD code=0;
    if(!loaded) {
        FARPROC local=GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"LoadLibraryW");
        MEMORY_BASIC_INFORMATION mbi;wchar_t hostpath[32768];MODULEENTRY32W host;
        if(!local || !VirtualQuery((void*)local,&mbi,sizeof(mbi)) || !GetModuleFileNameW(mbi.AllocationBase,hostpath,32768))goto done;
        wchar_t *base=wcsrchr(hostpath,L'\\');base=base?base+1:hostpath;
        if(!find_module(pid,base,&host))goto done;
        uintptr_t remote_load=(uintptr_t)host.modBaseAddr+((uintptr_t)local-(uintptr_t)mbi.AllocationBase);
        SIZE_T bytes=(wcslen(path)+1)*sizeof(wchar_t),written=0;
        void *remote_path=VirtualAllocEx(process,0,bytes,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
        if(!remote_path)goto done;
        if(!WriteProcessMemory(process,remote_path,path,bytes,&written) || written!=bytes){VirtualFreeEx(process,remote_path,0,MEM_RELEASE);goto done;}
        int run=run_remote(process,remote_load,remote_path,&code);
        if(run!=-1)VirtualFreeEx(process,remote_path,0,MEM_RELEASE);
        if(run!=1 || !find_module(pid,DLL_NAME,&dll)){puts("Windows did not load RM18R22. Close the game before retrying.");goto done;}
    }
    DWORD rva=export_rva(path,disable?"DriDisable":"DriInitialize");if(!rva)goto done;
    if(run_remote(process,(uintptr_t)dll.modBaseAddr+rva,0,&code)!=1)goto done;
    wchar_t receipt[32768];wcscpy(receipt,path);wchar_t *slash=wcsrchr(receipt,L'\\');
    if(slash) {
        SYSTEMTIME t;GetSystemTime(&t);
        swprintf(slash+1,32768-(slash+1-receipt),L"RM18R22_Receipt_%04u%02u%02u_%02u%02u%02u_%lu.txt",t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,(unsigned long)pid);
        FILE *f=_wfopen(receipt,L"wb");if(f){fprintf(f,"build=RM18R22_RELEASE\nmonitor_build=R22_TU1117_T3_STEAM_TEST\nmode=%s\ngame_sha256=%s\ncombined_dll_sha256=%s\ninitialize_result=%lu\nprocess_id=%lu\nsource_baseline=RM18R21\nemergency_HC_promotions=CPLUS_THEN_C_THEN_CMINUS_NATIVE_EMPTY_TIERS_ONLY\nnormal_HC_promotion_floor=CPLUS\nrefresh_fix=PP28_AND_CURRENT_OFFSEASON_EVALUATORS\nposition_feature_enabled=1\nannual_development_enabled=1\nroster_feature_enabled=1\nfull_board_reservation=0\nenrollment_ceiling=90\nroster_limits=STORED_AND_ACTIVE_ENROLLMENT_LIMITS_VERIFIED\npermanent_custom_cuts=CONDITIONAL_ABOVE90_AFTER_QUALIFIED_HOMES\ninvalid_user_roster=CPU_RULES_THIS_ENC_PASS\ninvalid_user_triggers=BELOW85_ABOVE90_COMPATIBLE_MINIMUM_GAP\nvalid_user_roster=UNTOUCHED\nlive_requalification=REFRESH_UNCOMMITTED_PLAN\ncut_lifecycle=VERIFIED_RETIREMENT_THEN_NATIVE_DISPOSAL_ALLOWED\nfinal_audit=OWNERSHIP_FOOTBALL_NIL_CENSUS_AND_CALLBACK_ORDER\nrole_selection=NATIVE_PROJECTED_ROLES_REQUIRE_REVIEW\ncombined_live_status=CAROUSEL_AND_YEAR3_PRE_PASSED\npackaged_launch_live_check=SEPARATE_FROM_GAMEPLAY_TEST\n",disable?"DISABLE":"ACTIVE",game_digest,DRI_DLL_HASH,(unsigned long)code,(unsigned long)pid);fclose(f);}
    }
    if(code==19)puts("An older DRI DLL is already loaded. Close the game; disable the older DRI plugin or manual launcher, then relaunch through Frosty.");
    if(code==21)puts("RM18R22 is already initialized. This launcher is needed only once per game process.");
    if(code){printf("RM18R22 refused activation (code %lu). Do not advance. Keep logs/receipt and restart the game.\n",code);goto done;}
    puts(disable?"RM18R22 disabled. Restart the game before continuing.":"RM18R22 READY: combined recruiting, carousel, positions/development, enrollment90 and roster prep enabled.");
    if(!disable)puts("For support, close the game and run COLLECT-DRI-DIAGNOSTICS.cmd from the release folder. Send its ZIP and your last good dynasty save.");
    result=disable?0:monitor_runtime(process,path,pid);
done:
    CloseHandle(process);return result;
}
