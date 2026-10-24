#ifndef GF2VR_PC_OUTPUT_H
#define GF2VR_PC_OUTPUT_H
#include "performance_config.h"
/* Pure main-owner state machine. The adapter's managed ABI is mockable. */
typedef struct {
    int (*read)(void *,int *,int *,int *);
    int (*set)(void *,int,int,int);
    void (*report)(void *,const char *,unsigned long long);
    void *context;
    int (*desktop)(void *,int *,int *);
} PcOutputApi;
typedef struct {unsigned captured,failed,applied;int width,height,mode,current_width,current_height;} PcOutput;
static void pc_output_fail(PcOutput *s,const PcOutputApi *a,unsigned reason) {
    if(!s->failed){s->failed=1;a->report(a->context,"pc_output_failed",reason);}
}
static void pc_output_tick(PcOutput *s,const PcOutputConfig *c,const PcOutputApi *a,int entered,int exited) {
    if(!c->fixed||s->failed)return;
    if(!s->captured) {
        if(!a->read(a->context,&s->width,&s->height,&s->mode)||s->width<1||s->height<1){pc_output_fail(s,a,1);return;}
        s->captured=1;s->current_width=s->width;s->current_height=s->height;
        a->report(a->context,"pc_output_mode",(unsigned)s->mode);
        int desktop_width=0,desktop_height=0;
        if(!a->desktop(a->context,&desktop_width,&desktop_height)||desktop_width<1||desktop_height<1) {
            a->report(a->context,"pc_output_desktop",0);pc_output_fail(s,a,5);return;
        }
        a->report(a->context,"pc_output_desktop",((unsigned long long)(unsigned)desktop_width<<16)|(unsigned)desktop_height);
        if(!c->always) {
            if(desktop_width==(int)c->width&&desktop_height==(int)c->height)
                a->report(a->context,"pc_output_startup_native",((unsigned long long)c->width<<16)|c->height);
            else if(s->width==(int)c->width&&s->height==(int)c->height) {
                /* Unity may persist a VR-only resolution on quit/crash. Recover
                   on this safe callback, never from a process-detach handler. */
                if(!a->set(a->context,desktop_width,desktop_height,s->mode)){pc_output_fail(s,a,6);return;}
                s->width=s->current_width=desktop_width;s->height=s->current_height=desktop_height;
                a->report(a->context,"pc_output_startup_restored",((unsigned long long)(unsigned)desktop_width<<16)|(unsigned)desktop_height);
                /* Do not overwrite the startup restore in the same callback. */
                return;
            }
        }
    }
    /* Source revoke/rebuild and MTG_REQUESTED are not user/session exit. A
       resize may cause exactly those transient states; never feed them back
       into restoration and oscillate between startup and configured sizes. */
    unsigned wanted=c->always||(!exited&&(s->applied||entered));
    if(wanted==s->applied)return;
    int width=wanted?(int)c->width:s->width,height=wanted?(int)c->height:s->height;
    if(width!=s->current_width||height!=s->current_height) {
        /* Re-read current mode, never substitute a hardcoded fullscreen mode.
           Startup width/height remain the restoration authority. */
        int ignored_w,ignored_h,mode;
        if(!a->read(a->context,&ignored_w,&ignored_h,&mode)||!a->set(a->context,width,height,mode)){
            pc_output_fail(s,a,wanted?2:3);return;
        }
        s->current_width=width;s->current_height=height;
        a->report(a->context,wanted?"pc_output_applied":"pc_output_restored",((unsigned long long)(unsigned)width<<16)|(unsigned)height);
    }
    s->applied=wanted;
}
#endif
