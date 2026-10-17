#ifndef GF2_ACTIVITY_CANVAS_SCALE_H
#define GF2_ACTIVITY_CANVAS_SCALE_H
/* Original scalar ownership only. No inverse scaling, layout/child transform
   reset or force during WorldSpace. Caller retains exact original identities. */
typedef struct {float original;int width,height;unsigned valid;} CanvasScaleSnapshot;
typedef struct {
    void *context;
    int (*current)(void *);
    int (*read)(void *,float *);
    int (*write)(void *,float);
    int (*viewport)(void *,int *,int *);
} CanvasScaleApi;
static int canvas_scale_valid(float value) {
    union {float f;unsigned bits;} u={value};
    return value>0 && (u.bits&0x7f800000u)!=0x7f800000u;
}
static int canvas_scale_capture(CanvasScaleSnapshot *s,float value,int width,int height) {
    if(!s || !canvas_scale_valid(value) || width<=0 || height<=0)return 0;
    *s=(CanvasScaleSnapshot){value,width,height,1};return 1;
}
static int canvas_scale_restore(const CanvasScaleSnapshot *s,const CanvasScaleApi *api) {
    if(!s || !s->valid || !canvas_scale_valid(s->original) || !api || !api->current ||
       !api->read || !api->write || !api->viewport || !api->current(api->context))return 0;
    float current=0;int width=0,height=0;
    if(!api->read(api->context,&current) || !canvas_scale_valid(current) ||
       !api->viewport(api->context,&width,&height) || !api->current(api->context))return 0;
    /* Changed viewport or foreign finite scalar belongs to the game. The
       caller still restores mode1, but does not impose an old scalar. */
    if(width!=s->width || height!=s->height || current==s->original || current!=1.0f)
        return api->current(api->context);
    /* Observed leak: owned mode2 delayed scale2.4 ->1. Restore BEFORE mode1
       native reshape. A setter can throw after applying: readback decides. */
    (void)api->write(api->context,s->original);
    float actual=0;
    return api->current(api->context) && api->read(api->context,&actual) &&
        canvas_scale_valid(actual) && actual==s->original && api->current(api->context);
}
#endif
