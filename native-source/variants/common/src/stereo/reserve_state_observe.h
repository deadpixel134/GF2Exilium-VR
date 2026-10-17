#ifndef GF2_RESERVE_STATE_OBSERVE_H
#define GF2_RESERVE_STATE_OBSERVE_H
/* Diagnostic-only array ABI. No owner layout, product transitions or exports.
   First rejection counts use the original predicate order under its existing lock.
   EMPTY/reclaim/fault/cancel/stale and RESERVED stages are subcounts, not new rules. */
#ifdef GF2VR_RESERVE_STATE_OBSERVE
enum {
    RSO_ATTEMPTS, RSO_INVALID, RSO_BUSY, RSO_LOCKED, RSO_OK, RSO_NO_ELIGIBLE,
    RSO_SERIAL_REJECT, RSO_EVENT_LIMIT, RSO_OTHER_REJECT,
    RSO_NOT_CURRENT, RSO_NOT_RENDERABLE, RSO_RESERVED, RSO_CLOSED,
    RSO_EMPTY, RSO_RECLAIMING, RSO_FAULT, RSO_CANCELLED, RSO_STALE,
    RSO_RESERVED_CPU, RSO_RESERVED_RENDER, RSO_RESERVED_PRE_COPY,
    RSO_RESERVED_COPY, RSO_RESERVED_LEASES, RSO_RESERVED_TRANSFERRED,
    RSO_RESERVED_TRANSFER, RSO_EMPTY_0, RSO_EMPTY_1, RSO_EMPTY_2, RSO_COUNT
};
void eye_reserve_observe_take(unsigned long long values[RSO_COUNT]);
#endif
#endif
