/* Existing integer/enum record setter shared by position and roster processing. */
static __declspec(noinline) int set_scalar(Rec p,unsigned index,unsigned kind,int value) {
    U data,field,vt,fn;uint32_t row;int after;
    if(kind==1)return 0; /* no boolean writes in the authorized pilot */
    if(!field_context(p,index,&data,&row,&field) || !local_read(data,&vt,8) ||
       !local_read(vt+0x148,&fn,8) || !code_pointer(fn))return 0;
    ((void(*)(U,uint32_t,U,int))fn)(data,row,field,value);
    ((void(*)(U))(game_base+0x80595e0))(data);
    return scalar(p,index,kind,&after) && after==value;
}
