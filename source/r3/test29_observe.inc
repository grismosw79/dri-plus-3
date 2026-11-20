/* Test29 observational timing. GetTickCount64 is monotonic on the supported
   Windows host. Emulated callbacks explicitly do not claim measured time. */
static uint64_t test29_clock_ms(void) {
#ifdef DRI_EMULATION_TEST
 return 0;
#else
 return GetTickCount64();
#endif
}
static void test29_phase_log(const char *phase,unsigned tid,U eval,uint64_t start) {
 if(log_file==INVALID_HANDLE_VALUE || !eval)return;
 Rec f=context_ref(eval,6),season=f?reference(f,44):0;
 if(season) {
  uint64_t stop=test29_clock_ms();
  log_line("TEST29_PHASE_MS,%s,%u,%u,%u,%u,%llu,%s",phase,tid,
   scalar(season,2,0x100),scalar(season,5,0x108),scalar(season,4,0x100),
   (unsigned long long)(stop>=start?stop-start:0),
#ifdef DRI_EMULATION_TEST
   "TEST_NO_CLOCK"
#else
   "MEASURED"
#endif
  );
 }
 release(season);release(f);
}
