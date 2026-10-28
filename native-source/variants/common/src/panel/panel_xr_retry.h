#ifndef GF2VR_PANEL_XR_RETRY_H
#define GF2VR_PANEL_XR_RETRY_H
/* A session that already ran can still fail later: a frame/image/swapchain
   call error, a lost session or a failed event poll (for example under heavy
   load). That used to retire VR for the rest of the process. Retire the
   failed session completely, then try a new one a bounded number of times.
   A runtime/user exit request, shutdown, a changed game device and a
   quarantined session stay final, as does any failure of the very first run
   before a session exists. A retried session that stays up for the healthy
   window starts a fresh incident budget. */
#ifndef GF2VR_XR_RETRY_LIMIT
#define GF2VR_XR_RETRY_LIMIT 6u
#endif
#ifndef GF2VR_XR_RETRY_INTERVAL_MS
#define GF2VR_XR_RETRY_INTERVAL_MS 10000u
#endif
#ifndef GF2VR_XR_RETRY_HEALTHY_MS
#define GF2VR_XR_RETRY_HEALTHY_MS 60000u
#endif
typedef struct { unsigned disabled,attempts; U64 due; } PanelXrRetry;
static PanelXrRetry panel_xr_retry;
static BOOL panel_xr_retry_wait(const PanelXrRetry *r,U64 now) {
    return r->disabled || (r->attempts && now<r->due);
}
/* began: tick of the first successful BeginSession of the ended run, 0 if none. */
static void panel_xr_retry_end(PanelXrRetry *r,BOOL eligible,U64 began,U64 now,U64 frames) {
    if(began && now-began>=GF2VR_XR_RETRY_HEALTHY_MS) r->attempts=0;
    if(eligible && r->attempts<GF2VR_XR_RETRY_LIMIT) {
        ++r->attempts; r->due=now+GF2VR_XR_RETRY_INTERVAL_MS;
        event("xr_session_retry_scheduled",((U64)r->attempts<<32)|(frames&0xffffffffu),GetCurrentThreadId());
        return;
    }
    if(eligible) event("xr_session_retry_exhausted",r->attempts,GetCurrentThreadId());
    r->disabled=1;
}
/* Before a session exists: final on the first run, one used attempt on a retry. */
static void panel_xr_retry_setup_failed(PanelXrRetry *r,U64 now) {
    if(r->attempts) panel_xr_retry_end(r,1,0,now,0);
    else r->disabled=1;
}
#endif
