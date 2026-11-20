#ifndef DRI_FAILURE_DIALOG_H
#define DRI_FAILURE_DIALOG_H
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

/* Format a support code; keep the full original row in the diagnostics. */
static int dri_code_char(unsigned char c) {
 return (c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'||c=='-';
}
static void dri_fault_code(const char *row,char out[192]) {
 strcpy(out,"RUNTIME_FAULT");
 if(!row || strncmp(row,"FAULT,",6))return;
 const char *part=row+6,*end=strchr(part,',');
 size_t size=end?(size_t)(end-part):strlen(part);
 if(!size || size>120)return;
 for(size_t i=0;i<size;i++)if(!dri_code_char((unsigned char)part[i]))return;
 memcpy(out,part,size);out[size]=0;
 if(!end)return;
 const char *exception=end+1;
 size_t length=strcspn(exception,",\r\n");
 if(length!=10 || exception[0]!='0' || (exception[1]!='x'&&exception[1]!='X'))return;
 for(size_t i=2;i<10;i++) {
  char c=exception[i];
  if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F')))return;
 }
 memcpy(out+size," / ",3);memcpy(out+size+3,exception,10);out[size+13]=0;
}

static void dri_failure_message(const wchar_t *dll_path,unsigned long pid,
                                const char *code,wchar_t *out,size_t capacity) {
 (void)dll_path;(void)pid;
 if(!capacity)return;
 int result=swprintf(out,capacity,
  L"Error code: %hs\n\n"
  L"Stop advancing and close the game.\n"
  L"Send the diagnostics ZIP and your last good dynasty save.\n\n"
  L"Run COLLECT-DRI-DIAGNOSTICS.cmd in the release folder.\n"
  L"Include what you did just before the error.",code);
 if(result<0 || (size_t)result>=capacity)
  swprintf(out,capacity,L"Error code: %hs\nSend DRI logs, receipt TXT and last good save.",code);
 out[capacity-1]=0;
}
#endif
