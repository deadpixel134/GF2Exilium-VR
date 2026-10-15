#ifndef GF2_ACTIVITY_CANVAS_SCOPE_H
#define GF2_ACTIVITY_CANVAS_SCOPE_H
#include <stdint.h>
/* One main-owner Canvas borrow. A successful setter is not restoration proof.
   Caller retains the exact Canvas/Transform until active becomes zero. */
typedef struct {
    void *context;
    int (*mode)(void *,unsigned *);
    int (*set_mode)(void *,unsigned);
    int (*geometry)(void *,float [20]); /* world matrix + local rect, with binding checks */
} ActivityCanvasScopeApi;
typedef struct {
    uint64_t serial;unsigned owner,active,failed;int camera;
    float geometry[20];
} ActivityCanvasScope;
static int acs_geometry_equal(const float *a,const float *b) {
    for(unsigned i=0;i<20;++i) {
        union {float f;unsigned u;} x={a[i]},y={b[i]};
        if((x.u&0x7f800000u)==0x7f800000u || (y.u&0x7f800000u)==0x7f800000u || x.f!=y.f)return 0;
    }
    return a[18]>0 && a[19]>0;
}
static int acs_restore(ActivityCanvasScope *s,const ActivityCanvasScopeApi *a,unsigned thread) {
    if(!s || !a || !thread || s->owner!=thread)return 0;
    if(!s->active)return !s->failed;
    unsigned mode=99;
    if(!a->mode(a->context,&mode)){s->failed=1;return 0;}
    if(mode==2) {
        /* A throw can occur after a setter takes effect. Always read back. */
        (void)a->set_mode(a->context,1);
        if(!a->mode(a->context,&mode)){s->failed=1;return 0;}
    }
    if(mode!=1){s->failed=1;return 0;} /* unknown/foreign mode: no blind overwrite */
    float check[20];
    int unchanged=a->geometry(a->context,check) && acs_geometry_equal(s->geometry,check);
    s->active=0; /* our mode change is gone even if the game changed geometry/binding */
    if(!unchanged)s->failed=1;
    return !s->failed;
}
static int acs_begin(ActivityCanvasScope *s,const ActivityCanvasScopeApi *a,unsigned thread,
    uint64_t serial,int camera) {
    if(!s || !a || !a->mode || !a->set_mode || !a->geometry || !thread || !serial || !camera || s->active)return 0;
    unsigned mode=99;float before[20];
    if(!a->mode(a->context,&mode) || mode!=1 || !a->geometry(a->context,before) ||
       !acs_geometry_equal(before,before))return 0;
    *s=(ActivityCanvasScope){.serial=serial,.owner=thread,.camera=camera,.active=1};
    for(unsigned i=0;i<20;++i)s->geometry[i]=before[i];
    int called=a->set_mode(a->context,2);float after[20];
    if(!called || !a->mode(a->context,&mode) || mode!=2 || !a->geometry(a->context,after) ||
       !acs_geometry_equal(before,after)) {
        s->failed=1;(void)acs_restore(s,a,thread);return 0;
    }
    return 1;
}
static int acs_end(ActivityCanvasScope *s,const ActivityCanvasScopeApi *a,unsigned thread,
    uint64_t serial,int camera) {
    if(!s || !s->active || s->owner!=thread || s->serial!=serial || s->camera!=camera)return 0;
    /* Compare before restoring too; a setter must not hide changed geometry. */
    float check[20];unsigned mode=99;
    if(!a->mode(a->context,&mode) || mode!=2 || !a->geometry(a->context,check) ||
       !acs_geometry_equal(s->geometry,check))s->failed=1;
    return acs_restore(s,a,thread);
}
#endif
