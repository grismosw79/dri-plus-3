/* Buffer diagnostic writes only. Cache/save writes retain their original API.
   Explicit flushes, errors and stage boundaries remain durable. */
#ifndef RM18R22_LOG_IO_H
#define RM18R22_LOG_IO_H
extern int rm18_deep_enabled;
static int rm18_deep(void){return rm18_deep_enabled;}
static SRWLOCK rm18_io_lock=SRWLOCK_INIT;
static unsigned char rm18_log_buffer[65536];
static DWORD rm18_log_used;
static ULONGLONG rm18_last_write;
static BOOL rm18_drain(void){
 if(!rm18_log_used)return TRUE;DWORD written=0;
 if(!WriteFile(log_file,rm18_log_buffer,rm18_log_used,&written,0)||written!=rm18_log_used)return FALSE;
 rm18_log_used=0;rm18_last_write=GetTickCount64();return TRUE;
}
static BOOL rm18_write(HANDLE h,LPCVOID data,DWORD size,LPDWORD written,LPOVERLAPPED overlap){
 if(h!=log_file||overlap)return WriteFile(h,data,size,written,overlap);
 AcquireSRWLockExclusive(&rm18_io_lock);BOOL ok=TRUE;
 if(size>sizeof(rm18_log_buffer)) {ok=rm18_drain()&&WriteFile(h,data,size,written,0);}
 else {
  if(rm18_log_used+size>sizeof(rm18_log_buffer))ok=rm18_drain();
  if(ok){memcpy(rm18_log_buffer+rm18_log_used,data,size);rm18_log_used+=size;if(written)*written=size;
   if(GetTickCount64()-rm18_last_write>=1000|| (size>=6&&!memcmp(data,"FAULT,",6)))ok=rm18_drain();}
 }
 ReleaseSRWLockExclusive(&rm18_io_lock);return ok;
}
static BOOL rm18_flush(HANDLE h){
 if(h!=log_file)return FlushFileBuffers(h);
 AcquireSRWLockExclusive(&rm18_io_lock);BOOL ok=rm18_drain();
 if(ok)ok=FlushFileBuffers(h);ReleaseSRWLockExclusive(&rm18_io_lock);return ok;
}
#define WriteFile rm18_write
#define FlushFileBuffers rm18_flush
#endif
