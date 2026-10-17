#ifndef GF2VR_PRIVATE_TOUCH2_POSE_H
#define GF2VR_PRIVATE_TOUCH2_POSE_H

/* Private, engine-free XR-to-Unity POD boundary. Include only from the
 * GF2VR_TOUCH_PRIVATE overlay. One XR writer, one Unity reader; neither waits.
 * Zero-initialize the ring before either owner begins. Do not clear a live ring:
 * changes to the nonzero epoch invalidate old contents without racing owners.
 * Publish input and read output belong to their calling owner and must not
 * alias the ring's internal storage.
 */
#include <float.h>

typedef struct Touch2Packet {
    unsigned long long epoch, frame;
    long long time;
    unsigned tracked[2];
    float position[2][3]; /* Raw XR coordinates; untracked hands are zeroed. */
} Touch2Packet;

typedef struct Touch2Slot {
    unsigned lock;
    Touch2Packet packet;
} Touch2Slot;

typedef struct Touch2Ring {
    Touch2Slot slot[16];
} Touch2Ring;

typedef struct Touch2Common {
    double anchor[3], offset[3], reference_head[3], navigation[4];
    double head_scale, units;
} Touch2Common;

_Static_assert(__atomic_always_lock_free(sizeof(unsigned), 0),
    "Private touch slots require a lock-free unsigned compare/exchange");

static int touch2_finite(double v) {
    return v == v && v <= DBL_MAX && v >= -DBL_MAX;
}

static int touch2_identity(unsigned long long epoch,
    unsigned long long frame, long long time) {
    return epoch != 0 && frame != 0 && time > 0;
}

static int touch2_try_lock(Touch2Slot *slot) {
    unsigned expected = 0;
    return __atomic_compare_exchange_n(&slot->lock, &expected, 1, 0,
        __ATOMIC_ACQUIRE, __ATOMIC_RELAXED);
}

static void touch2_unlock(Touch2Slot *slot) {
    __atomic_store_n(&slot->lock, 0, __ATOMIC_RELEASE);
}

static int touch2_packet_valid(const Touch2Packet *p) {
    if (!p || !touch2_identity(p->epoch, p->frame, p->time)) return 0;
    for (unsigned hand = 0; hand < 2; ++hand) {
        if (p->tracked[hand] > 1) return 0;
        if (p->tracked[hand]) for (unsigned axis = 0; axis < 3; ++axis)
            if (!touch2_finite(p->position[hand][axis])) return 0;
    }
    return 1;
}

static int touch2_publish(Touch2Ring *ring, const Touch2Packet *packet) {
    if (!ring || !touch2_packet_valid(packet)) return 0;
    Touch2Slot *slot = &ring->slot[packet->frame & 15u];
    if (!touch2_try_lock(slot)) return 0;
    slot->packet = *packet;
    for (unsigned hand = 0; hand < 2; ++hand)
        if (!slot->packet.tracked[hand]) for (unsigned axis = 0; axis < 3; ++axis)
            slot->packet.position[hand][axis] = 0;
    touch2_unlock(slot);
    return 1;
}

static int touch2_read(Touch2Ring *ring, unsigned long long epoch,
    unsigned long long frame, long long time, Touch2Packet *out) {
    if (!ring || !out || !touch2_identity(epoch, frame, time)) return 0;
    Touch2Slot *slot = &ring->slot[frame & 15u];
    if (!touch2_try_lock(slot)) return 0;
    Touch2Packet candidate = slot->packet;
    touch2_unlock(slot);
    if (candidate.epoch != epoch || candidate.frame != frame ||
        candidate.time != time || !touch2_packet_valid(&candidate)) return 0;
    *out = candidate;
    return 1;
}

/* Captured eye common terms only. No pose/nav/WorldDrag calls, eye offsets or
 * eye-midpoint/IPD correction. reference_head and navigation are already in
 * Unity coordinates. Caller must check packet.tracked[hand] before invoking.
 * Failure leaves the caller's output unchanged.
 */
static int touch2_transform(const Touch2Common *common,
    const float raw_xr_position[3], float out[3]) {
    if (!common || !raw_xr_position || !out ||
        !touch2_finite(common->head_scale) || common->head_scale <= 0 ||
        !touch2_finite(common->units) || common->units <= 0) return 0;
    for (unsigned axis = 0; axis < 3; ++axis)
        if (!touch2_finite(common->anchor[axis]) ||
            !touch2_finite(common->offset[axis]) ||
            !touch2_finite(common->reference_head[axis]) ||
            !touch2_finite(raw_xr_position[axis])) return 0;
    double norm = 0;
    for (unsigned i = 0; i < 4; ++i) {
        if (!touch2_finite(common->navigation[i])) return 0;
        norm += common->navigation[i] * common->navigation[i];
    }
    if (!touch2_finite(norm) || norm < 0.999999 || norm > 1.000001) return 0;
    double scale = common->head_scale * common->units;
    if (!touch2_finite(scale) || scale <= 0) return 0;

    double displacement[3] = {
        (double)raw_xr_position[0] - common->reference_head[0],
        (double)raw_xr_position[1] - common->reference_head[1],
        -(double)raw_xr_position[2] - common->reference_head[2]
    };
    const double *q = common->navigation;
    double t[3] = {
        2 * (q[1] * displacement[2] - q[2] * displacement[1]),
        2 * (q[2] * displacement[0] - q[0] * displacement[2]),
        2 * (q[0] * displacement[1] - q[1] * displacement[0])
    };
    double rotated[3] = {
        displacement[0] + q[3] * t[0] + q[1] * t[2] - q[2] * t[1],
        displacement[1] + q[3] * t[1] + q[2] * t[0] - q[0] * t[2],
        displacement[2] + q[3] * t[2] + q[0] * t[1] - q[1] * t[0]
    };
    float result[3];
    for (unsigned axis = 0; axis < 3; ++axis) {
        double value = common->anchor[axis] + common->offset[axis] +
            rotated[axis] * scale;
        if (!touch2_finite(value) || value > FLT_MAX || value < -FLT_MAX) return 0;
        result[axis] = (float)value;
    }
    for (unsigned axis = 0; axis < 3; ++axis) out[axis] = result[axis];
    return 1;
}

#endif
