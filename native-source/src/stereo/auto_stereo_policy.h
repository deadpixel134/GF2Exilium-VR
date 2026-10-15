#ifndef GF2_AUTO_STEREO_POLICY_H
#define GF2_AUTO_STEREO_POLICY_H
#include "main_test_grant.h"
/* Main-owner intent only. NOT a source, pose, GPU or presentation grant. */
typedef struct {unsigned manual_flat;} AutoStereoIntent;
static inline int auto_stereo_intent_update(AutoStereoIntent *a,MainTestGrant *g,
                                           int enabled,int focused,int down) {
    focused=focused && enabled;
    int pressed=focused && down && g->key_released;
    main_test_key(g,focused,down);
    if(pressed)a->manual_flat=g->state==MTG_OFF;
    if(enabled && focused && !a->manual_flat && g->state==MTG_OFF) {
        g->state=MTG_REQUESTED;return 1;
    }
    return 0;
}
#endif
