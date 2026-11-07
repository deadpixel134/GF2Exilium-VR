#ifndef GF2VR_LIVE_SETTINGS_TRANSACTION_H
#define GF2VR_LIVE_SETTINGS_TRANSACTION_H
#include "live_settings_config.h"
/* Sole main-owner transaction engine. Transport and XR never mutate its banks.
   Adapter returns 1=confirmed, 0=failed, -1=pending at its safe owner boundary. */
typedef struct {
    int (*prepare)(const LiveSettingsDocuments *,const LiveSettingsValues *);
    int (*apply)(const LiveSettingsDocuments *,const LiveSettingsValues *,unsigned rollback);
    void (*finish)(unsigned committed);
} LiveSettingsAdapter;
typedef struct {LiveSettingsDocuments documents;LiveSettingsValues values;} LiveSettingsBank;
typedef struct {
    LiveSettingsBank banks[2];unsigned current,previous,proposed,phase,owners,initialized,operation,failed_apply;
    unsigned long long revision,transaction,base,began,prepared_hash;unsigned prepared_length;
} LiveSettingsTransaction;
static unsigned long long live_settings_request_hash(const unsigned char *p,unsigned n){
    unsigned long long hash=14695981039346656037ull;for(unsigned i=0;i<n;++i){hash^=p[i];hash*=1099511628211ull;}return hash;
}
static void live_settings_reply(LiveSettingsWire *w,unsigned status,unsigned length,unsigned long long revision){
    w->status=status;w->length=length;w->revision=revision;__atomic_store_n(&w->ready,2,__ATOMIC_RELEASE);
}
static unsigned live_settings_state_payload(LiveSettingsWire *w,const LiveSettingsTransaction *t){
    unsigned char *p=w->payload;live_settings_put32(p,w->pid);live_settings_put32(p+4,w->dirty);
    live_settings_put32(p+8,t->phase);live_settings_put32(p+12,t->owners);
    live_settings_put64(p+16,w->instance);live_settings_put64(p+24,t->transaction);unsigned at=32;
    const char *strings[]={w->root_utf8,w->build};
    for(unsigned i=0;i<2;++i){unsigned n=live_settings_length(strings[i]);live_settings_put32(p+at,n);at+=4;
        for(unsigned k=0;k<n;++k)p[at++]=(unsigned char)strings[i][k];}
    return at+live_settings_documents_encode(p+at,&t->banks[t->current].documents);
}
static void live_settings_transaction_tick(LiveSettingsTransaction *t,LiveSettingsWire *w,const LiveSettingsAdapter *a,unsigned long long now){
    // A timed-out compensating operation retains its immutable bank and keeps progressing.
    if(t->phase==4&&!t->operation){
        int recovered=a->apply(&t->banks[t->previous].documents,&t->banks[t->previous].values,1);
        if(recovered==1){t->current=t->previous;t->revision=t->base;t->phase=0;t->owners=15;a->finish(0);}
    }
    if(__atomic_load_n(&w->ready,__ATOMIC_ACQUIRE)!=1)return;
    if(!t->operation){
        if(w->command==0){
            if(t->initialized||!live_settings_documents_decode(w->payload,w->length,w->dirty,&t->banks[0].documents)||
               !live_settings_config_parse(&t->banks[0].documents,&t->banks[0].values)){live_settings_reply(w,LSW_REJECTED,0,0);return;}
            t->initialized=1;t->current=0;t->revision=1;live_settings_reply(w,LSW_OK,0,t->revision);return;
        }
        if(!t->initialized){live_settings_reply(w,LSW_UNAVAILABLE,0,0);return;}
        if(w->command!=LSW_PREPARE&&w->length){live_settings_reply(w,LSW_REJECTED,0,t->revision);return;}
        if(w->command==LSW_STATE){live_settings_reply(w,LSW_OK,live_settings_state_payload(w,t),t->revision);return;}
        if(w->command==LSW_PREPARE){
            if((t->phase==1||t->phase==2)&&w->request==t->transaction&&w->revision==t->base){
                unsigned same=w->length==t->prepared_length&&live_settings_request_hash(w->payload,w->length)==t->prepared_hash;
                live_settings_reply(w,same?LSW_OK:LSW_REJECTED,0,t->revision);return;
            }
            if(t->phase||w->revision!=t->revision||t->revision==~0ull){live_settings_reply(w,LSW_BUSY,0,t->revision);return;}
            t->proposed=t->current^1u;LiveSettingsBank *b=&t->banks[t->proposed];
            if(!live_settings_documents_decode(w->payload,w->length,w->dirty,&b->documents)||!live_settings_config_parse(&b->documents,&b->values)||
               a->prepare(&b->documents,&b->values)!=1){live_settings_reply(w,LSW_REJECTED,0,t->revision);return;}
            t->transaction=w->request;t->base=t->revision;t->previous=t->current;t->phase=1;t->owners=0;t->failed_apply=0;
            t->prepared_hash=live_settings_request_hash(w->payload,w->length);t->prepared_length=w->length;
            live_settings_reply(w,LSW_OK,0,t->revision);return;
        }
        if(w->request!=t->transaction){live_settings_reply(w,LSW_BUSY,0,t->revision);return;}
        if(w->command==LSW_FINALIZE){
            if(w->revision!=t->base+1||!((t->phase==2)||(t->phase==0&&t->revision==t->base+1))){live_settings_reply(w,LSW_REJECTED,0,t->revision);return;}
            if(t->phase==0){live_settings_reply(w,LSW_OK,0,t->revision);return;}
            int final=a->apply(&t->banks[t->current].documents,&t->banks[t->current].values,0);
            if(final<0)return;
            if(final==0){live_settings_reply(w,LSW_REJECTED,0,t->revision);return;}
            t->phase=0;a->finish(1);live_settings_reply(w,LSW_OK,0,t->revision);return;
        }
        if(w->command==LSW_APPLY){
            if(w->revision!=t->base){live_settings_reply(w,LSW_BUSY,0,t->revision);return;}
            if(t->phase==2){live_settings_reply(w,LSW_OK,live_settings_state_payload(w,t),t->revision);return;}
            if(t->phase!=1){live_settings_reply(w,LSW_BUSY,0,t->revision);return;}
            t->phase=3;t->operation=LSW_APPLY;t->began=now;
        }else if(w->command==LSW_ROLLBACK){
            if(w->revision!=t->base){live_settings_reply(w,LSW_BUSY,0,t->revision);return;}
            if(t->phase==1||(t->phase==0&&t->revision==t->base)){
                t->phase=0;a->finish(0);live_settings_reply(w,LSW_OK,0,t->revision);return;
            }
            if(t->phase!=2&&t->phase!=3&&t->phase!=4){live_settings_reply(w,LSW_REJECTED,0,t->revision);return;}
            t->phase=4;t->operation=LSW_ROLLBACK;t->began=now;
        }else{live_settings_reply(w,LSW_REJECTED,0,t->revision);return;}
    }
    unsigned rollback=t->phase==4;LiveSettingsBank *b=&t->banks[rollback?t->previous:t->proposed];
    int applied=a->apply(&b->documents,&b->values,rollback);
    if(applied==1){
        t->current=rollback?t->previous:t->proposed;t->revision=t->base+(rollback?0:1);t->owners=15;
        unsigned status=rollback&&t->failed_apply?LSW_REJECTED:LSW_OK;t->phase=rollback?0:2;
        t->operation=0;if(rollback)a->finish(0);
        live_settings_reply(w,status,!rollback?live_settings_state_payload(w,t):0,t->revision);return;
    }
    if(applied==0&&!rollback){t->phase=4;t->failed_apply=1;t->began=now;return;}
    if(now<t->began||now-t->began>20000){
        // A failed/pending owner is never reported as applied or freed on a timeout.
        t->phase=4;t->failed_apply=1;t->operation=0;live_settings_reply(w,LSW_UNAVAILABLE,0,t->revision);
    }
}
#endif
