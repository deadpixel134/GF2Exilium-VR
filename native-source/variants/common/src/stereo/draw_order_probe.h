#ifndef GF2_DRAW_ORDER_PROBE_H
#define GF2_DRAW_ORDER_PROBE_H
#include "eye_event_copy.h"
/* Diagnostic resources only. The event query observes prior immediate-context
   work; these tiny textures are not game-eye output or an XR image lease. */
int draw_order_prepare(EyeEventCopy *,int,EyeGpuTicket,unsigned,void *,void *);
#endif
