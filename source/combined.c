#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <wchar.h>
int rm18_deep_enabled;
static HINSTANCE self_module;
static volatile LONG initialized;
static DWORD init_result=17;
#define DECLARE(n) DWORD WINAPI Rm18##n##Initialize(void*); DWORD WINAPI Rm18##n##Disable(void*); BOOL WINAPI Rm18##n##Attach(HINSTANCE,DWORD,LPVOID);
DECLARE(R3) DECLARE(Positions) DECLARE(Capacity) DECLARE(Roster) DECLARE(Reserve)
int Rm18PositionPreflight(void),Rm18RosterPreflight(void),Rm18CapacityPreflight(void),Rm18ReservePreflight(void);
static int old_loaded(void){
 const wchar_t *names[]={L"DRIPlus3-RM18R21.dll",L"DRIPlus3-RM18R20.dll",L"DRIPlus3-RM18R19.dll",L"DRIPlus3-RM18R18.dll",L"DRIPlus3-RM18R17.dll",L"DRIPlus3-RM18R16.dll",L"DRIPlus3-RM18R15.dll",L"DRIPlus3-RM18R14.dll",L"DRIPlus3-RM18R13.dll",L"DRIPlus3-RM18R12.dll",L"DRIPlus3-RM18R11.dll",L"DRIPlus3-RM18R10.dll",L"DRIPlus3-RM18R9.dll",L"DRIPlus3-RM18R8.dll",L"DRIPlus3-RM18R7.dll",L"DRIPlus3-RM18R6.dll",L"DRIPlus3-RM18R5.dll",L"DRIPlus3-RM18.dll",L"DRIPlus3-RM18R1.dll",L"DRIPlus3-RM18R2.dll",L"DRIPlus3-RM18R4.dll",L"DRIPlus3-RM18R3.dll",L"DRIChallengerRetention.dll",L"DRIRosterManagementRM07.dll",L"DRIRosterManagementRM08.dll",L"DRIRosterManagementRM08R1.dll",L"DRIRosterManagementRM09.dll",L"DRIRosterManagementRM09R1.dll",L"DRIRosterManagementRM10.dll",L"DRIRosterManagementRM11.dll",L"DRIRosterManagementRM12.dll",L"DRIRosterManagementRM13.dll",L"DRIRosterManagementRM13R1.dll",L"DRIRosterManagementRM15.dll",L"DRIRosterManagementRM16.dll",L"DRIRosterManagementRM16R1.dll",L"DRIRosterManagementRM16R2.dll",L"DRIRosterManagementRM17.dll",L"DRIRosterManagementRM17R1.dll",L"DRIRosterManagementRM17R2.dll",L"DRIRosterManagementRM17R3.dll"};
 for(unsigned i=0;i<sizeof(names)/sizeof(names[0]);i++)if(GetModuleHandleW(names[i]))return 1;return 0;
}
static void receipt(DWORD code){
 wchar_t path[32768];DWORD n=GetModuleFileNameW(self_module,path,32000);if(!n||n>=32000)return;while(n&&path[n-1]!=L'\\')n--;
 swprintf(path+n,32768-n,L"RM18R22_Combined_Status_%lu.txt",(unsigned long)GetCurrentProcessId());FILE *f=_wfopen(path,L"wb");
 if(f){fprintf(f,"build=RM18R22_EXTENDED_TEST\ninitialize_result=%lu\ndeep_logging=%d\ncombined_live_validation=PENDING\nannual_development_live_validation=PENDING\nmeasured_sim_speed=PENDING\n",(unsigned long)code,rm18_deep_enabled);fclose(f);}
}
__declspec(dllexport) DWORD WINAPI DriInitialize(void *unused){
 (void)unused;if(InterlockedCompareExchange(&initialized,1,0))return init_result?init_result:21;
 DWORD code=0;
 wchar_t path[32768];DWORD n=GetModuleFileNameW(self_module,path,32000);
 if(n&&n<32000){while(n&&path[n-1]!=L'\\')n--;wcscpy(path+n,L"RM18R22-DEEP-LOGGING.txt");DWORD attr=GetFileAttributesW(path);rm18_deep_enabled=attr!=INVALID_FILE_ATTRIBUTES&&!(attr&FILE_ATTRIBUTE_DIRECTORY);}
 if(old_loaded()){code=19;goto done;}
 /* All auxiliary native guards are checked before any modification. */
 if(!Rm18PositionPreflight()){code=113;goto done;}
 if(!Rm18CapacityPreflight()){code=213;goto done;}
 if(!Rm18RosterPreflight()){code=313;goto done;}
 if(!Rm18ReservePreflight()){code=413;goto done;}
 code=Rm18R3Initialize((void*)(uintptr_t)1);if(code){code+=1000;goto done;}
 code=Rm18ReserveInitialize(0);if(code){code+=2000;goto fail;}
 code=Rm18CapacityInitialize(0);if(code){code+=3000;goto fail;}
 code=Rm18PositionsInitialize(0);if(code){code+=4000;goto fail;}
 code=Rm18RosterInitialize(0);if(code){code+=5000;goto fail;}
 goto done;
fail:
 Rm18R3Disable(0);Rm18PositionsDisable(0);Rm18RosterDisable(0);
 /* No unloading or blind patch rollback; launcher requires a fresh process. */
done:
 init_result=code;receipt(code);return code;
}
__declspec(dllexport) DWORD WINAPI DriDisable(void *unused){
 (void)unused;Rm18RosterDisable(0);Rm18PositionsDisable(0);Rm18R3Disable(0);
 /* Keep capacity/reserve until restart: existing 86-90 rosters require them. */
 return 0;
}
BOOL WINAPI DllMain(HINSTANCE h,DWORD reason,LPVOID r){
 if(reason==DLL_PROCESS_ATTACH){self_module=h;DisableThreadLibraryCalls(h);}
 Rm18R3Attach(h,reason,r);Rm18PositionsAttach(h,reason,r);Rm18RosterAttach(h,reason,r);return TRUE;
}

__declspec(dllexport) const struct {char tag[24];uintptr_t p[15];} rm18_init_diag={"RM18R22-ROOT-INIT",{(uintptr_t)&Rm18PositionPreflight,(uintptr_t)&Rm18RosterPreflight,(uintptr_t)&Rm18CapacityPreflight,(uintptr_t)&Rm18ReservePreflight,(uintptr_t)&Rm18R3Initialize,(uintptr_t)&Rm18ReserveInitialize,(uintptr_t)&Rm18CapacityInitialize,(uintptr_t)&Rm18PositionsInitialize,(uintptr_t)&Rm18RosterInitialize,(uintptr_t)&Rm18R3Disable,(uintptr_t)&Rm18PositionsDisable,(uintptr_t)&Rm18RosterDisable,(uintptr_t)&initialized,(uintptr_t)&init_result,(uintptr_t)&receipt}};
