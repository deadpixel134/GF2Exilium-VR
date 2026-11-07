#ifndef GF2VR_LIVE_SETTINGS_GESTURE_H
#define GF2VR_LIVE_SETTINGS_GESTURE_H
/* Pure side-Grip classifier. Hand selection belongs to the existing aux owner.
   Release classifies a short press; a long release never toggles the panel too. */
enum { LSG_NONE, LSG_SHORT, LSG_LONG };
typedef struct {unsigned armed,down,fired;unsigned long long since;} LiveSettingsGesture;
static void live_settings_gesture_reset(LiveSettingsGesture *s) { *s=(LiveSettingsGesture){0}; }
static unsigned live_settings_gesture_step(LiveSettingsGesture *s,unsigned valid,unsigned down,unsigned long long now) {
    if(!valid){live_settings_gesture_reset(s);return LSG_NONE;}
    if(!s->armed){if(!down)s->armed=1;return LSG_NONE;}
    if(down&&!s->down){s->since=now;s->fired=0;s->down=1;return LSG_NONE;}
    if(s->down&&now<s->since){live_settings_gesture_reset(s);return LSG_NONE;}
    unsigned result=LSG_NONE;
    if(s->down&&!s->fired&&now-s->since>=1000){s->fired=1;result=LSG_LONG;}
    if(!down&&s->down){if(!s->fired)result=LSG_SHORT;s->down=s->fired=0;s->since=0;}
    return result;
}
/* A modal/focus/role transition releases inputs, then requires a wholly neutral
   tracked sample before any game input may resume. Contact/tracking bypass it. */
typedef struct {unsigned blocked;} LiveSettingsInputGate;
static unsigned live_settings_input_allowed(LiveSettingsInputGate *g,unsigned modal,unsigned valid,unsigned neutral) {
    if(modal||!valid){g->blocked=1;return 0;}
    if(g->blocked){if(neutral)g->blocked=0;return 0;}
    return 1;
}
#endif
