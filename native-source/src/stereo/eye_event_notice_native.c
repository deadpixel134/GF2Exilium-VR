#ifndef GF2VR_EVENT_NOTICE_HANDOFF
#error Use original eye_gpu_copy.c when event handoff is OFF
#endif
#define eye_gpu_submit notice_submit_original
#define eye_gpu_drain notice_drain_original
#include "eye_gpu_copy.c"
#undef eye_gpu_submit
#undef eye_gpu_drain
#define GF2_EYE_EVENT_NOTICE_IMPLEMENTATION
#include "eye_event_notice.h"
int eye_gpu_submit(EyeGpuCopy *p,void *d,void *c,void *l,void *r,void *dl,void *dr,EyeGpuTicket t){
 int result=notice_submit_original(p,d,c,l,r,dl,dr,t);if(result)eye_gpu_notice_post(p);return result;}
int eye_gpu_drain(EyeGpuCopy *p,void *d,void *c,void *l,void *r,EyeGpuTicket t){
 int result=notice_drain_original(p,d,c,l,r,t);if(result)eye_gpu_notice_post(p);return result;}
