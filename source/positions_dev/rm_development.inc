#include "annual_capture.inc"
static void dev_apply_down(void *module,Rec player,Rec team,int index,int before) {
    const RMDevRecord *d=dev_records+index;int after;
    if(!scalar(player,271,3,&after)){capture_error("development trait read");return;}
    if(after<0||after>3||after<before){capture_error("unexpected native development result");return;}
    unsigned draw=(unsigned)(dev_draw((unsigned)index,2)%10000);
    if(after>before){log_line("DEV_DECISION row=%u before=%d after=%d action=UP downgrade_skipped=1",d->row,before,after);return;}
    if(!before||!d->has_row||d->down_bp<=0||draw>=(unsigned)d->down_bp) {
        if(rm18_deep())
            log_line("DEV_DECISION row=%u before=%d after=%d action=HOLD down_bp=%d draw=%u reason=%s",d->row,before,after,d->down_bp,draw,dev_reasons[d->reason]);
        else
            InterlockedIncrement(&dev_held_by_team[d->team]);
        return;
    }
    int values[RM_FIELD_COUNT],expected[RM_FIELD_COUNT];Rec nil=0;
    if(!read_all(player,values)){capture_error("development preservation capture");return;}
    player_snapshot_values("DOWN_BEFORE",player,d->source,values);
    memcpy(expected,values,sizeof(values));expected[field_slot("TraitDevelopment")]=before-1;
    if(!set_scalar(player,271,3,before-1)){capture_error("development decrease write");return;}
    ((void(*)(void*,Rec*,Rec*,int,Rec*))(game_base+0x831b8c0))(module,&player,&team,-1,&nil);release(nil);
    player_snapshot("DOWN_NATIVE_AFTER",player,d->source,0,0);
    if(!read_all(player,values)){capture_error("development after capture");return;}
    for(unsigned f=0;f<RM_FIELD_COUNT;f++)if(values[f]!=expected[f]&&!set_slot(player,(int)f,expected[f])){capture_error("development preservation repair");return;}
    if(!read_all(player,values)||memcmp(values,expected,sizeof(values))){capture_error("development preservation mismatch");return;}
    player_snapshot_values("DOWN_AFTER",player,d->source,values);
    log_line("DEV_DECISION row=%u before=%d after=%d action=DOWN down_bp=%d draw=%u reason=%s preservation=PASS",d->row,before,before-1,d->down_bp,draw,dev_reasons[d->reason]);
}
static void dev_callback(void *module,Rec *player,Rec *team,Rec *talent,int source) {
    RMDevContext context={0};context.index=-1;
    void *previous=0,*previous_log=TlsGetValue(tls_index);int id=-1,before=-1,active=0;
    U store=0;uint32_t row=0;
    if(team&&player&&*player&&target(*team)&&scalar(*team,383,2,&id)&&league_valid&&
       prep_seen[id]==2&&!training_seen[id]&&identity(*player,&store,&row)) {
        int index=dev_lookup(row);
        if(index>=0&&dev_state[index].ready&&dev_state[index].store==store&&dev_records[index].team==id) {
            if(InterlockedCompareExchange(&dev_state[index].seen,1,0)!=0) {
                log_line("DEV_DUPLICATE_SKIPPED row=%u no_second_development_event",row);return;
            }
            if(scalar(*player,271,3,&before)&&before>=0&&before<=3) {
                if(before!=dev_records[index].before) {
                    dev_state[index].ready=0;
                    log_line("DEV_CONTEXT_DEFER row=%u original_trait=%d current_trait=%d native_only=1",row,dev_records[index].before,before);
                    goto forward;
                }
                context.player=*player;context.team=*team;context.index=index;
                previous=TlsGetValue(dev_tls_index);
                if(TlsSetValue(dev_tls_index,&context)&&TlsSetValue(tls_index,(void*)(intptr_t)(id+1)))active=1;
                else {TlsSetValue(dev_tls_index,previous);capture_error("development TLS context");}
            } else capture_error("development initial trait");
        }
    }
forward:
    if(active)player_snapshot("DEV_BEFORE",*player,source,0,0);
    ((void(*)(void*,Rec*,Rec*,Rec*))(game_base+0x831bae0))(module,player,team,talent);
    if(active) {
        TlsSetValue(dev_tls_index,previous);
        if(!capture_fault) {
            if(context.score_calls!=1||context.chance_calls!=1)capture_error("development callback count");
            else dev_apply_down(module,*player,*team,context.index,before);
        }
        player_snapshot("DEV_AFTER",*player,source,context.score_calls,context.score);
    }
    TlsSetValue(tls_index,previous_log);
}
static void rm_dev_roster(void *a,Rec*b,Rec*c,Rec*d){dev_callback(a,b,c,d,0);}
static void rm_dev_commit(void *a,Rec*b,Rec*c,Rec*d){dev_callback(a,b,c,d,1);}
static int rm_score(void *module,Rec *player) {
    int result=((int(*)(void*,Rec*))(game_base+0x831b4f0))(module,player);
    RMDevContext *c=TlsGetValue(dev_tls_index);
    if(c&&enabled&&!capture_fault&&player&&same_record(*player,c->player)) {
        const RMDevRecord *d=dev_records+c->index;
        if(rm18_deep())
            log_line("SEASON_SCORE row=%u native_current=%d original_native=%d revised=%d original_position=%d has_row=%d",d->row,result,d->native_score,d->score,d->position,d->has_row);
        result=d->score;c->score=result;c->score_calls++;
    }
    return result;
}
static int rm_chance(void *module,int score,Rec *player,Rec *talent) {
    int native=((int(*)(void*,int,Rec*,Rec*))(game_base+0x831b060))(module,score,player,talent);
    RMDevContext *c=TlsGetValue(dev_tls_index);
    if(c&&enabled&&!capture_fault&&player&&same_record(*player,c->player)) {
        const RMDevRecord *d=dev_records+c->index;
        unsigned draw=(unsigned)(dev_draw((unsigned)c->index,1)%101);c->chance_calls++;
        int gate=d->before<3&&d->up_outcomes>0;
        int eligible=gate&&(int)draw<=native;
        if(rm18_deep())
            log_line("UPGRADE_DRAW row=%u score=%d native_chance=%d draw_0_100=%u eligible=%d base_outcomes=%d gate=%d native_modifier=%d",
                     d->row,score,native,draw,eligible,d->up_outcomes,gate,gate?native-(d->up_outcomes-1):0);
        return eligible?101:-1;
    }
    return native;
}
