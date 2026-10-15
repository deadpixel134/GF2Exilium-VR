#ifndef GF2_END_BOUNDARY_OBSERVE_H
#define GF2_END_BOUNDARY_OBSERVE_H
/* Diagnostic-only, sole XR owner; numeric summaries, no borrowed resources.
   Armed only by successful actual fresh publication, capped by reporting,
   and entirely absent from the product build. A cap never stops XR work. */
typedef unsigned long long EboU64;
typedef struct {
    EboU64 frequency,api_ticks,api_max_ticks,driver_ticks,api_calls,api_success;
    EboU64 attempts,pre_busy,post_busy,clock_invalid,pose_samples,projected_samples;
    unsigned armed,reports,motion_ppm,gap_ppm,context_mask;
    unsigned motion_peak_ppm,gap_peak_ppm,max_api_motion_ppm,max_api_gap_ppm,max_api_context_mask;
} EndBoundaryObservation;
enum { EBO_REPORT_CAP=12 };
extern EndBoundaryObservation end_boundary_observation;
int ebo_active(void);
EboU64 ebo_stamp(void);
void ebo_arm(void);
void ebo_api(EboU64 before,EboU64 after,int result);
void ebo_driver(EboU64 before,EboU64 after);
void ebo_busy(int post,int result);
void ebo_next_window(void);
#endif
