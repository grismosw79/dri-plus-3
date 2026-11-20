/* Portal-cycle state. Only numeric identities are persisted. The live index is rebuilt
   from the active dynasty's actual transfer pool before it can be used. */
#define RC4_ROWS 32768
#define RC4_PAIRS 552
#define RC4_MAGIC UINT32_C(0x34425251)
typedef struct { unsigned index,player,pid,origin; unsigned char pos,stars,mot,observed; } Rc4Member;
typedef struct { unsigned short team,slot,week,primary,backup; } Rc4Pair;
typedef struct {
    unsigned magic,league,year,count,proof,pairs;
    uint64_t signature;
    unsigned char nil[(RC4_ROWS+7)/8];
    Rc4Pair pair[RC4_PAIRS];
    uint64_t checksum;
} Rc4Disk;
static struct {
    U franchise,franchise_table,rec_table,player_table,pool;
    Rec held_franchise;
    unsigned year,week,count; int window;
    Rc4Member member[RC4_ROWS];
    Rc4Disk disk;
} rc4;
static __declspec(thread) Rec rc4_evaluating;
static struct { U f; unsigned year; const unsigned char *reason; } rc4_publishing;
static uint64_t rc4_hash(uint64_t h,const void *p,size_t n) {
    const unsigned char *b=p;
    while(n--) h=(h ^ *b++)*UINT64_C(1099511628211);
    return h;
}
static int rc4_path(wchar_t *path,size_t cap) {
    DWORD n=GetModuleFileNameW(self_module,path,(DWORD)cap-130);
    if(!n || n>=cap-130)return 0;
    while(n && path[n-1]!=L'\\')n--;
    wcscpy(path+n,L"Portal-Cache");
    if(!CreateDirectoryW(path,0) && GetLastError()!=ERROR_ALREADY_EXISTS)return 0;
    return swprintf(path+n+12,cap-n-12,L"\\%08x-%u-%016llx.bin",rc4.disk.league,rc4.disk.year,
                    (unsigned long long)rc4.disk.signature)>0;
}
static int rc4_file(int write) {
    wchar_t path[1024],temp[1100];DWORD bytes=0;Rc4Disk *d=&rc4.disk;
    if(!rc4_path(path,1024))return 0;
    if(write){
        swprintf(temp,1100,L"%ls.%lu.tmp",path,(unsigned long)GetCurrentProcessId());
        HANDLE h=CreateFileW(temp,GENERIC_WRITE,0,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
        if(h==INVALID_HANDLE_VALUE)return 0;
        d->checksum=rc4_hash(UINT64_C(1469598103934665603),d,offsetof(Rc4Disk,checksum));
        int ok=WriteFile(h,d,sizeof(*d),&bytes,0) && bytes==sizeof(*d) && FlushFileBuffers(h);
        CloseHandle(h);
        if(ok && MoveFileExW(temp,path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))return 1;
        DeleteFileW(temp);return 0;
    }
    HANDLE h=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
    if(h==INVALID_HANDLE_VALUE)return 0;
    Rc4Disk saved={0};int ok=GetFileSize(h,0)==sizeof(saved) && ReadFile(h,&saved,sizeof(saved),&bytes,0) && bytes==sizeof(saved);
    CloseHandle(h);
    if(!ok || saved.magic!=RC4_MAGIC || saved.league!=d->league || saved.year!=d->year || saved.count!=d->count ||
       saved.signature!=d->signature || saved.proof>1 || saved.pairs>RC4_PAIRS ||
       saved.checksum!=rc4_hash(UINT64_C(1469598103934665603),&saved,offsetof(Rc4Disk,checksum)))return 0;
    for(unsigned i=0;i<saved.pairs;i++){
        Rc4Pair *p=saved.pair+i;
        if(p->team>=138 || !p->slot || p->slot>128 || p->primary>=RC4_ROWS || p->backup>=RC4_ROWS ||
           p->primary==p->backup || p->week>6 || !rc4.member[p->primary].index || !rc4.member[p->backup].index ||
           rc4.member[p->primary].pos!=0 || rc4.member[p->backup].pos!=0)return 0;
    }
    *d=saved;return 1;
}
/* On a cold reload, the current save's own offers determine which cached
   pairs from this cohort existed on this branch of the dynasty. */
static void rc4_restore_pairs(Rec f,Rec pool,unsigned week){
    if(!rc4.disk.pairs)return;
    Rec teams=array_field(f,45);U n=teams?array_count(teams):0;
    if(!n || n>138){release(teams);rc4.disk.pairs=0;return;}
    Rec by_team[138]={0};
    for(U i=0;i<n;i++){
        Rec team=array_at(teams,i);unsigned t=team?scalar(team,383,0x100):138;
        if(t<138 && !by_team[t])by_team[t]=team;else release(team);
    }
    unsigned kept=0;
    for(unsigned i=0;i<rc4.disk.pairs;i++){
        Rc4Pair pair=rc4.disk.pair[i];Rec team=by_team[pair.team];
        if(pair.week>week || !team)continue;
        unsigned members[2]={pair.primary,pair.backup};int confirmed=1;
        for(unsigned j=0;j<2 && confirmed;j++){
            unsigned rr=members[j];Rec player=array_at(pool,rc4.member[rr].index-1);
            confirmed=player && row(player)==rr;
            if(confirmed && scalar(player,9,0x108)<5){
                Rec offer=t36_target(team,player);unsigned status=offer?scalar(offer,12,0x108):0;
                confirmed=status>=2 && status<=4;release(offer);
            }
            release(player);
        }
        if(confirmed)rc4.disk.pair[kept++]=pair;
    }
    for(unsigned i=0;i<138;i++)release(by_team[i]);
    rc4.disk.pairs=kept;release(teams);
}
static void rc4_save(void){if(!rc4_file(1))log_line("RC4_CACHE_WRITE_UNAVAILABLE,%u,%u",rc4.year,rc4.week);}
static void rc4_inactive(void){
    rc4.rec_table=0;rc4.franchise=0;rc4.window=0;
    Rec held=rc4.held_franchise;rc4.held_franchise=0;release(held);
}
/* Called only at recruiting boundaries. A non-null reason array is from the final
   Players Leaving request records, including native persuasion and roster additions. */
static void rc4_bind(Rec f,Rec s,Rec list,const unsigned char *reason) {
    if(!f || !s || scalar(s,5,0x108)!=6 || scalar(s,4,0x100)>6){rc4_inactive();return;}
    unsigned year=scalar(s,2,0x100),week=scalar(s,4,0x100),league=scalar(f,28,0x100);
    unsigned count=list?(unsigned)array_count(list):0;
    if(!reason && rc4_publishing.f==(U)f && rc4_publishing.year==year && (!rc4.disk.proof || rc4.count!=count))reason=rc4_publishing.reason;
    if(!league || !count || count>12000){rc4_inactive();return;}
    int warm=rc4.rec_table && rc4.franchise==(U)f && rc4.pool==(U)list && rc4.year==year &&
             rc4.disk.league==league && rc4.count==count && week>=rc4.week;
    int window=scalar(s,50,0xf8)!=0;
    if(!warm || week!=rc4.week || window!=rc4.window)log_line("RC4_PORTAL_WINDOW,%u,%u,%d",year,week,window);
    rc4.week=week;rc4.window=window;
    if(warm && !reason)return;
    rc4.rec_table=0;
    U rt=0,pt=0;uint64_t fingerprint=0;unsigned nils=0,matched=0;
    unsigned char tags[sizeof(rc4.disk.nil)]={0};
    memset(rc4.member,0,sizeof(rc4.member));
    for(unsigned i=0;i<count;i++){
        Rec r=array_at(list,i),p=r && transfer_recruit(r)?reference(r,5):0;
        unsigned rr=r?row(r):RC4_ROWS,pr=p?row(p):RC4_ROWS;
        if(!p || pr>=RC4_ROWS || rr>=RC4_ROWS || rc4.member[rr].index ||
           (rt && (rt!=q((U)r+0x10) || pt!=q((U)p+0x10)))){release(p);release(r);return;}
        rt=q((U)r+0x10);pt=q((U)p+0x10);
        Rc4Member *m=rc4.member+rr;
        *m=(Rc4Member){i+1,pr,scalar(p,228,0x100),scalar(p,230,0x100),(unsigned char)scalar(p,3,0x108),
                     (unsigned char)scalar(p,231,0x108),(unsigned char)scalar(p,233,0x108),0};
        unsigned key[]={rr,pr,m->pid,m->origin,m->pos,m->mot};
        fingerprint+=rc4_hash(UINT64_C(1469598103934665603),key,sizeof(key));
        if(reason && reason[pr]){matched++;if(reason[pr]==16){tags[rr>>3]|=(unsigned char)(1u<<(rr&7));nils++;}}
        release(p);release(r);
    }
    if(!warm || rc4.disk.signature!=fingerprint){
        memset(&rc4.disk,0,sizeof(rc4.disk));
        rc4.disk.magic=RC4_MAGIC;rc4.disk.year=year;rc4.disk.league=league;rc4.disk.count=count;rc4.disk.signature=fingerprint;
        if(rc4_file(0)){
            if(reason && rc4.disk.pairs){rc4.disk.pairs=0;rc4.disk.proof=0;}
            else rc4_restore_pairs(f,list,week);
        }
    }
    if(rc4.held_franchise!=f){
        ((void (*)(Rec))vfunc(f,0))(f);
        Rec prior=rc4.held_franchise;rc4.held_franchise=f;release(prior);
    }
    rc4.franchise=(U)f;rc4.franchise_table=q((U)f+0x10);rc4.year=year;rc4.count=count;
    rc4.pool=(U)list;rc4.rec_table=rt;rc4.player_table=pt;
    if(reason && matched){
        int changed=!rc4.disk.proof || memcmp(tags,rc4.disk.nil,sizeof(tags));
        memcpy(rc4.disk.nil,tags,sizeof(tags));rc4.disk.proof=1;
        if(changed)rc4_save();
        log_line("RC4_NIL_ENTRY,%u,%u,%u,%u,%u,FINAL_REQUEST_AND_PORTAL_POOL",year,week,count,matched,nils);
    }else if(!warm)log_line("RC4_PORTAL_BIND,%u,%u,%u,%u,%u",year,week,count,rc4.disk.proof,rc4.disk.pairs);
}
static void rc4_restore_if_cold(U eval){
    if(rc4.rec_table || !eval)return;
    Rec f=context_ref(eval,6),s=f?reference(f,44):0;
    if(s && scalar(s,5,0x108)==6 && scalar(s,4,0x100)<=6){Rec pool=array_field(f,46);rc4_bind(f,s,pool,0);release(pool);}
    release(s);release(f);
}
static Rc4Member *rc4_member(Rec r) {
    if(!rc4.rec_table || !r || q((U)r+0x10)!=rc4.rec_table || row(r)>=RC4_ROWS)return 0;
    Rc4Member *m=rc4.member+row(r);return m->index?m:0;
}
static int rc4_elite(U rec){Rc4Member *m=rc4_member((Rec)rec);return m && m->pos==0 && (m->stars==3 || m->stars==4);}
static int rc4_live_window(U eval){
    Rec f=context_ref(eval,6),s=f?reference(f,44):0;
    int open=s && scalar(s,5,0x108)==6 && scalar(s,4,0x100)<=6 && scalar(s,50,0xf8);
    release(s);release(f);return open;
}
static int rc4_grade(Rec r,Rec p,Rec info,int base){
    Rc4Member *m=rc4_member(r);
    if(!m || !rc4.disk.proof || !info || base<0 || base>=12 || m->mot!=scalar(info,3,0x108) ||
       !(rc4.disk.nil[row(r)>>3] & (1u<<(row(r)&7))) || scalar(r,9,0x108)>5)return base;
    Rec s=rc4.held_franchise?reference(rc4.held_franchise,44):0;
    int in_portal=s && scalar(s,5,0x108)==6 && scalar(s,2,0x100)==rc4.year && scalar(s,4,0x100)<=6;
    release(s);if(!in_portal)return base;
    Rec player=reference(r,5);int valid=player && (p==r || (p && row(p)==m->player && q((U)p+0x10)==rc4.player_table)) && row(player)==m->player &&
        q((U)player+0x10)==rc4.player_table && scalar(player,228,0x100)==m->pid;
    release(player);
    if(valid && !m->observed){
        m->observed=1;
        log_line("RC4_NIL_GRADE,%u,%u,%u,%u,%u,%u,%u,%d,%d",rc4.year,rc4.week,m->pid,row(r),m->player,m->pos,m->mot,base,base+1);
    }
    return valid?base+1:base;
}
