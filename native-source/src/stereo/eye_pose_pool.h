#ifndef GF2_EYE_POSE_POOL_H
#define GF2_EYE_POSE_POOL_H
#include "eye_flight_owner.h"
#include "xr_eye_input.h"
/* Two immutable by-value predictions associated with the numeric flight owner.
   Zero initialize once. Owner must outlive the pool. No input, LocateViews,
   reference capture, native pointer or Unity calls are performed here.
   Only offer/retire below may allocate/recycle this owner's slots; all other
   numeric flight transitions remain on EyeFlightOwner. BUSY requires retry.
   Lock order: pool -> owner; never enter this pool while holding owner.locked.
   Returned copies are NOT permission to render/publish: recheck flight tokens
   at those boundaries. Cancelled payloads remain retained for retirement. */
typedef struct {
    EyeFlightToken token;
    XrEyeInput pose;
} EyePoseSlot;
typedef struct {
    unsigned locked;
    EyePoseSlot slots[EFO_CAPACITY];
} EyePosePool;
int eye_pose_pool_offer(EyePosePool *,EyeFlightOwner *,EyeFlightToken,
                        long long display_time,int renderable,const XrEyeInput *,unsigned *slot);
/* Empty shouldRender=false frames carry a numeric frame obligation, no pose. */
int eye_pose_pool_read(EyePosePool *,EyeFlightOwner *,EyeFlightToken,
                       EyeFlightSnapshot *,XrEyeInput *);
/* Must follow take + actual native/managed payload cleanup. This clears the
   pose and acknowledges numeric reuse together; pending/fault slots refuse. */
int eye_pose_pool_retire(EyePosePool *,EyeFlightOwner *,EyeFlightToken);
#endif
