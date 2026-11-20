#include "car35_policy.h"
#define CAR35_TEXT_RVA 0x1000u
#define CAR35_TEXT_SIZE 0x0a3b4060u
#define CAR35_GATE_RVA 0x0834eea0u
#define CAR35_CAPACITY_RVA 0x0832cce0u
#define CAR35_SCORE_RVA 0x0832c610u
static volatile LONG car35_event_count;
static int car35_local_read(uintptr_t p,void *out,size_t n) {
    SIZE_T got=0;return p && ReadProcessMemory(GetCurrentProcess(),(void*)p,out,n,&got) && got==n;
}
static int car35_code_pointer(uintptr_t p) {
    return p>=game_base+CAR35_TEXT_RVA && p<game_base+CAR35_TEXT_RVA+CAR35_TEXT_SIZE;
}
/* Scalar reads borrow the records retained by the original call stack. */
static int car35_record_field(void *record,unsigned index,int kind,int *out) {
    uintptr_t vtable, getter, type, cache, field, data, data_vtable, reader;
    uint32_t row;
    int16_t count;
    if(!record || !car35_local_read((uintptr_t)record,&vtable,8) ||
       !car35_local_read(vtable+0x38,&getter,8) || !car35_code_pointer(getter)) return 0;
    type=((uintptr_t(*)(void*))getter)(record);
    if(!type || !car35_local_read(type+0xd0,&cache,8) || !car35_local_read(type+0xd8,&count,2) ||
       count<=0 || index>=(unsigned)count || !cache) return 0;
    field=((uintptr_t(*)(void*,unsigned))(game_base+0x08055d60u))((void*)type,index);
    if(field==0xffffffffu)return 0;
    if(!car35_local_read((uintptr_t)record+0x10,&data,8) || !data ||
       !car35_local_read((uintptr_t)record+0x18,&row,4) ||
       !car35_local_read(data,&data_vtable,8) ||
       !car35_local_read(data_vtable+(kind==1?0xf8:kind==2?0x100:0x108),&reader,8) || !car35_code_pointer(reader)) return 0;
    if(kind==1) *out=((unsigned char(*)(void*,uint32_t,void*))reader)((void*)data,row,(void*)field)!=0;
    else *out=((int(*)(void*,uint32_t,void*))reader)((void*)data,row,(void*)field);
    return 1;
}

/* Stored reference reader: RCX=data, RDX=&out, R8d=row, R9=field.
   Native example: candidate builder 08342078..083420E4. The getter owns one
   reference in out; release it once through virtual +8. No record setter.
   Use Team.UserCharacter (407), never scalar-read IsUserManaged (bool()). */
static int car35_record_reference_present(void *record,unsigned index,int *out) {
    uintptr_t vtable,getter,type,cache,field,data,data_vtable,reader,release;
    uint32_t row;
    int16_t count;
    void *value=NULL;
    if(!record || !car35_local_read((uintptr_t)record,&vtable,8) ||
       !car35_local_read(vtable+0x38,&getter,8) || !car35_code_pointer(getter)) return 0;
    type=((uintptr_t(*)(void*))getter)(record);
    if(!type || !car35_local_read(type+0xd0,&cache,8) || !car35_local_read(type+0xd8,&count,2) ||
       count<=0 || index>=(unsigned)count || !cache) return 0;
    field=((uintptr_t(*)(void*,unsigned))(game_base+0x08055d60u))((void*)type,index);
    if(field==0xffffffffu)return 0;
    if(!car35_local_read((uintptr_t)record+0x10,&data,8) || !data ||
       !car35_local_read((uintptr_t)record+0x18,&row,4) || !car35_local_read(data,&data_vtable,8) ||
       !car35_local_read(data_vtable+0x128,&reader,8) || !car35_code_pointer(reader)) return 0;
    ((void*(*)(void*,void**,uint32_t,void*))reader)((void*)data,&value,row,(void*)field);
    if(value) {
        if(!car35_local_read((uintptr_t)value,&vtable,8) || !car35_local_read(vtable+8,&release,8) ||
           !car35_code_pointer(release)) return 0;
        ((void(*)(void*))release)(value);
    }
    *out=value!=NULL;
    return 1;
}


/* Retain the linked coach while the native signer consumes the offer. */
static void *car35_retain_reference(void *record,unsigned index) {
    uintptr_t vt,getter,type,field,data,dv,reader;uint32_t row;int16_t count;
    void *value=NULL;
    if(!record || !car35_local_read((uintptr_t)record,&vt,8) || !car35_local_read(vt+0x38,&getter,8) || !car35_code_pointer(getter))return NULL;
    type=((uintptr_t(*)(void*))getter)(record);
    if(!type || !car35_local_read(type+0xd8,&count,2) || count<=0 || index>=(unsigned)count)return NULL;
    field=((uintptr_t(*)(void*,unsigned))(game_base+0x08055d60u))((void*)type,index);
    if(field==0xffffffffu || !car35_local_read((uintptr_t)record+0x10,&data,8) || !data ||
       !car35_local_read((uintptr_t)record+0x18,&row,4) || !car35_local_read(data,&dv,8) ||
       !car35_local_read(dv+0x128,&reader,8) || !car35_code_pointer(reader))return NULL;
    ((void*(*)(void*,void**,uint32_t,void*))reader)((void*)data,&value,row,(void*)field);
    return value;
}
static void car35_release_reference(void *record) {
    uintptr_t vt,release;
    if(record && car35_local_read((uintptr_t)record,&vt,8) && car35_local_read(vt+8,&release,8) && car35_code_pointer(release))
        ((void(*)(void*))release)(record);
}
