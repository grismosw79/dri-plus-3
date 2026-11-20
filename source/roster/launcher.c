/* RM17R3 diagnostic launcher. Original accepted RM13R1 ships unchanged alongside. */
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

static const char GAME_HASH[]="8247e6dcc68b78672ac1c79e2deb41900925e6303a71c1e0ace02ef8602b3be2";
static const char GAME_HASH_MOD_MANAGER[]="99ac5da70511c0c8ec5f8677d0e847e6443e1bfa56b22842228edae4723387f8";
static int supported_game_hash(const char *digest) {
    return digest && (!strcmp(digest,GAME_HASH) || !strcmp(digest,GAME_HASH_MOD_MANAGER));
}
static const wchar_t DLL_NAME[]=L"DRIRosterManagementRM17R3.dll";
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
    puts("DRI+ RM17R3 - full CPU roster preparation");
    puts("Dynamic CPU rosters, native NIL payments and 85-90 targets. Use a disposable ENC save.");
    if(argc!=2 || (wcscmp(argv[1],L"pilot") && wcscmp(argv[1],L"disable"))) {
        puts("Usage: DRIRosterManagementRM17R3.exe pilot | disable");return 2;
    }
    int disable=!wcscmp(argv[1],L"disable");
    DWORD pid=find_game();if(!pid){puts("Start the game and activate the accepted Test36R3 runtime first.");return 3;}
    MODULEENTRY32W game,r3,dll,old_rm,capacity;
    if(find_module(pid,L"DRIRosterManagementRM17R2.dll",&old_rm) || find_module(pid,L"DRIRosterManagementRM17R1.dll",&old_rm) || find_module(pid,L"DRIRosterManagementRM17.dll",&old_rm) || find_module(pid,L"DRIRosterManagementRM16R2.dll",&old_rm) || find_module(pid,L"DRIRosterManagementRM16R1.dll",&old_rm) || find_module(pid,L"DRIRosterManagementRM16.dll",&old_rm) || find_module(pid,L"DRIRosterManagementRM15.dll",&old_rm) || find_module(pid,L"DRIRosterManagementRM13.dll",&old_rm) || find_module(pid,L"DRIRosterManagementRM07.dll",&old_rm) || find_module(pid,L"DRIRosterManagementRM08.dll",&old_rm) || find_module(pid,L"DRIRosterManagementRM08R1.dll",&old_rm) || find_module(pid,L"DRIRosterManagementRM09.dll",&old_rm) || find_module(pid,L"DRIRosterManagementRM09R1.dll",&old_rm) || find_module(pid,L"DRIRosterManagementRM10.dll",&old_rm) || find_module(pid,L"DRIRosterManagementRM11.dll",&old_rm) || find_module(pid,L"DRIRosterManagementRM12.dll",&old_rm)) {
        puts("A conflicting RM17 or earlier roster-management DLL is loaded. Close the game and use a fresh process for RM17R3.");return 19;
    }
    if(!find_module(pid,L"CollegeFB27.exe",&game) || !find_module(pid,L"DRIChallengerRetention.dll",&r3)) {
        puts("The accepted R3 DLL must already be loaded. RM17R3 does not launch or replace R3.");return 4;
    }
    if(!find_module(pid,L"DRIRosterManagementRM13R1.dll",&capacity)) {puts("Start the bundled accepted RM13R1 capacity DLL first (or use START-RM17R3).\n");return 4;}
    wchar_t path[32768];DWORD n=GetModuleFileNameW(0,path,32000);
    if(!n || n>=32000)return 5;while(n && path[n-1]!=L'\\')n--;
    if(n+wcslen(DLL_NAME)+1>=32768)return 5;wcscpy(path+n,DLL_NAME);
    char digest[65],game_digest[65];puts("Checking executable, accepted R3 DLL and RM17R3 DLL...");
    if(!hash_file(game.szExePath,digest) || !supported_game_hash(digest)) {
        puts("Unsupported game executable. No patch was installed.");return 6;
    }
    memcpy(game_digest,digest,65);
    if(!hash_file(r3.szExePath,digest) || strcmp(digest,"5b29b69dfefd93162ebae583dd318ddc5276e87d847a812768fd6cb94a2c9144")) {
        puts("The loaded R3 DLL file is not the accepted Test36R3 benchmark. Nothing was patched.");return 7;
    }
    if(!hash_file(capacity.szExePath,digest) || strcmp(digest,"43bca3ef640536107019ccba8cac6e5d12d53c8d2f83a76c38463552b4f8f54f")){puts("RM13R1 must match the accepted capacity build.");return 7;}
    if(!hash_file(path,digest) || strcmp(digest,DRI_DLL_HASH)){puts("RM17R3 DLL/launcher mismatch. Extract the complete RM17R3 kit again.");return 7;}
    int loaded=find_module(pid,DLL_NAME,&dll);
    if(loaded && _wcsicmp(path,dll.szExePath)){puts("Another RM17R3 folder is loaded. Restart the game before switching folders.");return 8;}
    if(disable && !loaded){puts("RM17R3 is not loaded.");return 0;}
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
        if(run!=1 || !find_module(pid,DLL_NAME,&dll)){puts("Windows did not load RM17R3. Close the game before retrying.");goto done;}
    }
    DWORD rva=export_rva(path,disable?"DriDisable":"DriInitialize");if(!rva)goto done;
    if(run_remote(process,(uintptr_t)dll.modBaseAddr+rva,0,&code)!=1)goto done;
    wchar_t receipt[32768];wcscpy(receipt,path);wchar_t *slash=wcsrchr(receipt,L'\\');
    if(slash) {
        SYSTEMTIME t;GetSystemTime(&t);
        swprintf(slash+1,32768-(slash+1-receipt),L"RM17R3_Receipt_%04u%02u%02u_%02u%02u%02u_%lu.txt",t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,(unsigned long)pid);
        FILE *f=_wfopen(receipt,L"wb");if(f){fprintf(f,"build=RM17R3_FULL_CPU_ROSTER_PREP\nmode=%s\nscope=VALIDATED_RM13_CAP90_ENC_BRANCH\ngame_sha256=%s\naccepted_r3_sha256=5b29b69dfefd93162ebae583dd318ddc5276e87d847a812768fd6cb94a2c9144\nrm17r3_sha256=%s\ninitialize_result=%lu\nprocess_id=%lu\nobserved_call_sites=14\naccepted_capacity_dll_unchanged=1\nposition_feature_enabled=0\ndev_feature_enabled=0\ntransfer_feature_enabled=1\npermanent_custom_cuts=0\nnative_cpu_cuts_blocked=ONLY_AFTER_ACCEPTED_PREP\nautomatic_user_fill_blocked=ONLY_AFTER_ACCEPTED_PREP\npreflight_failure=NATIVE_FALLTHROUGH_REJECTED_BRANCH\nunsafe_continuation=STOP_PROCESS_NO_ROLLBACK\ncustom_native_transactions=CONDITIONAL\nnative_flow_validation=PENDING_RETURNED_LOG_AND_SAVE\nacademy_policy=UNSIGNED_HS_ONLY_NO_TRANSFERS\nproposed_actions_applied=PENDING_RUNTIME_LOG\n",disable?"DISABLE":"ACTIVE_PILOT",game_digest,DRI_DLL_HASH,(unsigned long)code,(unsigned long)pid);fclose(f);}
    }
    if(code==13)puts("Code 13: a code/schema guard failed. The RM17R3 JSONL names the exact module and RVA. Do not advance; return the collected logs.");
    if(code==17)puts("Code 17: initialization is already in progress. Do not run another launcher; close the game.");
    if(code==21)puts("Code 21: RM17R3 already initialized in this process. This is not a new activation; restart before a new attempt.");
    if(code){printf("RM17R3 refused activation (code %lu). Keep the receipt/log and restart the game.\n",code);goto done;}
    puts(disable?"RM17R3 capture disabled; native forwarding remains. Restart the game to unload.":"RM17R3 READY. On the bundled ENC copy, advance once to Year 2 preseason. Full-league runtime and saved-state validation pending.");
    if(!disable)puts("Return RM17R3 JSONL, both receipts, and the preseason save. Keep your original ENC save; this revision protects user rosters and tightens destination roles.");
    result=0;
done:
    CloseHandle(process);return result;
}
