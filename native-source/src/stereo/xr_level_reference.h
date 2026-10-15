#ifndef GF2_XR_LEVEL_REFERENCE_H
#define GF2_XR_LEVEL_REFERENCE_H
#include "xr_eye_input.h"
/* LOCAL owns forward/up, independently of the user's entry posture. Position
   compensation maps the authored camera to the first tracked HMD position;
   physical rotation (including yaw) stays live. No Guardian/floor assumption. */
static inline int xr_level_reference(const XrPosef *head,XrPosef *out) {
    if(!out)return 0;
    *out=(XrPosef){0};
    if(!head || !xr_eye_pose_valid(head))return 0;
    XrPosef reference={.orientation.w=1,.position=head->position};
    if(!xr_eye_pose_valid(&reference))return 0;
    *out=reference;return 1;
}
#endif
