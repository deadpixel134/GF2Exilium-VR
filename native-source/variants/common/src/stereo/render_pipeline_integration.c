#ifdef GF2VR_TOUCH_PRIVATE
static void touch_p2_tick(unsigned long long);
static void touch_p2_fresh(void);
static void touch_p2_xr(long long,int);
static void touch_p2_mode(unsigned);
static void touch_p2_capture(void *,const void *);
static void touch_p2_lifetime(unsigned long long);
static void touch_p2_composed(const void *,const void *,const double *,const float *,double);
static void touch_p3_begin(void);
static void touch_p3_final(const void *,const void *,const void *,const void *,const double *,const double *);
static void touch_p2_prepare(void *,unsigned long long,unsigned long long);
static void touch_p2_draw(void *);
#endif
#line 1
/* Root/wrapper integration. Normal local build stays disabled; the explicit
   MAIN-STEREO candidate enables the approved main-only manual route. */
#ifdef GF2VR_ACTIVITY_ROLES_OBSERVE
#ifndef GF2VR_ACTIVITY_ADMISSION_OBSERVE
#define GF2VR_ACTIVITY_ADMISSION_OBSERVE
#endif
#define GF2VR_MAIN_PHASE_ROLES
#define GF2VR_MAIN_PHASE_DISPATCH
#endif
#ifdef GF2VR_ACTIVITY_ADMISSION_OBSERVE
/* Existing owner-thread scalar stages and copied topology only. No extra
   engine, graphics or XR query; independent of all production decisions. */
#define GF2VR_ADMISSION_OBSERVE
#define GF2VR_MAIN_PHASE_OBSERVE
#endif
#ifdef GF2VR_BANK_THROUGHPUT_OBSERVE
/* Same selected bank behavior; capped main/XR-only existing-boundary QPC. */
#define GF2VR_ASYNC_THROUGHPUT_OBSERVE
#endif
#ifdef GF2VR_STEREO_DEPTH
#ifdef GF2VR_WORLD_SCALE
static double stereo_world_head_scale(void);
#endif
static int stereo_depth_pose(const void *,const void *,const void *,const double [2][3],const double *,const float *,void *);
static void stereo_depth_applied(const void *,const void *,const void *,const void *);
#define GF2VR_STEREO_DEPTH_POSE(m,i,a,e,t,n,p) stereo_depth_pose(m,i,a,e,t,n,p)
#define GF2VR_STEREO_DEPTH_APPLIED(i,a,p,t) stereo_depth_applied(i,a,p,t)
#endif
#ifdef GF2VR_ACTIVITY_PRIMARY_UI
#ifdef GF2VR_ACTIVITY_DIALOGUE_RAY
static void dialogue_ray_capture(const void *,const void *,const void *,const void *,const float *,const void *,unsigned);
#define GF2VR_ACTIVITY_UI_RAY_CAPTURE(g,m,i,t,x,p,s) dialogue_ray_capture(g,m,i,t,x,p,s)
#ifdef GF2VR_ACTIVITY_UI_LIFETIME
static void dialogue_ray_invalidate(void);
#define GF2VR_ACTIVITY_UI_INVALIDATE(a) ((void)(a),dialogue_ray_invalidate())
#endif
#endif
#ifdef GF2VR_ACTIVITY_UI_DIALOGUE_OWNER_OBSERVE
static void activity_ui_dialogue_owner(void *,const void *,unsigned,const void *);
#define GF2VR_ACTIVITY_UI_DIALOGUE_OWNER(a,e,n,r) activity_ui_dialogue_owner(a,e,n,r)
#endif
static int primary_ui_capture(void *,void *,const void *,const void *);
#ifdef GF2VR_ACTIVITY_DIALOGUE_UI
static int dialogue_ui_capture(void *,void *,const void *,const void *);
static int dialogue_ui_geometry(void *,unsigned long long,float [20]);
static int primary_ui_geometry(void *,unsigned long long,float [20]);
#define GF2VR_ACTIVITY_DIALOGUE_CAPTURE(a,s,e,r) dialogue_ui_capture(a,s,e,r)
#define GF2VR_ACTIVITY_DIALOGUE_GEOMETRY(a,g,o) dialogue_ui_geometry(a,g,o)
#define GF2VR_ACTIVITY_PRIMARY_GEOMETRY(a,g,o) primary_ui_geometry(a,g,o)
#ifdef GF2VR_ACTIVITY_UI_ISOLATION_OBSERVE
static void activity_ui_isolation_observe(void *,const void *,unsigned,unsigned,const float *,const float *,const void *,const void *,const void *,const void *);
#define GF2VR_ACTIVITY_UI_ISOLATION_REPORT(g,t,a,p,x,y,v0,r0,v1,r1) activity_ui_isolation_observe(g,t,a,p,x,y,v0,r0,v1,r1)
#endif
#endif
static int primary_ui_current(void *);static int primary_ui_visible(void);
static void primary_ui_aux(unsigned);
static void primary_ui_report(unsigned long long,unsigned long long,unsigned,float);
#define GF2VR_ACTIVITY_UI_CAPTURE(a,s,e,r) primary_ui_capture(a,s,e,r)
#define GF2VR_ACTIVITY_UI_CURRENT(a) primary_ui_current(a)
#define GF2VR_ACTIVITY_UI_VISIBLE() primary_ui_visible()
#define GF2VR_ACTIVITY_UI_REPORT(g,f,s,p) primary_ui_report(g,f,s,p)
#define GF2VR_ACTIVITY_UI_AUX(shown) primary_ui_aux(shown)
#ifdef GF2VR_ACTIVITY_UI_GENERATION_LEASE
static int primary_ui_generation_prepare(void *,unsigned long long);
static int primary_ui_generation_release(void *);
#define GF2VR_ACTIVITY_UI_GENERATION_PREPARE(a,g) primary_ui_generation_prepare(a,g)
#define GF2VR_ACTIVITY_UI_GENERATION_RELEASE(a) primary_ui_generation_release(a)
#ifdef GF2VR_ACTIVITY_UI_LIFETIME
static int primary_ui_transition_release(void *);
#define GF2VR_ACTIVITY_UI_TRANSITION_RELEASE(a) primary_ui_transition_release(a)
#endif
#endif
#endif
#ifdef GF2VR_ACTIVITY_UI_DRAW_OBSERVE
static void ui_draw_prepare(unsigned long long);
static void ui_draw_return(unsigned long long);
static void ui_draw_owner_tick(void);
#define GF2VR_UI_DRAW_PREPARE(s) ui_draw_prepare(s)
#define GF2VR_UI_DRAW_RETURN(s) ui_draw_return(s)
#endif
static void panel_main_tick(unsigned long long n);
#ifdef GF2VR_NAVIGATION_BLOCK_OBSERVE
static void navigation_block_applied(const void *,const void *,const void *,const void *);
#define GF2VR_NAVIGATION_BLOCK_APPLIED(i,a,p,t) navigation_block_applied(i,a,p,t)
#endif
static void stereo_local_tick(unsigned long long n);
#ifdef GF2VR_ACTIVITY_INPUT_BOUNDARY_OBSERVE
static void activity_input_boundary_report(const void *,unsigned,unsigned,unsigned,unsigned,unsigned,void *const *);
#define GF2VR_TOPOLOGY_INPUT_REPORT(s,r,n,v,i,j,c) activity_input_boundary_report(s,r,n,v,i,j,c)
#endif
#ifdef GF2VR_ACTIVITY_VIEWPORT
static void activity_viewport_report(unsigned long long,unsigned,unsigned,const float *,const float *,const float *);
#define GF2VR_ACTIVITY_VIEWPORT_REPORT(g,m,e,b,o,a) activity_viewport_report(g,m,e,b,o,a)
#endif
#ifdef GF2VR_ACTIVITY_SCALE_PREPARE_OBSERVE
static void activity_prepare_boundary_report(unsigned long long,unsigned,unsigned,unsigned,int,int,unsigned);
static void activity_viewport_boundary_report(void *,void *,unsigned,unsigned,const void *,const void *,const void *);
#define GF2VR_ACTIVITY_PREPARE_BOUNDARY_REPORT(g,n,m,s,w,h,o) activity_prepare_boundary_report(g,n,m,s,w,h,o)
#define GF2VR_ACTIVITY_VIEWPORT_BOUNDARY_REPORT(g,c,a,p,b,o,r) activity_viewport_boundary_report(g,c,a,p,b,o,r)
#endif
static void panel_ready_run(void *state,void *module);
static void stereo_root_begin(unsigned long long call,int sample);
static void stereo_root_end(int failed);
#ifdef GF2VR_THUMBSTICK_NAVIGATION
static int stereo_navigation_pose(const void *,const void *,const void *,double,const double *,double,double *,float *);
#define GF2VR_NAVIGATION_POSE(m,i,a,y,d,r,o,p) stereo_navigation_pose(m,i,a,y,d,r,o,p)
#endif
#ifdef GF2VR_CAMERA_PLACEMENT_OBSERVE
static void placement_source(const void *,unsigned long long,const void *,const void *);
static void placement_compose(const void *,const void *,const double *,const float *,const double *,const double *,const void *,double,double,double);
static void placement_applied(const void *,int);
#define GF2VR_PLACEMENT_SOURCE(s,g,p,a) placement_source(s,g,p,a)
#define GF2VR_PLACEMENT_COMPOSE(i,a,n,o,d,t,p,y,x,r) placement_compose(i,a,n,o,d,t,p,y,x,r)
#define GF2VR_PLACEMENT_APPLIED(t,ok) placement_applied(t,ok)
#endif
#ifdef GF2VR_PRODUCER_SCOPE_OBSERVE
static void producer_scope_submit(int,void *,int);
static void producer_scope_finish(unsigned long long,int);
static void producer_scope_foreign(void);
#ifdef GF2VR_PRODUCER_FIRST_FAULT_OBSERVE
static void producer_scope_foreign_trace(const char *,unsigned);
#endif
#endif
static void stereo_output_render_tick(void);
#ifdef GF2VR_PERF_COMPLETION_SERVICE
static void stereo_output_render_tail(void);
#endif
static void stereo_output_root_finished(unsigned long long serial);
static void stereo_output_owner_tick(void);
static void stereo_output_xr_frame(void *panel,unsigned long long frame);
static void stereo_output_xr_invalidate(void *panel);
static int stereo_output_xr_close(void *panel);
static int stereo_output_stop_session(void *panel);
static int stereo_output_session_stopped(void *panel);
static int stereo_frame_abort_required(void *panel);
static void stereo_frame_abort_input(void);
static void stereo_output_xr_open(void *state,void *module);
#ifdef GF2VR_CONTROLLER_INPUT
static unsigned long long stereo_publication_generation;
#endif
int _fltused;
#ifdef GF2VR_PERF_PREPARE_RECOVERY
static void stereo_refresh_failure(void *,unsigned);
static void stereo_preparation_reconciled(void *);
#define GF2VR_REFRESH_FAILURE(o,phase) stereo_refresh_failure(o,phase)
#define GF2VR_PREPARATION_RECONCILED(p) stereo_preparation_reconciled(p)
#endif
#ifdef GF2VR_PERF_REVOKE_OBSERVE
static unsigned revoke_admission_stage,revoke_prepare_stage;
static void stereo_revoke_trace(void *,unsigned);
static void stereo_clear_trace(void *);
#define GF2VR_MAIN_REVOKE_TRACE(g,line) stereo_revoke_trace(g,line)
#define GF2VR_SOURCE_CLEAR_TRACE(s) stereo_clear_trace(s)
#define GF2VR_ADMISSION_STEP(n) ((void)(revoke_admission_stage=(n)))
#define GF2VR_PREPARE_STEP(n) (revoke_prepare_stage=(n),1)
#endif
#ifdef GF2VR_SOURCE_RETIRE_OBSERVE
static void sr_begin(void);
static void sr_failure(unsigned,unsigned);
static void sr_slot_before(void *,unsigned);
static void sr_slot_after(void *,unsigned,int,int);
static void sr_legacy_before(void *);
static void sr_end(void *,int);
static void sr_guard(void *,unsigned long);
#define GF2VR_RETIRE_BEGIN() sr_begin()
#define GF2VR_RETIRE_FAILURE(reason,eye) sr_failure(reason,eye)
#define GF2VR_RETIRE_END(source,clean) sr_end(source,clean)
#define GF2VR_SOURCE_RETIRE_SLOT_BEFORE(pool,slot) sr_slot_before(pool,slot)
#define GF2VR_SOURCE_RETIRE_SLOT_AFTER(pool,slot,eyes,cache) sr_slot_after(pool,slot,eyes,cache)
#define GF2VR_SOURCE_RETIRE_LEGACY(source) sr_legacy_before(source)
#define GF2VR_SOURCE_RETIRE_GUARD(source,thread) sr_guard(source,thread)
#endif
#ifdef GF2VR_COPY_GPU_OBSERVE
#include "copy_gpu_observe.h"
#endif
#ifdef GF2VR_COPY_PRESENT_OBSERVE
#include "copy_present_observe.h"
#endif
#ifdef GF2VR_PERF_OBSERVE
static void stereo_pacing_record(unsigned long long start,long long period,unsigned long long wait,
    unsigned long long copy,unsigned long long end,int render);
#define GF2VR_PACING_RECORD(start,period,wait,copy,end,render) stereo_pacing_record(start,period,wait,copy,end,render)
static unsigned long long stereo_pacing_fresh;
#ifdef GF2VR_PERF_END_SPLIT
/* XR owner only: distinguish producer wait from the runtime EndFrame call. */
static unsigned long long stereo_pacing_eye_wait_ms,stereo_pacing_runtime_end_ms;
#endif
#endif
#ifdef GF2VR_MAIN_TRANSITION_HOLD
static unsigned long long stereo_handoff_from;
static void stereo_hold_session_state(int state);
#define GF2VR_OUTPUT_SESSION_STATE(state) stereo_hold_session_state(state)
#define GF2VR_HANDOFF_BEGIN(g) __atomic_store_n(&stereo_handoff_from,(g),__ATOMIC_RELEASE)
#ifdef GF2VR_PRESENTATION_CACHE
/* Main publishes cancellation only; the completed payload remains XR-owned.
   This catches off/on or invalidation even between two XR observations. */
static unsigned long long stereo_completed_cancel_clock;
#define GF2VR_HANDOFF_CANCEL() do { \
    __atomic_store_n(&stereo_handoff_from,0,__ATOMIC_RELEASE); \
    __atomic_add_fetch(&stereo_completed_cancel_clock,1,__ATOMIC_ACQ_REL); \
} while(0)
#else
#define GF2VR_HANDOFF_CANCEL() __atomic_store_n(&stereo_handoff_from,0,__ATOMIC_RELEASE)
#endif
#endif
#ifdef GF2VR_RETIRE_OBSERVE
#include "retirement_observe_state.h"
#endif
#ifdef GF2VR_FOCUS_CORRECT
static void focus_projection_optics(unsigned long long,const void *,const void *);
#endif
#ifdef GF2VR_ORIENTATION_OBSERVE
static void orientation_optics(unsigned long long,const void *,const void *,const void *);
static void orientation_pose(const void *,const void *);
#define GF2VR_OPTICS_OBSERVE(f,p,v,m) orientation_optics(f,p,v,m)
#define GF2VR_POSE_OBSERVE(p,a) orientation_pose(p,a)
#endif
#ifdef GF2VR_FOCUS_CORRECT
#undef GF2VR_OPTICS_OBSERVE
#if defined(GF2VR_NUMERIC_RESIDENCY) && !defined(GF2VR_ORIENTATION_OBSERVE)
#define GF2VR_OPTICS_OBSERVE(f,p,v,m) focus_projection_optics(f,v,m)
#else
#define GF2VR_OPTICS_OBSERVE(f,p,v,m) do { orientation_optics(f,p,v,m);focus_projection_optics(f,v,m); } while(0)
#endif
#endif
#ifdef GF2VR_PREPARE_OBSERVE
static unsigned prepare_active,prepare_stage,prepare_samples;
static unsigned prepare_last_valid;
static unsigned long long prepare_last_code;
static int prepare_step(unsigned stage) {if(prepare_active) prepare_stage=stage;return 1;}
#define GF2VR_PREPARE_STEP(n) prepare_step(n)
#endif
#ifdef GF2VR_TWO_FLIGHT_ARM_OBSERVE
/* Main-thread-only nested preparation stage; no new Unity/XR/GPU query. */
static unsigned two_arm_nested_stage;
#ifndef GF2VR_PREPARE_STEP
static int two_arm_prepare_step(unsigned stage) {two_arm_nested_stage=stage;return 1;}
#define GF2VR_PREPARE_STEP(n) two_arm_prepare_step(n)
#endif
#endif
#ifdef GF2VR_ADMISSION_OBSERVE
static unsigned admission_active,admission_stage;
#ifdef BOUNDARY_TEST
static unsigned admission_test_relation;
#endif
static void admission_step(unsigned stage) {if(admission_active) admission_stage=stage;}
static void admission_transform(unsigned root,void *actual,void *expected);
#define GF2VR_ADMISSION_STEP(n) admission_step(n)
#define GF2VR_ADMISSION_TRANSFORM(root,actual,expected) admission_transform(root,actual,expected)
#endif
#ifdef GF2VR_MAIN_STEREO
#if defined(GF2VR_BANK_GENERATION_RECOVERY)
#define GF2VR_OBSERVER_ID "MAIN-BANK-GENERATION-RECOVERY-001"
#elif defined(GF2VR_MAIN_HANDOFF_OBSERVE)
#define GF2VR_OBSERVER_ID "MAIN-PRODUCTION-BOUNDARY-OBS-001"
#elif defined(GF2VR_BANK_EYE_WRITE_OBSERVE)
#define GF2VR_OBSERVER_ID "PERF-VERSIONDLL-EYEWRITE-OBS-001"
#elif defined(GF2VR_BANK_GPU_OBSERVE)
#define GF2VR_OBSERVER_ID "PERF-VERSIONDLL-GPU-OBS-001"
#elif defined(GF2VR_BANK_NATIVE_OBSERVE)
#define GF2VR_OBSERVER_ID "PERF-BANK-NATIVE-OBS-001"
#elif defined(GF2VR_BANK_CPU_OBSERVE)
#define GF2VR_OBSERVER_ID "PERF-BANK-CPU-OBS-001"
#elif defined(GF2VR_BANK_CYCLE_OBSERVE)
#define GF2VR_OBSERVER_ID "PERF-BANK-CYCLE-OBS-001"
#elif defined(GF2VR_BANK_WAIT_AFTER_END)
#define GF2VR_OBSERVER_ID "PERF-BANK-WAIT-AFTER-END-001"
#elif defined(GF2VR_BANK_END_OBSERVE)
#define GF2VR_OBSERVER_ID "PERF-BANK-END-OBS-001"
#elif defined(GF2VR_BANK_PARTIAL_READMIT)
#define GF2VR_OBSERVER_ID "PERF-BANK-PARTIAL-READMIT-001"
#elif defined(GF2VR_BANK_READMIT_OBSERVE)
#define GF2VR_OBSERVER_ID "PERF-BANK-READMIT-OBS-002"
#elif defined(GF2VR_BANK_READMIT)
#define GF2VR_OBSERVER_ID "PERF-BANK-READMIT-001"
#elif defined(GF2VR_BANK_TRANSFER_OBSERVE)
#define GF2VR_OBSERVER_ID "PERF-TRANSFER-THROUGHPUT-OBS-001"
#elif defined(GF2VR_ACTIVITY_ROLES_OBSERVE)
#define GF2VR_OBSERVER_ID "ACTIVITY-ROLES-OBS-001"
#elif defined(GF2VR_ACTIVITY_ADMISSION_OBSERVE)
#define GF2VR_OBSERVER_ID "ACTIVITY-ADMISSION-OBS-001"
#elif defined(GF2VR_BANK_TRANSFER)
#define GF2VR_OBSERVER_ID "PERF-BANK-TRANSFER-AUTO-001"
#elif defined(GF2VR_AUTO_STEREO)
#define GF2VR_OBSERVER_ID "AUTO-STEREO-001"
#elif defined(GF2VR_BANK_ADMISSION_OBSERVE)
#define GF2VR_OBSERVER_ID "PERF-BANK-ADMISSION-OBS-001"
#elif defined(GF2VR_BANK_COPY_AFTER_END)
#define GF2VR_OBSERVER_ID "PERF-BANK-AFTER-END-001"
#elif defined(GF2VR_BANK_DISPATCH_OBSERVE)
#define GF2VR_OBSERVER_ID "PERF-BANK-DISPATCH-OBS-001"
#elif defined(GF2VR_BANK_COPY_SERVICE)
#define GF2VR_OBSERVER_ID "PERF-BANK-COPY-SERVICE-001"
#elif defined(GF2VR_BANK_FENCE_OBSERVE)
#define GF2VR_OBSERVER_ID "PERF-BANK-FENCE-OBS-001"
#elif defined(GF2VR_BANK_FENCE_COMPLETION)
#define GF2VR_OBSERVER_ID "PERF-BANK-FENCE-001"
#elif defined(GF2VR_BANK_THROUGHPUT_OBSERVE)
#define GF2VR_OBSERVER_ID "PERF-BANK-THROUGHPUT-OBS-001"
#elif defined(GF2VR_BANK_PRESENTATION)
#define GF2VR_OBSERVER_ID "PERF-BANK-PRESENTATION-001"
#elif defined(GF2VR_TWO_FLIGHT_ROOT_OBSERVE)
#define GF2VR_OBSERVER_ID "PERF-TWO-FLIGHT-ROOT-OBS-004"
#elif defined(GF2VR_TWO_FLIGHT_PREFETCH_OBSERVE)
#define GF2VR_OBSERVER_ID "PERF-TWO-FLIGHT-PREFETCH-OBS-003"
#elif defined(GF2VR_PERF_ROOT_FENCE_ORDER)
#define GF2VR_OBSERVER_ID "PERF-ROOT-FENCE-ORDER-001"
#elif defined(GF2VR_PERF_FENCE_TIMING)
#define GF2VR_OBSERVER_ID "PERF-FENCE-TIMING-001"
#elif defined(GF2VR_PERF_ROOT_CADENCE)
#define GF2VR_OBSERVER_ID "PERF-ROOT-CADENCE-001"
#elif defined(GF2VR_PERF_REFRESH_VALIDITY)
#define GF2VR_OBSERVER_ID "PERF-REFRESH-VALIDITY-001"
#elif defined(GF2VR_PERF_PREPARE_RECOVERY)
#define GF2VR_OBSERVER_ID "PERF-PREPARE-RECOVERY-001"
#elif defined(GF2VR_PERF_REVOKE_OBSERVE)
#define GF2VR_OBSERVER_ID "PERF-REVOKE-OBS-001"
#elif defined(GF2VR_PERF_FENCE_DELIVERY)
#define GF2VR_OBSERVER_ID "PERF-FENCE-DELIVERY-001"
#elif defined(GF2VR_PERF_FENCE_OBSERVE)
#define GF2VR_OBSERVER_ID "PERF-FENCE-OBS-001"
#elif defined(GF2VR_ENTRY_BOOTSTRAP)
#define GF2VR_OBSERVER_ID "ENTRY-BOOTSTRAP-001"
#elif defined(GF2VR_PERF_SERVICE_OBSERVE)
#define GF2VR_OBSERVER_ID "PERF-SERVICE-OBS-001"
#elif defined(GF2VR_PERF_COMPLETION_SERVICE)
#define GF2VR_OBSERVER_ID "PERF-COMPLETION-SERVICE-001"
#elif defined(GF2VR_PERF_PRODUCER_OBSERVE)
#define GF2VR_OBSERVER_ID "PERF-PRODUCER-OBS-001"
#elif defined(GF2VR_PERF_END_SPLIT)
#define GF2VR_OBSERVER_ID "PERF-END-SPLIT-001"
#elif defined(GF2VR_PERF_ASYNC_EYE_WAIT)
#define GF2VR_OBSERVER_ID "PERF-ASYNC-PANEL-002"
#elif defined(GF2VR_PERF_ASYNC_PANEL)
#define GF2VR_OBSERVER_ID "PERF-ASYNC-PANEL-001"
#elif defined(GF2VR_PERF_BOUNDARY)
#define GF2VR_OBSERVER_ID "PERF-PRESENT-BOUNDARY-001"
#elif defined(GF2VR_PERF_POLL)
#define GF2VR_OBSERVER_ID "PERF-PRESENT-POLL-001"
#elif defined(GF2VR_PERF_OBSERVE)
#define GF2VR_OBSERVER_ID "PERF-PACING-OBS-001"
#elif defined(GF2VR_HOLD_300)
#define GF2VR_OBSERVER_ID "MAIN-HOLD-300-002"
#elif defined(GF2VR_FACE_SWAP)
#define GF2VR_OBSERVER_ID "CONTROLLER-FACE-003"
#elif defined(GF2VR_CLICK_STABILITY)
#define GF2VR_OBSERVER_ID "CONTROLLER-CLICK-002"
#elif defined(GF2VR_CONTROLLER_INPUT)
#define GF2VR_OBSERVER_ID "CONTROLLER-PANEL-001"
#elif defined(GF2VR_WORLD_DRAG)
#define GF2VR_OBSERVER_ID "WORLD-DRAG-001"
#elif defined(GF2VR_COMPOSITION_OBSERVE)
#define GF2VR_OBSERVER_ID "MAIN-COMPOSITION-BIND-CLOSEOUT-001"
#elif defined(GF2VR_AUX_OBSERVE)
#define GF2VR_OBSERVER_ID "MAIN-PANEL-OBS-001"
#elif defined(GF2VR_AUX_PANEL)
#define GF2VR_OBSERVER_ID "MAIN-PANEL-001"
#elif defined(GF2VR_FOCUS_CORRECT)
#define GF2VR_OBSERVER_ID "FOCUS-FIX-001"
#elif defined(GF2VR_FOCUS_MRT)
#define GF2VR_OBSERVER_ID "FOCUS-OBS-004"
#elif defined(GF2VR_FOCUS_LINEAGE_OBSERVE)
#define GF2VR_OBSERVER_ID "FOCUS-OBS-003"
#elif defined(GF2VR_FOCUS_GPU_OBSERVE)
#define GF2VR_OBSERVER_ID "FOCUS-OBS-002"
#elif defined(GF2VR_ORIENTATION_OBSERVE)
#define GF2VR_OBSERVER_ID "ORIENTATION-OBS-002"
#elif defined(GF2VR_PREPARE_OBSERVE)
#define GF2VR_OBSERVER_ID "SOURCE-PREPARE-013"
#elif defined(GF2VR_ADMISSION_OBSERVE)
#define GF2VR_OBSERVER_ID "SOURCE-ADMISSION-002"
#else
#ifdef GF2VR_INITIAL_CALIBRATION
#define GF2VR_OBSERVER_ID "POSE-INITIAL-001"
#else
#define GF2VR_OBSERVER_ID "MAIN-STEREO-009"
#endif
#endif
#define GF2VR_RENDER_BIND_PERSISTENT 1
#elif defined(GF2VR_DRAW_ORDER_OBSERVE)
#define GF2VR_OBSERVER_ID "DRAW-ORDER-002"
#elif defined(GF2VR_SUBMISSION_OBSERVE)
#define GF2VR_OBSERVER_ID "SUBMIT-COMMANDS-001"
#else
#define GF2VR_OBSERVER_ID "LOCAL-RENDER-INTEGRATION"
#endif
#define GF2VR_MAIN_TICK(n) stereo_local_tick(n)
#define GF2VR_READY_RUN(s,module) panel_ready_run(s,module)
#define GF2VR_RENDER_SCOPE_BEGIN(c,s) stereo_root_begin(c,s)
#define GF2VR_RENDER_SCOPE_END(f) stereo_root_end(f)
#define GF2VR_RENDER_OWNER_TICK() stereo_output_render_tick()
#ifdef GF2VR_PERF_COMPLETION_SERVICE
#define GF2VR_RENDER_OWNER_TAIL() stereo_output_render_tail()
#endif
#define GF2VR_RENDER_SOURCE_SNAPSHOT() ((void)0)
#include "../bootstrap/xr_bootstrap.c"
#include "../panel/panel_capture.h"
#include "panel_eye_bridge.h"
#ifdef GF2VR_CONTROLLER_HAND_ROLES
static unsigned stereo_panel_role_hand(void);
#endif
#ifdef GF2VR_CONTROLLER_GAME_MODE
static unsigned controller_game_drag_consumed(void);
#endif
#ifdef GF2VR_WORLD_DRAG
#ifdef GF2VR_CAMERA_RETURN
#include "camera_return_handoff.h"
static CameraReturnHandoff stereo_camera_return;
#ifdef GF2VR_CAMERA_FOLLOW_TOGGLE
#include "camera_follow_handoff.h"
static CameraFollowHandoff stereo_camera_follow;
static int stereo_camera_follow_prepare(const void *,unsigned long long);
#endif
static int stereo_camera_return_end(int,unsigned long long,unsigned long long,unsigned long long,unsigned long long);
static int stereo_camera_return_pose(const void *,const void *,const void *,const double *,double,double,const void *,const void *,unsigned long long);
#endif
#include "world_drag_runtime.h"
#endif
#ifdef GF2VR_AUX_PANEL
#include "aux_panel.h"
#ifdef GF2VR_CONTROLLER_HAND_ROLES
static unsigned stereo_panel_role_hand(void) {return stereo_aux.panel_hand;}
#endif
static int stereo_panel_wanted(void *panel,const XrFrameState *frame);
#define GF2VR_PANEL_WANTED(panel,frame) stereo_panel_wanted(panel,frame)
#ifdef GF2VR_WORLD_DRAG
#define GF2VR_PANEL_CLOSE() do {while(!aux_close(&stereo_aux))Sleep(10);}while(0)
#else
#define GF2VR_PANEL_CLOSE() aux_close(&stereo_aux)
#endif
#endif
static XrResult stereo_output_xr_end(void *panel,const XrFrameEndInfo *end,int visible);
#ifdef GF2VR_PERF_FENCE_DELIVERY
static void stereo_output_fence_tick(void);
#endif
#define GF2VR_OUTPUT_FRAME(panel,number) stereo_output_xr_frame(panel,number)
#define GF2VR_OUTPUT_INVALIDATE(panel) stereo_output_xr_invalidate(panel)
#define GF2VR_OUTPUT_CLOSE(panel) stereo_output_xr_close(panel)
#define GF2VR_OUTPUT_STOP_SESSION(panel) stereo_output_stop_session(panel)
#define GF2VR_OUTPUT_SESSION_STOPPED(panel) stereo_output_session_stopped(panel)
#define GF2VR_FRAME_ABORT_REQUIRED(panel) stereo_frame_abort_required(panel)
#define GF2VR_FRAME_ABORT_INPUT() stereo_frame_abort_input()
#define GF2VR_OUTPUT_OPEN(state,module) stereo_output_xr_open(state,module)
#define GF2VR_OUTPUT_END_FRAME(panel,end,visible) stereo_output_xr_end(panel,end,visible)
static XrResult stereo_frame_wait(void *,const XrFrameWaitInfo *,XrFrameState *);
static XrResult stereo_frame_begin(void *,const XrFrameState *,int);
#define GF2VR_FRAME_WAIT(panel,wait,frame) stereo_frame_wait(panel,wait,frame)
#define GF2VR_FRAME_BEGIN(panel,frame,exiting) stereo_frame_begin(panel,frame,exiting)
#include "../panel/panel_xr.h"
#ifdef GF2VR_CONTROLLER_INPUT
#ifdef GF2VR_CONTROLLER_GAME_MODE
static unsigned controller_game_move_hand,controller_game_consumed;
static int controller_game_runtime_busy(void);
static void controller_game_runtime_end(XrResult);
static void controller_game_runtime_cancel(void);
static int controller_game_runtime_close(void);
#endif
#include "controller_runtime.h"
#ifdef GF2VR_CONTROLLER_GAME_MODE
#include "controller_game_runtime.h"
#endif
#endif
#include "xr_eye_output.h"
#include "xr_eye_config.h"
#include "xr_eye_size.h"
#ifdef GF2VR_RENDER_SCALE
#include "render_scale_runtime.h"
#endif
#line 325
#include "numeric_loss_observe.h"
#include "xr_eye_frame_driver.h"
#include "xr_eye_flight_images.h"
#ifdef GF2VR_BANK_PRESENTATION
#include "xr_presentation_config.h"
static XrPresentationFlights stereo_banks;
static XrBankPresentation stereo_bank_cache;
#ifdef GF2VR_READY_RECEIPT_READMIT
#include "xr_ready_readmit.h"
#endif
#line 331
#ifdef GF2VR_BANK_READMIT
static unsigned stereo_hold_focused;
#endif
#ifdef BOUNDARY_TEST
static unsigned stereo_bank_enabled;
#else
static unsigned stereo_bank_enabled=1;
#endif
static XrResult stereo_bank_end(PanelXr *,const XrFrameEndInfo *,int);
static int stereo_bank_drain(PanelXr *,int);
static int stereo_bank_close(PanelXr *);
static int stereo_bank_stop_session(PanelXr *);
static int stereo_bank_session_stopped(PanelXr *);
#ifdef GF2VR_BANK_COPY_SERVICE
static int stereo_bank_copy_service(PanelXr *);
#ifdef BOUNDARY_TEST
static unsigned bank_service_test_gate,bank_service_test_waiting,bank_service_test_posted;
#endif
#endif
#endif
#include "source_observer.h"
#ifdef GF2VR_PERF_FENCE_TIMING
__declspec(dllimport) BOOL WINAPI QueryPerformanceFrequency(long long *);
#ifdef GF2VR_PERF_ROOT_FENCE_ORDER
__declspec(dllimport) BOOL WINAPI QueryPerformanceCounter(long long *);
#endif
/* Written only by the main owner after interval DONE/take; read/reset by that
   same owner at the bounded root report. Raw QPC ticks retain precision. */
static struct {
    U64 count,fallback,sum_complete,min_complete,max_complete;
    U64 bracket_count,sum_bracket,min_bracket,max_bracket,polls,pending;
#ifdef GF2VR_PERF_ROOT_FENCE_ORDER
    U64 root_serial,root_at,root_count,root_pending_after,root_inside_bracket;
    U64 root_complete_before,root_unmeasured;
#endif
} perf_fence_timing;
#ifdef GF2VR_PERF_ROOT_FENCE_ORDER
/* Numeric-only same-ticket classification after DONE. A pending fence poll
   following the blocked root proves the GPU had not completed at that root.
   A root inside two polls is deliberately indeterminate. */
static unsigned perf_root_fence_class(U64 root,U64 last_pending,U64 first_complete) {
    if(!root || !last_pending || !first_complete || last_pending>=first_complete) return 0;
    if(root<last_pending) return 1;
    if(root<=first_complete) return 2;
    return 3;
}
#ifdef BOUNDARY_TEST
__declspec(dllexport) int perf_root_fence_order_test(void) {
    if(perf_root_fence_class(0,10,20)!=0 || perf_root_fence_class(11,0,20)!=0 ||
       perf_root_fence_class(11,20,20)!=0 || perf_root_fence_class(9,10,20)!=1 ||
       perf_root_fence_class(10,10,20)!=2 || perf_root_fence_class(11,10,20)!=2 ||
       perf_root_fence_class(20,10,20)!=2 || perf_root_fence_class(21,10,20)!=3) return __LINE__;
    return 0;
}
#endif
#endif
#endif
#ifdef GF2VR_PERF_PRODUCER_OBSERVE
/* No diagnostic timestamp authorizes presentation or release. Render storage
   stays render-owned; XR storage crosses only the existing DONE publication. */
static struct {
    EyeGpuTicket ticket;
    U64 begin,copy,first_poll,last_pending,polls,pending,ready,copied,report;
} producer_render;
static unsigned producer_render_records,producer_xr_records,producer_owner_records;
static void producer_record(const char *name,EyeGpuTicket ticket,U64 frame,
                            const char **keys,const U64 *values,unsigned count) {
    DWORD saved=GetLastError();long long frequency=0;
    if(count>8 || !QueryPerformanceFrequency(&frequency) || frequency<=0) {SetLastError(saved);return;}
    char line[1600],*p=source_prefix(line,name);
    p=literal(p,",\"serial\":");p=decimal(p,ticket.serial);
    p=literal(p,",\"generation\":");p=decimal(p,ticket.generation);
    p=literal(p,",\"epoch\":");p=decimal(p,ticket.epoch);
    p=literal(p,",\"frame\":");p=decimal(p,frame);
    p=literal(p,",\"thread\":");p=decimal(p,GetCurrentThreadId());
    p=literal(p,",\"qpc_frequency\":");p=decimal(p,(U64)frequency);
    for(unsigned i=0;i<count;++i) {
        p=literal(p,",\"");p=literal(p,keys[i]);p=literal(p,"\":");p=decimal(p,values[i]);
    }
    p=literal(p,"}\n");source_log(line,p);SetLastError(saved);
}
#ifdef BOUNDARY_TEST
__declspec(dllexport) int producer_timing_test(void) {
    if(producer_sample(0) || !producer_sample(1) || !producer_sample(7) || producer_sample(8) ||
       producer_sample(99) || !producer_sample(100) || !producer_sample(107) || producer_sample(108))return __LINE__;
    SetLastError(731);U64 first=producer_clock(),last=producer_clock();
    if(!first || last<first || GetLastError()!=731)return __LINE__;
    const char *keys[]={"first","last"};U64 values[]={first,last};
    producer_record("perf_producer_fixture",(EyeGpuTicket){101,3,9},11,keys,values,2);
    producer_record("perf_producer_rejected",(EyeGpuTicket){101,3,9},11,keys,values,9);
    return GetLastError()==731?0:__LINE__;
}
#endif
#endif
#include "render_observer.h"
#ifdef GF2VR_ADMISSION_OBSERVE
static void admission_transform(unsigned root,void *actual,void *expected) {
    if(!admission_active) return;
#if defined(GF2VR_ACTIVITY_ADMISSION_OBSERVE) && !defined(BOUNDARY_TEST)
    /* Reuse the failed guard's stage. Do not add ancestry queries merely for
       diagnostic classification; the existing reader already validated it. */
    (void)root;(void)actual;(void)expected;return;
#endif
    DWORD saved=GetLastError();
    /* Rejection-only metadata relation; never influences admission. Do not
       inspect an unpinned object. class_get_parent is already bound by SourceApi. */
    unsigned relation=root?1:0,depth=0;
    if(root && actual && expected) {
        if(actual==expected) relation=2;
        else if(sa.class_get_parent) {
            void *klass=actual;
            for(unsigned i=1;i<=8;++i) {
                klass=sa.class_get_parent(klass);if(!klass) break;
                if(klass==expected) {relation=3;depth=i;break;}
                if(klass==actual) break;
            }
        }
    }
    event("source_transform_rejection",(U64)relation|((U64)depth<<8),GetCurrentThreadId());
#ifdef BOUNDARY_TEST
    admission_test_relation=relation|(depth<<8);
#endif
    SetLastError(saved);
}
#endif
#include "camera_pipeline_scope.h"
#ifdef GF2VR_MULTICAMERA_OBSERVE
/* Scalars from the existing rooted reader, not extra Unity calls. */
static unsigned multicamera_admission_stage;
#undef GF2VR_ADMISSION_STEP
#define GF2VR_ADMISSION_STEP(n) ((void)(multicamera_admission_stage=(n)))
#endif
#if defined(GF2VR_NATIVE_TARGET_CACHE) && defined(GF2VR_NATIVE_BOUNDARY_OBSERVE)
#include "native_boundary_observe.h"
#endif
#include "source_render_frame.h"
#include "source_generation.h"
#ifdef GF2VR_WORLD_SCALE
#include "world_scale_runtime.h"
#endif
#ifdef GF2VR_STEREO_DEPTH
#include "stereo_depth_runtime.h"
#endif
#ifdef GF2VR_THUMBSTICK_NAVIGATION
#include "navigation_runtime.h"
#ifdef GF2VR_CAMERA_RETURN
#include "camera_return_runtime.h"
#endif
#endif
#ifdef GF2VR_CAMERA_PLACEMENT_OBSERVE
#include "camera_placement_observe.h"
#endif
#ifdef GF2VR_ACTIVITY_UI_FRAME_ELIGIBLE
#include "activity_ui_camera_registry.h"
#ifdef GF2VR_ACTIVITY_PRIVATE_BATCH
#include "activity_private_batch.h"
#endif
#define GF2VR_TOPOLOGY_PRIVATE(camera) uicr_owned(camera)
#define GF2VR_TOPOLOGY_REAP(cameras,count) uicr_reap(cameras,count)
#endif
#include "source_topology.h"
#ifdef GF2VR_ACTIVITY_INPUT_BOUNDARY_OBSERVE
/* Copied identities from already rooted raw entries plus the existing registry.
   No engine call, pin, policy decision or deduplication belongs to this observer. */
static void activity_input_boundary_report(const void *context,unsigned reason,unsigned count,
    unsigned seen,unsigned index,unsigned peer,void *const *cameras) {
    static unsigned failed,passed,busy;
    unsigned owner=uicr_thread();
    if(!owner || GetCurrentThreadId()!=owner || busy || !context || !cameras ||
       !count || count>32 || !seen || seen>count || reason>5 ||
       (reason?failed>=16:passed>=4))return;
    DWORD saved=GetLastError();busy=1;
    unsigned record=reason?++failed:++passed;
    char line[3500],*p=source_prefix(line,"activity_raw_input_boundary");
    p=literal(p,",\"record\":");p=decimal(p,record);
    p=literal(p,",\"reason\":");p=decimal(p,reason);
    p=literal(p,",\"stage\":");p=decimal(p,multicamera_admission_stage);
    p=literal(p,",\"owner_thread\":");p=decimal(p,owner);
    p=literal(p,",\"topology\":");p=hexadecimal(p,(U64)context);
    p=literal(p,",\"count\":");p=decimal(p,count);
    p=literal(p,",\"seen\":");p=decimal(p,seen);
    p=literal(p,",\"index\":");p=decimal(p,index);
    p=literal(p,",\"peer\":");p=decimal(p,peer);
    p=literal(p,",\"raw\":[");
    for(unsigned n=0;n<seen;++n) {
        U64 generation=0;unsigned registered=0,retired=0;int instance=0;
        for(unsigned i=0;i<UICR_CAPACITY;++i)if(uicr.rows[i].root && uicr.rows[i].camera==cameras[n]) {
            registered=1;generation=uicr.rows[i].generation;retired=uicr.rows[i].retired;instance=uicr.rows[i].instance;break;
        }
        if(n)p=literal(p,",");
        p=literal(p,"[");p=hexadecimal(p,(U64)cameras[n]);
        p=literal(p,",");p=decimal(p,registered);
        p=literal(p,",");p=hexadecimal(p,generation);
        p=literal(p,",");p=decimal(p,retired);
        p=literal(p,",");p=hexadecimal(p,(unsigned)instance);p=literal(p,"]");
    }
    p=literal(p,"]}\n");source_log(line,p);busy=0;SetLastError(saved);
}
#endif
#ifdef GF2VR_MAIN_PHASE_OBSERVE
static void main_phase_policy(const SourceTopologyEntry *,unsigned,unsigned,unsigned);
#ifdef GF2VR_ACTIVITY_ADMISSION_OBSERVE
static void activity_main_policy(void *,const SourceTopologyEntry *,unsigned,unsigned);
#define GF2VR_MAIN_POLICY_RECORD(g,e,n,r) activity_main_policy(g,e,n,r)
#else
#define GF2VR_MAIN_POLICY_RECORD(g,e,n,r) main_phase_policy(e,n,g->state,r)
#endif
#endif
#ifdef GF2VR_MAIN_HANDOFF
#define GF2VR_MAIN_HANDOFF_RECORD(old_id,new_id) event("main_camera_handoff",((U64)(unsigned)(old_id)<<32)|(unsigned)(new_id),GetCurrentThreadId())
#endif
#ifdef GF2VR_MULTICAMERA_OBSERVE
static void multicamera_policy_record(void *,const SourceTopologyEntry *,unsigned,unsigned);
#define GF2VR_MAIN_POLICY_DETAIL(g,e,n,r) multicamera_policy_record(g,e,n,r)
#define GF2VR_MAIN_POLICY_STEP(r) ((void)(detail=(r)))
#endif
#include "main_test_grant.h"
#ifdef GF2VR_MULTICAMERA_OBSERVE
#ifdef GF2VR_ACTIVITY_STACK_OBSERVE
static void activity_stack_observe(const SourceTopologyEntry *,unsigned,unsigned);
#endif
#include "main_multicamera_observe.h"
#endif
#include "source_eye_prepare.h"
#ifdef GF2VR_NATIVE_BOUNDARY_OBSERVE
#include "native_boundary_observe.h"
#endif
#include "source_live_owner.h"
#ifdef GF2VR_ACTIVITY_PRIMARY_UI
#include "activity_primary_ui_runtime.h"
#ifdef GF2VR_ACTIVITY_DIALOGUE_RAY
#include "dialogue_ray_geometry.h"
static void dialogue_ray_capture(const void *group,const void *math,const void *input,const void *applied,
    const float *geometry,const void *poses,unsigned shown) {
    const ActivityEyeGroup *g=group;const PoseAppliedTicket *t=applied;
    DialogueRaySurface s={.generation=t->source_generation,.epoch=t->reference_epoch,.frame=t->frame_id,
        .tick=GetTickCount64()};
    if(shown && g->dialogue_ui.data.binding && g->dialogue_ui.shown && geometry && poses){
        const ActivityPrimarySource *source=&g->source->active.dialogue;
        s.owner=(U64)source->canvas;s.image_width=source->entry.width;s.image_height=source->entry.height;
#ifdef GF2VR_ACTIVITY_UI_LIFETIME
        s.owner=g->source->ui_serial;
#endif
        s.valid=dialogue_ray_geometry(math,input,geometry,poses,&s.panel)!=0;
    }
    (void)dialogue_ray_publish(&dialogue_ray_mailbox,&s);
#ifdef GF2VR_ACTIVITY_UI_LIFETIME
    if(s.valid && !g->source->ui_change)
        __atomic_store_n(&dialogue_ray_live_ui,s.owner,__ATOMIC_RELEASE);
    else dialogue_ray_invalidate();
#endif
    static U64 last_owner,last_generation;static unsigned last_valid=2,reports;
    if(reports<32 && (last_valid!=s.valid || last_owner!=s.owner || last_generation!=s.generation)){
        ++reports;last_valid=s.valid;last_owner=s.owner;last_generation=s.generation;
        event("activity_dialogue_ray_surface",s.valid?s.generation:0,s.frame);
    }
}
#endif
#ifdef GF2VR_ACTIVITY_UI_ISOLATION_OBSERVE
#include "activity_ui_isolation_observe.h"
#endif
#ifdef GF2VR_ACTIVITY_UI_DIALOGUE_OWNER_OBSERVE
#include "activity_ui_dialogue_owner_observe.h"
#endif
static void primary_ui_report(unsigned long long generation,unsigned long long frame,unsigned shown,float plane) {
    static unsigned reports,last_shown=2;static U64 last_gen,last_tick;U64 now=GetTickCount64();
    if(reports>=64 || (last_shown==shown && last_gen==generation && now-last_tick<1000))return;
    DWORD saved=GetLastError();++reports;last_shown=shown;last_gen=generation;last_tick=now;
    char line[900],*p=source_prefix(line,"activity_primary_ui_frame");
    p=literal(p,",\"generation\":");p=hexadecimal(p,generation);p=literal(p,",\"frame\":");p=hexadecimal(p,frame);
    p=literal(p,",\"shown\":");p=decimal(p,shown);p=literal(p,",\"plane_f32\":");
    union{float value;unsigned bits;}v={.value=plane};p=hexadecimal(p,v.bits);p=literal(p,"}\n");source_log(line,p);SetLastError(saved);
}
#endif
#ifdef GF2VR_ACTIVITY_VIEWPORT
static void activity_viewport_report(unsigned long long generation,unsigned member,unsigned eye,
    const float *base,const float *before,const float *after) {
    static unsigned records;
    if(records>=16)return;
    DWORD saved=GetLastError();++records;
    char line[1300],*p=source_prefix(line,"activity_overlay_viewport");
    p=literal(p,",\"schema\":1,\"record\":");p=decimal(p,records);
    p=literal(p,",\"generation\":");p=hexadecimal(p,generation);
    p=literal(p,",\"member\":");p=decimal(p,member);
    p=literal(p,",\"eye\":");p=decimal(p,eye);
    const char *keys[]={"base_rect_f32","before_rect_f32","after_rect_f32"};
    const float *values[]={base,before,after};
    for(unsigned n=0;n<3;++n) {
        p=literal(p,",\"");p=literal(p,keys[n]);p=literal(p,"\":[");
        for(unsigned i=0;i<4;++i) {
            union {float f;unsigned bits;} value={.f=values[n][i]};
            if(i)p=literal(p,",");p=hexadecimal(p,value.bits);
        }
        p=literal(p,"]");
    }
    p=literal(p,"}\n");source_log(line,p);SetLastError(saved);
}
#endif
#ifdef GF2VR_ACTIVITY_SCALE_PREPARE_OBSERVE
/* Existing main-owned return values and Rect copies only; no extra engine call. */
static void activity_prepare_boundary_report(unsigned long long generation,unsigned count,unsigned member,
    unsigned stage,int width,int height,unsigned ok) {
    static unsigned records,busy;
    if(busy || records>=16)return;
    DWORD saved=GetLastError();busy=1;++records;
    char line[1100],*p=source_prefix(line,"activity_scale_prepare_boundary");
    p=literal(p,",\"record\":");p=decimal(p,records);
    p=literal(p,",\"generation\":");p=hexadecimal(p,generation);
    p=literal(p,",\"members\":");p=decimal(p,count);
    p=literal(p,",\"member\":");p=decimal(p,member);
    p=literal(p,",\"stage\":");p=decimal(p,stage);
    p=literal(p,",\"width\":");p=decimal(p,width>0?(unsigned)width:0);
    p=literal(p,",\"height\":");p=decimal(p,height>0?(unsigned)height:0);
    p=literal(p,",\"ok\":");p=decimal(p,ok);
    p=literal(p,"}\n");source_log(line,p);busy=0;SetLastError(saved);
}
static void activity_viewport_boundary_report(void *context,void *camera,unsigned attempted,unsigned passed,
    const void *base,const void *before,const void *after) {
    static unsigned records,busy;
    if(busy || records>=32 || !context)return;
    ActivityEyeGroup *g=context;
    if(!g->source || !g->source->source || GetCurrentThreadId()!=g->source->source->owner_thread)return;
    DWORD saved=GetLastError();busy=1;++records;
    char line[1800],*p=source_prefix(line,"activity_scale_viewport_boundary");
    p=literal(p,",\"record\":");p=decimal(p,records);
    p=literal(p,",\"generation\":");p=hexadecimal(p,g->generation);
    p=literal(p,",\"camera\":");p=hexadecimal(p,(U64)camera);
    p=literal(p,",\"attempted\":");p=decimal(p,attempted);
    p=literal(p,",\"passed\":");p=decimal(p,passed);
    const void *values[]={base,before,after};const char *keys[]={"base_rect_f32","before_rect_f32","after_rect_f32"};
    for(unsigned n=0;n<3;++n) {
        p=literal(p,",\"");p=literal(p,keys[n]);p=literal(p,"\":[");
        const ActivityViewport *v=values[n];
        for(unsigned i=0;i<4;++i) {
            union {float f;unsigned bits;} value={.f=v?v->v[i]:0};
            if(i)p=literal(p,",");p=hexadecimal(p,value.bits);
        }
        p=literal(p,"]");
    }
    p=literal(p,"}\n");source_log(line,p);busy=0;SetLastError(saved);
}
#endif
#ifdef GF2VR_ORIENTATION_OBSERVE
#include "orientation_observe.h"
#include "orientation_matrix_observe.h"
#endif
#include "eye_native_targets.h"
#include "eye_event_copy.h"
#include "eye_render_interval.h"
#include "eye_flight_backend.h"
#include "async_throughput_observe.h"
#include "stereo_hooks.h"
#include "draw_runtime_contract.h"
#if defined(GF2VR_SUBMISSION_OBSERVE) || defined(GF2VR_PREPARE_OBSERVE)
#include "submit_commands_observer.h"
#endif
#ifdef GF2VR_DRAW_ORDER_OBSERVE
#include "draw_order_observer.h"
#endif
static CameraPipelineScope stereo_pipeline;
#ifdef GF2VR_MAIN_PHASE_OBSERVE
#ifdef GF2VR_MAIN_PHASE_ROLES
static void main_phase_roles(const SourceTopologyEntry *,unsigned,unsigned);
#endif
static unsigned phase_records,phase_last_valid;
static U64 phase_last_signature;
#ifdef GF2VR_ACTIVITY_ADMISSION_OBSERVE
static unsigned activity_known_worlds,activity_flat_tracking,activity_tracking_focused;
static int activity_selected_instance,activity_ui_instance;
static void activity_main_policy(void *context,const SourceTopologyEntry *entries,
                                 unsigned count,unsigned reason) {
    const MainTestGrant *g=context;
    activity_known_worlds=g->world_count;
    activity_flat_tracking=g->flat_tracking;
    activity_tracking_focused=g->tracking_focused;
    activity_selected_instance=g->identity.instance_id;
    activity_ui_instance=g->ui_identity.instance_id;
    main_phase_policy(entries,count,g->state,reason);
}
#endif
static void main_phase_policy(const SourceTopologyEntry *entries,unsigned count,unsigned state,unsigned reason) {
    if(!entries || count>32) return;
    U64 signature=((U64)count<<32)|reason;
#ifdef GF2VR_ACTIVITY_ADMISSION_OBSERVE
    signature=signature*1099511628211ULL^activity_known_worlds^(unsigned)activity_selected_instance;
    signature=signature*1099511628211ULL^(unsigned)activity_ui_instance^activity_flat_tracking;
#endif
    for(unsigned i=0;i<count;++i) {
        const SourceTopologyEntry *e=&entries[i];
        signature=signature*1099511628211ULL^(U64)e->identity.source^(unsigned)e->identity.instance_id;
        signature=signature*1099511628211ULL^(unsigned)e->mask^(unsigned)e->width^((U64)(unsigned)e->height<<32);
        signature=signature*1099511628211ULL^e->has_target^(unsigned)e->display^((U64)(unsigned)e->orthographic<<32);
        for(unsigned j=0;j<e->identity.count && j<8;++j)
            signature=signature*1099511628211ULL^e->identity.owners[j]^(unsigned)e->identity.owner_ids[j];
    }
    if(phase_records>=24 || (phase_last_valid && phase_last_signature==signature)) return;
    phase_last_valid=1;phase_last_signature=signature;unsigned record=++phase_records;
    DWORD saved=GetLastError();
    char line[1800],*p=source_prefix(line,"main_phase_policy");
    p=literal(p,",\"record\":");p=decimal(p,record);
    p=literal(p,",\"serial\":");p=decimal(p,stereo_pipeline.render.serial);
    p=literal(p,",\"state\":");p=decimal(p,state);
    p=literal(p,",\"reason\":");p=decimal(p,reason);
    p=literal(p,",\"count\":");p=decimal(p,count);
#ifdef GF2VR_ACTIVITY_ADMISSION_OBSERVE
    p=literal(p,",\"known_worlds\":");p=decimal(p,activity_known_worlds);
    p=literal(p,",\"selected_instance\":");p=hexadecimal(p,(unsigned)activity_selected_instance);
    p=literal(p,",\"ui_instance\":");p=hexadecimal(p,(unsigned)activity_ui_instance);
    p=literal(p,",\"flat_tracking\":");p=decimal(p,activity_flat_tracking);
    p=literal(p,",\"tracking_focused\":");p=decimal(p,activity_tracking_focused);
#endif
    p=literal(p,"}\n");source_log(line,p);
    for(unsigned i=0;i<count;++i) {
        const SourceTopologyEntry *e=&entries[i];p=source_prefix(line,"main_phase_camera");
        p=literal(p,",\"record\":");p=decimal(p,record);
        p=literal(p,",\"index\":");p=decimal(p,i);
        p=literal(p,",\"source\":");p=hexadecimal(p,(U64)e->identity.source);
        p=literal(p,",\"instance\":");p=hexadecimal(p,(unsigned)e->identity.instance_id);
        p=literal(p,",\"orthographic\":");p=decimal(p,e->orthographic);
        p=literal(p,",\"display\":");p=decimal(p,e->display);
        p=literal(p,",\"target\":");p=decimal(p,e->has_target);
        p=literal(p,",\"width\":");p=decimal(p,e->width);
        p=literal(p,",\"height\":");p=decimal(p,e->height);
        p=literal(p,",\"mask\":");p=hexadecimal(p,(unsigned)e->mask);
        p=literal(p,",\"owners\":[");
        for(unsigned j=0;j<e->identity.count && j<8;++j) {
            if(j) p=literal(p,",");p=hexadecimal(p,e->identity.owners[j]);
        }
        p=literal(p,"]}\n");source_log(line,p);
    }
#ifdef GF2VR_MAIN_PHASE_ROLES
    main_phase_roles(entries,count,record);
#endif
    SetLastError(saved);
}
#endif
/* Populated only by the future live source/backend owner, after its exact
   pipeline/method/source/scale and output consumer contracts are established.
   Zero initialization is an operational original-array fallback, not eligibility. */
static struct {
    unsigned enabled;
    void *pipeline,*method;
    CameraDataOwner *data;
    const SourceRenderSelection *selection;
    SourcePoseApi *reader;
    SourceAnchorState *anchor;
    PoseMathBinding *math;
    XrEyeMailbox *mailbox;
    const CameraBatchApi *batch;
    SourceGeneration *generation;
    SourceTopology *topology;
    MainTestGrant *manual;
    SourceEyePreparation *preparation;
    int eye_width,eye_height;
} stereo_source;
static SourceRenderFrame stereo_source_frame;
static unsigned stereo_prepare_attempted;
static unsigned stereo_source_observed;
#ifdef GF2VR_ACTIVITY_PRIVATE_BATCH
static ActivityPrivateBatch stereo_private_batch;
#endif
static CameraRenderForward stereo_render_original;
static int stereo_cpu_returned;
static DWORD stereo_forward_error;
/* Local adapter for the verified Submit_Internal_Injected storage argument.
   No hook installed here. A returned matching Submit is correlation evidence,
   not proof that both eye commands were produced or reached the graphics owner. */
typedef void (*StereoSubmitForward)(void *);
static StereoSubmitForward stereo_submit_original;
static struct {
    void *context; /* Borrowed only while selected original Render is on stack. */
    U64 serial,returned_serial;
    unsigned depth,submit_depth,matches,ambiguous;
} stereo_submit_scope;
#ifdef GF2VR_SUBMIT_BOUNDARY
/* Main-owner numeric observations only; never consulted for eligibility. */
typedef struct {
    unsigned calls,selected_calls,context_mismatches,reason_mask,render_calls,selected_renders;
    U64 expected_context,observed_context;
} SubmitBoundary;
static SubmitBoundary submit_boundary;
#endif
#ifdef GF2VR_FOCUS_CORRECT
#include "focus_projection_runtime.h"
#endif
__declspec(dllimport) BOOL WINAPI ReadProcessMemory(HANDLE,const void *,void *,U64,U64 *);
#ifdef GF2VR_ROOT_INTERVAL_TIMING_OBSERVE
#include "root_interval_observe.h"
static RootIntervalTag rit_main_tag(void);
#endif
static void stereo_submit_forward(void *storage) {
    DWORD incoming=GetLastError();StereoSubmitForward original=stereo_submit_original;
    if(GetCurrentThreadId()!=stereo_pipeline.owner_thread) {
#ifdef GF2VR_PRODUCER_SCOPE_OBSERVE
#ifdef GF2VR_PRODUCER_FIRST_FAULT_OBSERVE
        producer_scope_foreign_trace(__FILE__,__LINE__);
#else
        producer_scope_foreign();
#endif
#endif
        SetLastError(incoming);original(storage);return;
    }
    unsigned selected=stereo_submit_scope.depth==1 && stereo_submit_scope.context &&
        stereo_pipeline.render.active && stereo_submit_scope.serial==stereo_pipeline.render.serial;
    void *context=0;U64 read=0;
    if(selected && (!storage || !ReadProcessMemory((HANDLE)(long long)-1,storage,&context,sizeof(context),&read) ||
                    read!=sizeof(context))) {
        stereo_submit_scope.ambiguous=1;
#ifdef GF2VR_SUBMIT_BOUNDARY
        submit_boundary.reason_mask|=1;
#endif
    }
    unsigned match=selected && context==stereo_submit_scope.context;
#ifdef GF2VR_SUBMIT_BOUNDARY
    ++submit_boundary.calls;
    if(selected) {
        ++submit_boundary.selected_calls;
        submit_boundary.expected_context=(U64)stereo_submit_scope.context;
        submit_boundary.observed_context=(U64)context;
        if(!match) ++submit_boundary.context_mismatches;
    }
#endif
    if(selected && stereo_submit_scope.submit_depth) {
        stereo_submit_scope.ambiguous=1;
#ifdef GF2VR_SUBMIT_BOUNDARY
        submit_boundary.reason_mask|=2;
#endif
    }
    ++stereo_submit_scope.submit_depth;
#if defined(GF2VR_SUBMISSION_OBSERVE) || defined(GF2VR_PREPARE_OBSERVE)
    if(match && stereo_submit_scope.submit_depth==1) submit_commands_before(context,stereo_submit_scope.serial);
#endif
#ifdef GF2VR_PRODUCER_SCOPE_OBSERVE
    if(selected)producer_scope_submit(1,context,match && !stereo_submit_scope.ambiguous);
#endif
    SetLastError(incoming);
#ifdef GF2VR_ROOT_INTERVAL_TIMING_OBSERVE
    RootIntervalTag timing_tag=(match && !stereo_submit_scope.ambiguous &&
        stereo_submit_scope.submit_depth==1)?rit_main_tag():(RootIntervalTag){0};
    rit_submit_call(timing_tag,original,storage);
#else
    original(storage);
#endif
    DWORD outgoing=GetLastError();
#ifdef GF2VR_PRODUCER_SCOPE_OBSERVE
    if(selected)producer_scope_submit(0,context,match && !stereo_submit_scope.ambiguous);
#endif
    --stereo_submit_scope.submit_depth;
#if defined(GF2VR_SUBMISSION_OBSERVE) || defined(GF2VR_PREPARE_OBSERVE)
    if(match && !stereo_submit_scope.submit_depth) submit_commands_after(stereo_submit_scope.serial);
#endif
    if(match && !stereo_submit_scope.ambiguous) {
        if(stereo_submit_scope.matches==~0u) {
            stereo_submit_scope.ambiguous=1;
#ifdef GF2VR_SUBMIT_BOUNDARY
            submit_boundary.reason_mask|=4;
#endif
        }
        else ++stereo_submit_scope.matches;
    }
    SetLastError(outgoing);
}
/* Output provider is disabled until submitted-context and XR acquisition are
   actually supplied. These fields are Unity-owner-thread only. The XR adapter
   must hold both acquired/waited images until completed is reconciled. */
static struct {
    unsigned enabled,busy;
    EyeNativeApi native;
    EyeGpuTicket submitted; /* Actual submission provider's evidence, never root-return synthesized. */
    EyeGpuTicket reserved; /* Acquired XR destination lease, NOT submission evidence. */
    void *destinations[2];
    EyeGpuTicket completed; /* Owner result awaits XR adapter consumption; no image release here. */
} stereo_output;
static EyeEventCopy stereo_output_event;
/* Explicit opt-in remains off until the coherent producer configuration is
   ready. This interval supplies command observations and drain, never output
   permission synthesized from root return. Storage survives published hooks. */
#if defined(GF2VR_MAIN_STEREO) && !defined(BOUNDARY_TEST)
static unsigned stereo_interval_enabled=1;
#else
static unsigned stereo_interval_enabled;
#endif
static EyeRenderInterval stereo_interval;
static DrawQueueAddress stereo_interval_child;
#if defined(GF2VR_MAIN_STEREO)
#include "eye_dispatch_bridge.h"
#endif
#ifdef GF2VR_PREPARE_OBSERVE
#include "native_draw_observe.h"
#include "eye_clear_observe.h"
#endif
static EyeIntervalResult stereo_interval_result;
static unsigned stereo_interval_xr_returned; /* published by interval DONE */
static void stereo_interval_callback(int id);
static struct {U64 serial;DrawQueueIdentity entry;unsigned ready;} stereo_draw_order;
#ifdef GF2VR_COMPOSITION_OBSERVE
static U64 composition_request_serial;
#endif
static DrawQueueAddress stereo_output_child;
static XrEyeOutput stereo_xr_output;
static XrEyeConfig stereo_xr_config;
static XrEyeSize stereo_xr_size;
static EyeGpuDescription stereo_frame_description; /* Unity owner only */
/* Compare-only snapshot protected by the private eye owner's frame borrow.
   Never dereference/release these addresses as owned COM references. */
static struct {EyeGpuTicket ticket;EyeNativeTargets targets;} stereo_native_frame;
static int stereo_native_same(const SourceRenderFrame *f,const EyeNativeTargets *now) {
    return f && now && f->serial && f->check.ticket.source_generation && f->epoch &&
        stereo_native_frame.ticket.serial==f->serial &&
        stereo_native_frame.ticket.generation==f->check.ticket.source_generation &&
        stereo_native_frame.ticket.epoch==f->epoch &&
        now->frame && now->frame==f->check.ticket.frame_id &&
        now->frame==stereo_native_frame.targets.frame && now->resource[0] && now->resource[1] &&
        now->resource[0]!=now->resource[1] &&
        now->resource[0]==stereo_native_frame.targets.resource[0] &&
        now->resource[1]==stereo_native_frame.targets.resource[1];
}
static void stereo_native_forget(U64 serial) {
    if(stereo_native_frame.ticket.serial!=serial) return;
    stereo_native_frame.ticket=(EyeGpuTicket){0};stereo_native_frame.targets=(EyeNativeTargets){0};
}
/* Actual source classifier must publish generation, or zero BEFORE invalidation.
   Never write this from root-return/completion. The optional generation owner
   writes this; actual game attribution and enable configuration remain absent. */
static U64 stereo_publication_generation;
static U64 stereo_generation_clock; /* never reset by managed owner detach/rebind */
static SourceLiveOwner *stereo_live;
/* Two-flight producer transaction. Activation remains off until the sole XR
   frame owner supplies immutable predictions and ordered image obligations. */
static struct {
    unsigned enabled,root_slot;
    U64 root_serial;
    void *original_cameras; /* main-only borrowed during wrapper call */
    EyeFlightOwner owner;
    EyePosePool poses;
    EyeFlightBackend backend;
    struct {
        EyeFlightSnapshot admitted;
        DrawQueueIdentity entry;
        EyeGpuDescription description;
        unsigned ready,root_finished,cpu_pending,cpu_ok,end_pending;
#ifdef GF2VR_PRODUCER_TRANSFER
        unsigned producer_failed;
#endif
#ifdef GF2VR_TWO_FLIGHT_ARM_OBSERVE
        unsigned arm_stage;
        unsigned arm_nested_stage;
        int arm_result;
#ifdef GF2VR_TWO_FLIGHT_NATIVE_OBSERVE
        unsigned arm_native_reason;
#endif
#endif
    } slots[EFO_CAPACITY];
} stereo_two = {
#if defined(GF2VR_TWO_FLIGHT_ROUTE) && !defined(BOUNDARY_TEST)
    .enabled=1,
#endif
    .root_slot=EFO_CAPACITY
};
static XrEyeFrameDriver stereo_two_xr;
static XrEyeFlightImages stereo_two_images;
#ifdef GF2VR_ASYNC_PRESENTATION
/* Dormant until the coherent product selector is enabled. Boundary fixtures
   exercise the real hooks explicitly without changing accepted installed code. */
#if defined(GF2VR_ASYNC_SCHEDULER) && !defined(BOUNDARY_TEST)
static unsigned stereo_async_enabled=1;
#else
static unsigned stereo_async_enabled;
#endif
static unsigned stereo_async_discard;
static int stereo_async_drain(PanelXr *panel) {
    if(!stereo_two_images.token.frame)return EFO_OK;
    EyeFlightToken token=stereo_two_images.token;EyeFlightSnapshot snapshot;
    int r=eye_flight_snapshot(&stereo_two.owner,token,&snapshot);if(r!=EFO_OK)return r;
    if(snapshot.flags&EFF_PRODUCER_RETAINED) {
        r=eye_flight_cancel_retained_offer(&stereo_two.owner,token);if(r!=EFO_OK)return r;
        /* Reservation may race revocation; read after the cancel lock. An
           unreserved cancelled offer can no longer be admitted by MAIN. */
        r=eye_flight_snapshot(&stereo_two.owner,token,&snapshot);if(r!=EFO_OK)return r;
    }
    r=xfi_release(&stereo_two_images,&stereo_xr_output,panel,&stereo_two.owner);
    if(r!=EFO_OK)return r;
    if(stereo_two_images.detached) {
        r=xfi_detached_finalize(&stereo_two_images,&stereo_xr_output,&stereo_two.owner,0,0);
        if(r==EFO_OK) {
            stereo_async_discard=0;
            /* MAIN only retires admitted roots. XR owns an unreserved offer's
               immutable pose cleanup after real End and discard disposition. */
            if(!(snapshot.flags&EFF_RESERVED)) {
                do {r=eye_flight_take(&stereo_two.owner,token,&snapshot);if(r==EFO_BUSY)Sleep(1);} while(r==EFO_BUSY);
                if(r!=EFO_OK)return r;
                do {r=eye_pose_pool_retire(&stereo_two.poses,&stereo_two.owner,token);if(r==EFO_BUSY)Sleep(1);} while(r==EFO_BUSY);
            }
        }
        return r;
    }
    return EFO_OK;
}
#endif
static U64 stereo_two_frame_clock;
#ifdef GF2VR_TWO_FLIGHT_PREFETCH_OBSERVE
/* XR-owner-only numeric observation of next-Wait overlap with current End.
   No additional XR, Unity, graphics, or resource-ownership operations. */
static struct {
    U64 start,request_tick,ready_delay_ms,ready_id;
    unsigned reports,begun,request_ok,request_busy,request_rejected;
    unsigned ready_before_end,ready_after_end,end_ready,end_waiting,end_none;
} two_prefetch_note;
static void two_prefetch_report(void) {
    U64 now=GetTickCount64();
    if(!two_prefetch_note.start || now-two_prefetch_note.start<2000 ||
       two_prefetch_note.reports>=24)return;
    DWORD saved=GetLastError();char line[900],*p=source_prefix(line,"two_prefetch_overlap");
    p=literal(p,",\"elapsed_ms\":");p=decimal(p,now-two_prefetch_note.start);
    p=literal(p,",\"begun\":");p=decimal(p,two_prefetch_note.begun);
    p=literal(p,",\"request_ok\":");p=decimal(p,two_prefetch_note.request_ok);
    p=literal(p,",\"request_busy\":");p=decimal(p,two_prefetch_note.request_busy);
    p=literal(p,",\"request_rejected\":");p=decimal(p,two_prefetch_note.request_rejected);
    p=literal(p,",\"ready_before_end\":");p=decimal(p,two_prefetch_note.ready_before_end);
    p=literal(p,",\"ready_after_end\":");p=decimal(p,two_prefetch_note.ready_after_end);
    p=literal(p,",\"end_ready\":");p=decimal(p,two_prefetch_note.end_ready);
    p=literal(p,",\"end_waiting\":");p=decimal(p,two_prefetch_note.end_waiting);
    p=literal(p,",\"end_none\":");p=decimal(p,two_prefetch_note.end_none);
    p=literal(p,",\"ready_delay_ms\":");p=decimal(p,two_prefetch_note.ready_delay_ms);
    p=literal(p,"}\n");source_log(line,p);SetLastError(saved);
    unsigned reports=two_prefetch_note.reports+1;
    U64 ready_id=two_prefetch_note.ready_id,request_tick=two_prefetch_note.request_tick;
    two_prefetch_note=(__typeof__(two_prefetch_note)){0};
    two_prefetch_note.reports=reports;two_prefetch_note.start=now;
    two_prefetch_note.ready_id=ready_id;two_prefetch_note.request_tick=request_tick;
}
#ifdef BOUNDARY_TEST
__declspec(dllexport) int two_prefetch_observe_test(void) {
    __typeof__(two_prefetch_note) saved_note=two_prefetch_note;
    DWORD saved_error=GetLastError();
    U64 now=GetTickCount64();
    two_prefetch_note=(__typeof__(two_prefetch_note)){0};
    two_prefetch_note.start=now>=2000?now-2000:1;
    two_prefetch_note.begun=5;two_prefetch_note.request_ok=4;
    two_prefetch_note.ready_before_end=3;two_prefetch_note.end_ready=3;
    two_prefetch_note.end_waiting=2;two_prefetch_note.ready_delay_ms=19;
    two_prefetch_note.ready_id=41;two_prefetch_note.request_tick=now;
    SetLastError(773);two_prefetch_report();
    int ok=two_prefetch_note.reports==1 && two_prefetch_note.ready_id==41 &&
        two_prefetch_note.request_tick==now && GetLastError()==773;
    two_prefetch_note=saved_note;SetLastError(saved_error);
    return ok?0:__LINE__;
}
#endif
#endif
static int stereo_two_xr_close(PanelXr *);
/* Sole XR writer synchronizes only numeric publication/reference identities;
   it never reads Unity's mutable source records or reseeds the physical origin. */
static int stereo_two_xr_sync(void) {
    unsigned free_slots;U64 generation,epoch;
    int r=eye_flight_available(&stereo_two.owner,&free_slots,&generation,&epoch);
    if(r!=EFO_OK)return r;
    U64 published=__atomic_load_n(&stereo_publication_generation,__ATOMIC_ACQUIRE);
    XrEyeOwner *o=&stereo_eye_runtime.owner;
    U64 reference=(!o->changing && !o->retiring && o->head_reference_epoch==o->epoch)?o->epoch:0;
    if(generation && (generation!=published || epoch!=reference)) {
        r=eye_flight_stop(&stereo_two.owner);if(r!=EFO_OK)return r;generation=0;
    }
    if(!generation && free_slots==EFO_CAPACITY && published && reference)
        return eye_flight_activate(&stereo_two.owner,published,reference);
    return EFO_OK;
}
static int stereo_two_xr_prefetch(void) {
    XrEyeFrameDriver *d=&stereo_two_xr;
    if(d->stopping || d->fault)return EFO_REJECTED;
#ifdef GF2VR_BANK_WAIT_AFTER_END
    /* VDXR Wait can hold End's synchronization boundary. Keep the bank owner
       free to acknowledge End and service detached completion first; the
       normal next-frame Wait loop then pumps that service while Wait blocks.
       current also protects once-only End's pending numeric acknowledgement. */
    if(stereo_bank_enabled && (d->current || d->api.open_id))return EFO_BUSY;
#endif
    if(!d->requested && !d->predicted) {
        if(stereo_two_frame_clock==~0ull)return EFO_REJECTED;
        int r;
#ifdef GF2VR_ASYNC_PRESENTATION
        if(stereo_async_enabled) {
#ifdef GF2VR_BANK_PRESENTATION
            if(stereo_bank_enabled)r=xfd_request_display(d,stereo_two_frame_clock+1);
            else
#endif
                r=xfd_request_single_writer_display(d,stereo_two_frame_clock+1);
        }
        else
#endif
            r=xfd_request(d,stereo_two_frame_clock+1);
#ifdef GF2VR_TWO_FLIGHT_PREFETCH_OBSERVE
        if(!two_prefetch_note.start)two_prefetch_note.start=GetTickCount64();
        if(r==EFO_OK){++two_prefetch_note.request_ok;two_prefetch_note.request_tick=GetTickCount64();}
        else if(r==EFO_BUSY)++two_prefetch_note.request_busy;
        else ++two_prefetch_note.request_rejected;
#endif
        if(r!=EFO_OK)return r;
        ++stereo_two_frame_clock;
    }
    int result=xfd_take(d);
#ifdef GF2VR_TWO_FLIGHT_PREFETCH_OBSERVE
    if(result==EFO_OK && d->predicted && d->predicted!=two_prefetch_note.ready_id) {
        two_prefetch_note.ready_id=d->predicted;
        if(d->current && d->begun)++two_prefetch_note.ready_before_end;
        else ++two_prefetch_note.ready_after_end;
        if(two_prefetch_note.request_tick) {
            U64 now=GetTickCount64();
            if(now>=two_prefetch_note.request_tick)
                two_prefetch_note.ready_delay_ms+=now-two_prefetch_note.request_tick;
        }
        two_prefetch_note.request_tick=0;
    }
#endif
    return result;
}
static XrResult stereo_frame_wait(void *opaque,const XrFrameWaitInfo *wait,XrFrameState *frame) {
    PanelXr *p=opaque;
    if(!stereo_two.enabled)return p->api.WaitFrame(p->session,wait,frame);
    XrEyeFrameDriver *d=&stereo_two_xr;
    if(!d->thread && xfd_open(d,&stereo_two.owner,&stereo_two.poses,&stereo_eye_runtime,
            p->session,p->api.WaitFrame,p->api.BeginFrame,p->api.EndFrame)!=EFO_OK)
        return XR_ERROR_RUNTIME_FAILURE;
    if(!xfd_owner(d) || d->api.prefetch.session!=p->session)return XR_ERROR_CALL_ORDER_INVALID;
    int r;
    do {r=stereo_two_xr_sync();if(r==EFO_BUSY)Sleep(1);} while(r==EFO_BUSY);
    if(r!=EFO_OK)return XR_ERROR_RUNTIME_FAILURE;
    do {
#ifdef GF2VR_BANK_COPY_SERVICE
        if(stereo_bank_enabled && stereo_bank_copy_service(p)==EFO_REJECTED)
            return XR_ERROR_RUNTIME_FAILURE;
#endif
        r=stereo_two_xr_prefetch();if(r==EFO_BUSY)Sleep(1);
    } while(r==EFO_BUSY);
    if(r!=EFO_OK)return XR_ERROR_RUNTIME_FAILURE;
    *frame=d->prediction;
    if(d->stopping)frame->shouldRender=0;
    return XR_SUCCESS; /* successful waited frame still requires Begin/End */
}
static int stereo_two_current_pose(void *context,U64 id,const XrFrameState *frame,
        unsigned attempted,const XrEyeInput *prediction,XrEyeInput *out) {
    if(context) {xr_eye_mailbox_invalidate(&stereo_eye_runtime.mailbox);*out=(XrEyeInput){0};return 0;}
    return stereo_eye_frame_prepare(0,id,frame,attempted,prediction,out);
}
static XrResult stereo_frame_begin(void *opaque,const XrFrameState *frame,int exiting) {
    if(!stereo_two.enabled)return panel_frame_begin_legacy(opaque,frame,exiting);
    PanelXr *panel=opaque;
    if(!xfd_owner(&stereo_two_xr) || stereo_two_xr.api.prefetch.session!=panel->session ||
       !frame || frame->type!=XR_TYPE_FRAME_STATE ||
       frame->predictedDisplayTime!=stereo_two_xr.prediction.predictedDisplayTime)
        return XR_ERROR_CALL_ORDER_INVALID;
    int r;
#ifdef GF2VR_BANK_READMIT
    if(!exiting && stereo_bank_enabled && stereo_hold_focused) {
        /* No wait here: contention preserves this display-only frame. */
        (void)xfd_readmit_bank_prediction(&stereo_two_xr,&stereo_banks,panel,
            __atomic_load_n(&stereo_publication_generation,__ATOMIC_ACQUIRE));
#ifdef GF2VR_READY_RECEIPT_READMIT
        (void)xfd_readmit_ready_prediction(&stereo_two_xr,&stereo_banks,panel,
            __atomic_load_n(&stereo_publication_generation,__ATOMIC_ACQUIRE),&stereo_completed_cancel_clock);
#endif
#line 975
    }
#endif
#ifdef GF2VR_NUMERIC_LOSS_OBSERVE
    unsigned nlo_frame_context=(stereo_two_xr.requested_display_only!=0)|
      ((stereo_two_xr.prediction.shouldRender!=0)<<1)|((stereo_two_xr.predicted_pose.valid!=0)<<2)|
      ((stereo_two_xr.pending_token.frame!=0)<<3)|((stereo_hold_focused!=0)<<4)|((exiting!=0)<<5)|
      ((stereo_two_xr.stopping!=0)<<6)|((stereo_two_xr.fault!=0)<<7);
#endif
    do {r=xfd_begin(&stereo_two_xr,stereo_two_current_pose,exiting?(void *)1:0);
        if(r==EFO_BUSY)Sleep(1);} while(r==EFO_BUSY);
    if(r!=EFO_OK)return XR_ERROR_RUNTIME_FAILURE;
#ifdef GF2VR_NUMERIC_LOSS_OBSERVE
    EyeFlightToken nlo_frame_token=stereo_two_xr.current_token.frame?stereo_two_xr.current_token:
       (EyeFlightToken){stereo_two_xr.current,stereo_two_xr.requested_generation,stereo_two_xr.requested_epoch};
    NLO_RECORD(stereo_two_xr.owner,nlo_frame_token,0,NLO_FRAME,stereo_two_xr.current_token.frame?0:1,
       stereo_two_xr.current_token.frame?EFO_OK:EFO_REJECTED,
       nlo_frame_context|((stereo_two_xr.current_pose.valid!=0)<<8));
#endif
    /* No second Begin: only next Wait/capture may overlap this current frame. */
    async_throughput_record(6,stereo_two_xr.current_token,0,0,EFO_OK,stereo_two_xr.current);
#ifdef GF2VR_TWO_FLIGHT_PREFETCH_OBSERVE
    ++two_prefetch_note.begun;
#endif
    if(!exiting && !stereo_two_xr.stopping)(void)stereo_two_xr_prefetch();
    return stereo_two_xr.begin_result;
}
static XrResult stereo_two_frame_end(const XrFrameEndInfo *end,int fresh) {
    EyeFlightToken token=stereo_two_xr.current_token;
#ifdef GF2VR_TWO_FLIGHT_PREFETCH_OBSERVE
    unsigned predicted_ready=stereo_two_xr.predicted!=0;
    unsigned prediction_waiting=stereo_two_xr.requested!=0;
#endif
    int r;int detached=0;
#ifdef GF2VR_ASYNC_PRESENTATION
    if(stereo_async_enabled && !fresh && token.frame &&
       xfi_same(stereo_two_images.token,token)) {
        do {r=stereo_two_images.pending_offer?
            eye_flight_can_retain_offer(&stereo_two.owner,token,end->displayTime):
            eye_flight_can_detach(&stereo_two.owner,token,end->displayTime);
            if(r==EFO_BUSY)Sleep(1);} while(r==EFO_BUSY);
        detached=r==EFO_OK?(stereo_two_images.pending_offer?2:1):0;
    }
#endif
    do {
#ifdef GF2VR_ASYNC_PRESENTATION
        if(detached==2)r=xfd_end_retained_offer(&stereo_two_xr,end,&stereo_two_images,&stereo_xr_output);
        else if(detached)r=xfd_end_detached(&stereo_two_xr,end,&stereo_xr_output);
        else
#endif
            r=xfd_end(&stereo_two_xr,end,fresh);
        if(r==EFO_BUSY)Sleep(1);
    } while(r==EFO_BUSY);
    (void)detached;
    if(r!=EFO_OK)return XR_ERROR_RUNTIME_FAILURE;
#ifdef GF2VR_TWO_FLIGHT_PREFETCH_OBSERVE
    if(predicted_ready)++two_prefetch_note.end_ready;
    else if(prediction_waiting)++two_prefetch_note.end_waiting;
    else ++two_prefetch_note.end_none;
    two_prefetch_report();
#endif
    /* An offered packet that Unity never reserved has no native/managed lease.
       Main only knows admitted roots. Ordinary End retires an orphan; the
       explicit detached pending-offer contract instead retains its pose. */
    if(token.frame) {
        EyeFlightSnapshot s;
        do {r=eye_flight_snapshot(&stereo_two.owner,token,&s);if(r==EFO_BUSY)Sleep(1);} while(r==EFO_BUSY);
        if(r==EFO_OK && !(s.flags&(EFF_RESERVED|EFF_DISPLAY_DETACHED))) {
#ifdef GF2VR_ASYNC_PRESENTATION
            /* Cancelled pre-admission bank: clear XR-local identity first.
               Otherwise the post-End sync would read an already retired pose.
               Unreserved work has acquired no image or managed/native ticket. */
            if(stereo_async_enabled && xfi_same(stereo_two_images.token,token) &&
               xfi_retire(&stereo_two_images,&stereo_xr_output,1)!=EFO_OK)return XR_ERROR_RUNTIME_FAILURE;
#endif
            if(!(s.flags&EFF_RECLAIMING)) {
                do {r=eye_flight_take(&stereo_two.owner,token,&s);if(r==EFO_BUSY)Sleep(1);} while(r==EFO_BUSY);
                if(r!=EFO_OK)return XR_ERROR_RUNTIME_FAILURE;
            }
            do {r=eye_pose_pool_retire(&stereo_two.poses,&stereo_two.owner,token);if(r==EFO_BUSY)Sleep(1);} while(r==EFO_BUSY);
            if(r!=EFO_OK)return XR_ERROR_RUNTIME_FAILURE;
        }
    }
    return stereo_two_xr.end_result;
}
#ifdef GF2VR_NUMERIC_RESIDENCY
#include "numeric_residency_log.h"
#endif
#line 1046
static int stereo_two_xr_close(PanelXr *panel) {
#ifdef GF2VR_BANK_PRESENTATION
#ifdef GF2VR_NUMERIC_RESIDENCY
    if(stereo_bank_enabled){int closed=stereo_bank_close(panel);if(closed)numeric_residency_dump();return closed;}
#else
    if(stereo_bank_enabled)return stereo_bank_close(panel);
#endif
#endif
    XrEyeFrameDriver *d=&stereo_two_xr;
    if(!d->thread)return !stereo_two_images.token.frame;
#ifdef GF2VR_ASYNC_PRESENTATION
    if(stereo_async_enabled && stereo_two_images.token.frame) {
        (void)eye_flight_stop(&stereo_two.owner);
        if(stereo_async_drain(panel)!=EFO_OK)return 0;
    }
#endif
    int r=xfd_close(d);if(r==EFO_OK)return !stereo_two_images.token.frame;
    if(d->fault || r==EFO_REJECTED)return 0; /* retain session/module, not guessed reset */
    if(d->requested)(void)xfd_take(d);
    if(!d->current && d->predicted)(void)xfd_begin(d,stereo_two_current_pose,(void *)1);
    if(d->current && !d->begun)(void)xfd_begin(d,stereo_two_current_pose,(void *)1);
    if(d->current && d->begun) {
        if(stereo_two_images.token.frame &&
           xfi_release(&stereo_two_images,&stereo_xr_output,panel,&stereo_two.owner)!=EFO_OK)return 0;
        XrFrameEndInfo end={.type=XR_TYPE_FRAME_END_INFO,.displayTime=d->current_state.predictedDisplayTime,
            .environmentBlendMode=XR_ENVIRONMENT_BLEND_MODE_OPAQUE};
        if(XR_SUCCEEDED(stereo_two_frame_end(&end,0)) && stereo_two_images.token.frame)
            (void)xfi_retire(&stereo_two_images,&stereo_xr_output,1);
    }
    return 0; /* retry after real balance and worker exit */
}
static int stereo_frame_abort_required(void *opaque) {
    PanelXr *p=opaque;
    return stereo_two.enabled && stereo_two_xr.thread && stereo_two_xr.fault &&
        stereo_two_xr.api.prefetch.session==p->session;
}
static void stereo_frame_abort_input(void) {
#ifdef GF2VR_CONTROLLER_GAME_MODE
    controller_game_runtime_cancel();
#endif
#ifdef GF2VR_CONTROLLER_INPUT
    controller_runtime_cancel(); /* local OS/input release, no XR frame call */
#endif
#ifdef GF2VR_WORLD_DRAG
    stereo_world_drag_reset();
#endif
#ifdef GF2VR_THUMBSTICK_NAVIGATION
    stereo_navigation_cancel();
#endif
}
static int stereo_output_stop_session(void *opaque) {
    PanelXr *panel=opaque;
#ifdef GF2VR_BANK_PRESENTATION
    if(stereo_bank_enabled && stereo_two.enabled)return stereo_bank_stop_session(panel);
#endif
    if(!stereo_two.enabled)return stereo_output_xr_close(panel);
    stereo_frame_abort_input();
    XrEyeFrameDriver *d=&stereo_two_xr;
    if(!d->thread)return !stereo_two_images.token.frame;
    if(d->api.prefetch.session!=panel->session || d->thread!=GetCurrentThreadId())return 0;
    d->stopping=1;
    int r;do {r=eye_flight_stop(d->owner);if(r==EFO_BUSY)Sleep(1);} while(r==EFO_BUSY);
    if(r!=EFO_OK)return 0;
    /* No Begin/End balance is required just to stop a session. In particular
       a hard frame-call error cannot be repaired with another guessed End.
       Never call EndSession while the wait worker still uses the session. */
    while(!xr_frame_prefetch_quiesce(&d->api.prefetch))Sleep(1);
    if(stereo_two_images.token.frame) {
        do {r=xfi_release(&stereo_two_images,&stereo_xr_output,panel,d->owner);
            if(r==EFO_BUSY || (r==EFO_REJECTED && !stereo_two_images.fault))Sleep(1);
        } while(r==EFO_BUSY || (r==EFO_REJECTED && !stereo_two_images.fault));
        if(r!=EFO_OK)return 0; /* real image/GPU fault remains quarantined */
#ifdef GF2VR_ASYNC_PRESENTATION
        if(stereo_async_enabled && stereo_two_images.detached) {
            do {r=stereo_async_drain(panel);if(r!=EFO_OK && !stereo_two_images.fault)Sleep(1);}
            while(r!=EFO_OK && !stereo_two_images.fault);
            if(r!=EFO_OK)return 0;
        }
#endif
    }
    return 1;
}
static int stereo_output_session_stopped(void *opaque) {
    PanelXr *panel=opaque;
#ifdef GF2VR_BANK_PRESENTATION
    if(stereo_bank_enabled && stereo_two.enabled)return stereo_bank_session_stopped(panel);
#endif
    if(!stereo_two.enabled)return 1;
    XrEyeFrameDriver *d=&stereo_two_xr;
    if(!d->thread)return !stereo_two_images.token.frame;
    if(panel->running || d->api.prefetch.session!=panel->session)return 0;
    EyeFlightToken terminal[EFO_CAPACITY];
    int r;do {r=xfd_session_stopped(d,terminal);if(r==EFO_BUSY)Sleep(1);} while(r==EFO_BUSY);
    if(r!=EFO_OK)return 0;
    if(stereo_two_images.token.frame && xfi_retire(&stereo_two_images,&stereo_xr_output,1)!=EFO_OK)return 0;
    /* Only XR-only packets are cleaned here; main owns every reserved source.
       GPU/CPU flags remain unchanged and gate its later native retirement. */
    for(unsigned i=0;i<EFO_CAPACITY;++i) {
        EyeFlightToken token=terminal[i];
        if(!token.frame)continue;
        EyeFlightSnapshot s;
        do {r=eye_flight_snapshot(d->owner,token,&s);if(r==EFO_BUSY)Sleep(1);} while(r==EFO_BUSY);
        if(r==EFO_REJECTED)continue; /* main already retired its reserved slot */
        if(r!=EFO_OK)return 0;
        if(s.flags&EFF_RESERVED)continue;
        if(!(s.flags&EFF_RECLAIMING)) {
            do {r=eye_flight_take(d->owner,token,&s);if(r==EFO_BUSY)Sleep(1);} while(r==EFO_BUSY);
            if(r!=EFO_OK)return 0;
        }
        do {r=eye_pose_pool_retire(d->poses,d->owner,token);if(r==EFO_BUSY)Sleep(1);} while(r==EFO_BUSY);
        if(r!=EFO_OK)return 0;
    }
    return stereo_output_xr_close(panel);
}
static int stereo_two_arm(void);
static void stereo_two_root_finished(U64,int);
static void stereo_two_owner_tick(void);
static void stereo_two_render_tick(void);
static int stereo_two_before_original(void *,void *,void *,void *);
static int stereo_draw_current(U64,DrawQueueIdentity *);
#if defined(GF2VR_MAIN_PHASE_ROLES) || defined(GF2VR_ACTIVITY_STACK_OBSERVE)
#include "main_phase_roles.h"
#ifdef GF2VR_MAIN_PHASE_DISPATCH
#include "main_phase_dispatch.h"
#endif
#include "main_phase_roles_log.h"
#endif
#ifdef GF2VR_ACTIVITY_STACK_OBSERVE
#include "camera_stack_managed.h"
#include "activity_stack_plan.h"
#include "activity_stack_observe.h"
#endif
#ifdef GF2VR_ACTIVITY_UI_OBSERVE
#include "activity_ui_observe.h"
#endif
/* Attach only a fully bound stable owner; enabling still belongs to the coherent
   render/Submit/XR configuration transaction, not success of managed binding. */
static int stereo_live_attach(SourceLiveOwner *o) {
    if(!o || (o->pair_count!=1 && !(o->pair_count==EFO_CAPACITY && stereo_two.enabled)) ||
       !o->ready || o->busy || o->retiring || o->thread!=GetCurrentThreadId() ||
       o->thread!=stereo_pipeline.owner_thread || o->generation.publication!=&stereo_publication_generation ||
       o->mailbox!=&stereo_eye_runtime.mailbox || o->generation.generation_clock!=&stereo_generation_clock || stereo_live || stereo_source.enabled ||
       stereo_source.generation || stereo_source_frame.serial || stereo_pipeline.render.active ||
       stereo_output.busy || stereo_output.reserved.serial || stereo_output.submitted.serial || stereo_output.completed.serial ||
       __atomic_load_n(&stereo_output_event.state,__ATOMIC_ACQUIRE)!=EEC_IDLE ||
       __atomic_load_n(&stereo_interval.state,__ATOMIC_ACQUIRE)!=ERI_IDLE) return 0;
    stereo_source.pipeline=o->pipeline;stereo_source.method=o->method;
    stereo_source.data=o->pair_count==1?&o->data:0;stereo_source.selection=&o->generation.selection;
    stereo_source.reader=&o->reader;stereo_source.anchor=&o->anchor;stereo_source.math=&o->math;
    stereo_source.mailbox=o->mailbox;stereo_source.batch=&o->batch;
    stereo_source.generation=&o->generation;stereo_source.topology=&o->topology;
    stereo_source.manual=&o->manual;stereo_source.preparation=o->pair_count==1?&o->preparation:0;
    stereo_source.eye_width=o->width;stereo_source.eye_height=o->height;
    stereo_output.native=o->native;stereo_live=o;return 1;
}
static int stereo_live_detach(void) {
    if(!stereo_live) return 1;
    if(GetCurrentThreadId()!=stereo_live->thread) return 0;
    stereo_source.enabled=0;main_test_revoke(&stereo_live->manual);
    source_generation_invalidate(&stereo_live->generation);
    if(stereo_live->pair_count==EFO_CAPACITY) {
        (void)eye_flight_stop(&stereo_two.owner);
        stereo_two_owner_tick();
        for(unsigned i=0;i<EFO_CAPACITY;++i)if(stereo_two.slots[i].admitted.token.frame ||
            stereo_live->flights.slots[i].occupied)return 0;
    }
    if(stereo_pipeline.render.active || stereo_source_frame.serial || stereo_output.busy ||
       stereo_output.reserved.serial || stereo_output.submitted.serial || stereo_output.completed.serial ||
       __atomic_load_n(&stereo_output_event.state,__ATOMIC_ACQUIRE)!=EEC_IDLE ||
       __atomic_load_n(&stereo_interval.state,__ATOMIC_ACQUIRE)!=ERI_IDLE) return 0;
    if(!source_live_close(stereo_live,GetCurrentThreadId())) return 0;
    __builtin_memset(&stereo_source,0,sizeof(stereo_source));
    stereo_output.native=(EyeNativeApi){0};stereo_live=0;return 1;
}
/* Hook configuration supplies the exact verified method before requesting
   startup. Capture only roots the current pipeline; managed binding/attachment
   happens at the next owner tick outside the active Render root. */
static SourceLiveOwner stereo_start_owner;
static StereoHooks stereo_hooks;
#if (defined(GF2VR_SUBMISSION_OBSERVE) || defined(GF2VR_MAIN_STEREO)) && !defined(BOUNDARY_TEST)
static unsigned stereo_hooks_permitted=1; /* Build mode controls startup/source permission. */
#else
static unsigned stereo_hooks_permitted;
#endif
static struct {
    unsigned requested,busy,failed,root;
    void *method,*pipeline;
    void (*unpin)(unsigned);
} stereo_start;
#ifdef GF2VR_COMPOSITION_OBSERVE
#include "composition_runtime_diag.h"
#endif
#ifdef BOUNDARY_TEST
static int (*stereo_start_test_step)(SourceLiveOwner *,unsigned);
static int (*stereo_start_test_close)(PoseMathBinding *);
#endif
static void stereo_start_capture(void *pipeline,void *method) {
    if(!stereo_start.requested || stereo_start.failed || stereo_start.busy || stereo_start.root ||
       method!=stereo_start.method || !pipeline || !stereo_pipeline.render.active ||
       GetCurrentThreadId()!=stereo_pipeline.owner_thread || !sa.gchandle_new || !sa.gchandle_free) return;
    if(stereo_live && stereo_live->pipeline==pipeline && stereo_live->method==method) return;
    stereo_start.busy=1;
    stereo_start.root=sa.gchandle_new(pipeline,1);
    if(stereo_start.root) {stereo_start.pipeline=pipeline;stereo_start.unpin=sa.gchandle_free;}
    else stereo_start.failed=1;
    if(stereo_live) {
        stereo_source.enabled=0;main_test_revoke(&stereo_live->manual);
        source_generation_invalidate(&stereo_live->generation);
    }
    stereo_start.busy=0;
}
static void stereo_start_tick(void) {
    if(GetCurrentThreadId()!=stereo_pipeline.owner_thread || stereo_start.busy || stereo_pipeline.render.active) return;
    DWORD saved=GetLastError();
    unsigned width=0,height=0;xr_eye_size_read(&stereo_xr_size,&width,&height);
    stereo_start.busy=1;
    if(stereo_live && (!stereo_start.requested || stereo_start.root || !width ||
       stereo_live->width!=(int)width || stereo_live->height!=(int)height)) {
        if(!stereo_live_detach()) goto end;
    }
    if(stereo_start_owner.thread && !stereo_live &&
       !source_live_close(&stereo_start_owner,GetCurrentThreadId())) goto end;
    if(!stereo_start.requested || stereo_start.failed) goto release;
    if(!stereo_start.root || !width || !height || stereo_live) goto end;
    int opened;
#ifdef BOUNDARY_TEST
    if(stereo_start_test_step) opened=(stereo_two.enabled?source_live_open_two_with:source_live_open_with)(&stereo_start_owner,GetCurrentThreadId(),
        stereo_start.pipeline,stereo_start.method,&stereo_eye_runtime.mailbox,&stereo_publication_generation,
        &stereo_generation_clock,(int)width,(int)height,1.0f,stereo_start_test_step,stereo_start_test_close);
    else
#endif
    opened=(stereo_two.enabled?source_live_open_two:source_live_open)(&stereo_start_owner,GetCurrentThreadId(),stereo_start.pipeline,stereo_start.method,
        &stereo_eye_runtime.mailbox,&stereo_publication_generation,&stereo_generation_clock,(int)width,(int)height,1.0f);
    /* 1 Unity unit/metre is an explicit trial value, not calibrated acceptance. */
    unsigned latest_width,latest_height;
    xr_eye_size_read(&stereo_xr_size,&latest_width,&latest_height);
    if(!opened) stereo_start.failed=1;
    if(opened && (latest_width!=width || latest_height!=height || !stereo_start.requested ||
                  !stereo_live_attach(&stereo_start_owner)))
        (void)source_live_close(&stereo_start_owner,GetCurrentThreadId());
    if(opened && stereo_live==&stereo_start_owner && stereo_two.enabled)
        event("two_flight_bound",stereo_live->pair_count,GetCurrentThreadId());
release:
    if(stereo_start.root) {
        unsigned root=stereo_start.root;stereo_start.root=0;stereo_start.pipeline=0;
        stereo_start.unpin(root);
    }
end:
    stereo_start.busy=0;SetLastError(saved);
}
static void stereo_manual_revoke(void) {
    if(stereo_source.manual) main_test_revoke(stereo_source.manual);
    if(stereo_source.generation && stereo_source.generation->owner_thread==GetCurrentThreadId())
        source_generation_invalidate(stereo_source.generation);
    __atomic_store_n(&stereo_publication_generation,0,__ATOMIC_RELEASE);
}
/* Optional test input only, on the Unity owner thread; never consumes keys or
   installs an input hook. Resolve user32 exports without adding a loader dep. */
#ifdef BOUNDARY_TEST
static void (*stereo_manual_test_input)(int *,int *);
#endif
#ifdef GF2VR_AUTO_STEREO
#include "auto_stereo_policy.h"
static AutoStereoIntent stereo_auto_intent;
#endif
static void stereo_manual_tick(void) {
    if(!stereo_source.manual || GetCurrentThreadId()!=stereo_pipeline.owner_thread) return;
    DWORD saved=GetLastError();
    /* ExecuteTasks callbacks are not render opportunities. Pending input is
       consumed/rejected by the next root; another callback cannot erase it. */
    HINSTANCE user=GetModuleHandleW(L"user32.dll");
    short (WINAPI *key)(int)=user?(void *)GetProcAddress(user,"GetAsyncKeyState"):0;
    void *(WINAPI *foreground)(void)=user?(void *)GetProcAddress(user,"GetForegroundWindow"):0;
    DWORD (WINAPI *process)(void *,DWORD *)=user?(void *)GetProcAddress(user,"GetWindowThreadProcessId"):0;
    DWORD pid=0;void *window=foreground?foreground():0;
    if(window && process) process(window,&pid);
    int focused=stereo_source.enabled && key && pid==GetCurrentProcessId();
    int down=focused && (((unsigned short)key(0x77)&0x8000u)!=0);
#ifdef BOUNDARY_TEST
    if(stereo_manual_test_input) stereo_manual_test_input(&focused,&down);
#endif
    unsigned before=stereo_source.manual->state;
#ifdef GF2VR_AUTO_STEREO
    if(stereo_auto_enabled) {
        if(auto_stereo_intent_update(&stereo_auto_intent,stereo_source.manual,
                                    stereo_source.enabled,focused,down))
            event("auto_stereo_requested",before,GetCurrentThreadId());
    } else main_test_key(stereo_source.manual,focused,down);
#else
    main_test_key(stereo_source.manual,focused,down);
#endif
    if(stereo_source.manual->state==MTG_OFF) {
        if(!stereo_source.manual->flat_tracking)stereo_manual_revoke();
        else {
            if(stereo_source.generation && stereo_source.generation->owner_thread==GetCurrentThreadId())
                source_generation_invalidate(stereo_source.generation);
            __atomic_store_n(&stereo_publication_generation,0,__ATOMIC_RELEASE);
        }
    }
#ifdef GF2VR_MAIN_STEREO
    static int prior_focus=-1,prior_down=-1;
    if(prior_focus!=focused || prior_down!=down || before!=stereo_source.manual->state) {
        event("main_f8_transition",(U64)focused|((U64)down<<1)|((U64)before<<8)|
            ((U64)stereo_source.manual->state<<16),GetCurrentThreadId());
        prior_focus=focused;prior_down=down;
    }
#else
    (void)before;
#endif
    SetLastError(saved);
}
/* Unity2019 D3D RenderTexture rows use the vertically flipped texture
   convention. CopyResource preserves those rows. OpenXR XrFovf permits reversed
   vertical endpoints to describe flipped view content. Adapt only the output
   descriptor: preserve runtime calibration, producer matrices and physical pose.
   Swapping, rather than negating, retains asymmetric angular coverage exactly. */
static XrFovf stereo_unity_texture_fov(XrFovf runtime) {
    XrFovf output=runtime;
#ifndef GF2VR_UPRIGHT_TRANSFER
    output.angleUp=runtime.angleDown;output.angleDown=runtime.angleUp;
#endif
    return output;
}
#ifdef GF2VR_AUX_OBSERVE
#include "aux_observe.h"
#endif
#ifdef GF2VR_OVR_DESCRIPTOR
#include "ovr_descriptor_observe.h"
#endif
#if defined(GF2VR_AUX_ISOLATE) || defined(GF2VR_AUX_ALPHA) || defined(GF2VR_AUX_FOV)
#include "aux_isolate.h"
#endif
#ifdef GF2VR_MAIN_TRANSITION_HOLD
#include "transition_hold.h"
#ifdef GF2VR_PRESENTATION_CACHE
#include "completed_presentation.h"
#endif
#ifdef GF2VR_PERF_OBSERVE
/* XR-owner-only bounded timing summary. No new waits, hooks, setters or GPU
   queries. Packed32-bit fields: frame count/elapsed, wait/copy, loop/end. */
#ifdef GF2VR_BANK_END_OBSERVE
#include "end_boundary_runtime.h"
#endif
static void stereo_pacing_record(U64 start,long long period,U64 wait,U64 copy,U64 end,int render) {
#ifdef GF2VR_TOUCH_PRIVATE
    touch_p2_xr(period,render);
#endif
#line 1390
#ifdef GF2VR_COPY_GPU_OBSERVE
    if(stereo_pacing_fresh)copy_gpu_observe_arm();
#endif
#ifdef GF2VR_COPY_PRESENT_OBSERVE
    if(stereo_pacing_fresh)copy_present_observe_arm();
#endif
    static U64 tick,frames,wait_total,copy_total,end_total,loop_total,period_us,render_frames;
    static unsigned reports;
    if(reports>=40)return;
    U64 now=GetTickCount64();if(!tick)tick=start;
    if(now<start||now<tick||period<=0||period>1000000000LL)return;
    ++frames;wait_total+=wait;copy_total+=copy;end_total+=end;loop_total+=now-start;
    period_us+=(U64)period/1000;render_frames+=render!=0;
    if(now-tick<2000)return;
    event("perf_xr_frames_elapsed",(frames<<32)|(now-tick),GetCurrentThreadId());
    event("perf_xr_wait_copy_ms",(wait_total<<32)|copy_total,GetCurrentThreadId());
    event("perf_xr_loop_end_ms",(loop_total<<32)|end_total,GetCurrentThreadId());
    event("perf_xr_period_us",period_us/frames,GetCurrentThreadId());
    event("perf_xr_fresh_render",(stereo_pacing_fresh<<32)|render_frames,GetCurrentThreadId());
#ifdef GF2VR_BANK_CAPACITY_READMIT
    event("perf_bank_capacity_readmit",stereo_two_xr.capacity_readmits,GetCurrentThreadId());
    stereo_two_xr.capacity_readmits=0;
#endif
#ifdef GF2VR_READY_RECEIPT_READMIT
    event("perf_bank_ready_readmit",stereo_two_xr.ready_readmits,GetCurrentThreadId());
    stereo_two_xr.ready_readmits=0;
#endif
#ifdef GF2VR_INDEPENDENT_READY_CAPACITY
    event("perf_bank_ready_writing_readmit",stereo_two_xr.ready_writing_readmits,GetCurrentThreadId());
    stereo_two_xr.ready_writing_readmits=0;
#endif
#line 1395
#ifdef GF2VR_XR_SUBMISSION_ACK
    event("perf_xr_submission_ack",stereo_two.backend.xr_submission_acks,GetCurrentThreadId());
    stereo_two.backend.xr_submission_acks=0;
#endif
#ifdef GF2VR_MAIN_TAIL_COMPLETION
    event("perf_main_tail_copy_ack",__atomic_exchange_n(&stereo_two.backend.main_tail_copy_acks,0,
        __ATOMIC_RELAXED),GetCurrentThreadId());
#endif
#ifdef GF2VR_PERF_END_SPLIT
    event("perf_xr_eye_wait_runtime_end_ms",(stereo_pacing_eye_wait_ms<<32)|stereo_pacing_runtime_end_ms,GetCurrentThreadId());
    stereo_pacing_eye_wait_ms=stereo_pacing_runtime_end_ms=0;
#endif
#ifdef GF2VR_BANK_END_OBSERVE
    ebo_report(now);
#endif
    ++reports;tick=now;frames=wait_total=copy_total=end_total=loop_total=period_us=render_frames=0;
    stereo_pacing_fresh=0;
}
#endif
static unsigned stereo_hold_focused;
static void stereo_hold_session_state(int state) {
    stereo_hold_focused=state==XR_SESSION_STATE_FOCUSED;
#ifdef GF2VR_AUTO_STEREO
    stereo_auto_xr_focused=stereo_hold_focused;
#endif
    if(!stereo_hold_focused)stereo_hold.valid=0;
#ifdef GF2VR_PRESENTATION_CACHE
    if(!stereo_hold_focused)completed_presentation_cancel(&stereo_completed);
#endif
#ifdef GF2VR_WORLD_DRAG
    if(!stereo_hold_focused)stereo_world_drag_reset();
#endif
#ifdef GF2VR_CONTROLLER_INPUT
    if(!stereo_hold_focused)controller_runtime_cancel();
#ifdef GF2VR_CONTROLLER_GAME_MODE
    if(!stereo_hold_focused)controller_game_runtime_cancel();
#endif
#endif
}
#endif
#ifdef GF2VR_AUX_PANEL
static int stereo_await_eye_offer(int visible,int input_valid,int had_projection) {
#ifdef GF2VR_PERF_ASYNC_EYE_WAIT
    (void)had_projection;
    return visible && input_valid;
#else
    return visible && input_valid && had_projection;
#endif
}
#ifdef GF2VR_PERF_FENCE_OBSERVE
#include "perf_fence_observer.h"
static PerfFenceObserver stereo_fence_observer;
static unsigned stereo_fence_records;
static PerfFenceRecord stereo_fence_report;
static unsigned stereo_fence_report_ready;
#endif
#ifdef GF2VR_ENTRY_BOOTSTRAP
static int stereo_eye_producer_pending(const XrEyeInput *input) {
    if(!__atomic_load_n(&stereo_publication_generation,__ATOMIC_ACQUIRE) ||
       !xr_eye_mailbox_current(&stereo_eye_runtime.mailbox,input)) return 0;
    unsigned state=__atomic_load_n(&stereo_xr_output.state,__ATOMIC_ACQUIRE);
    if(stereo_xr_output.frame==input->frame_id)
        return state==XEO_OFFERED || state==XEO_CLAIMED;
    /* Before the first numeric request there is no acquired eye/frame.
       Preserve this input opportunity until Unity publishes the request;
       XR must then leave EndFrame to drive that request on the next frame.
       Only the atomic state is read, never Unity's mutable description. */
    if(stereo_xr_output.frame || state!=XEO_IDLE) return 0;
    unsigned config=__atomic_load_n(&stereo_xr_config.state,__ATOMIC_ACQUIRE);
    return config==XEC_IDLE || config==XEC_WRITING;
}
#endif
#endif
#include "two_flight_observe.h"
static int stereo_two_eye_pending(PanelXr *panel) {
    (void)stereo_two_xr_prefetch(); /* only Wait/capture B, never Begin B */
    EyeFlightToken token=stereo_two_xr.current_token;
    if(!token.frame)return 0;
    EyeFlightSnapshot s;int r=eye_flight_snapshot(&stereo_two.owner,token,&s);
    TWO_WAIT_NOTE(1,r,s);
    if(r==EFO_BUSY)return 1;
    if(r!=EFO_OK || s.flags&EFF_FAULT)return 0;
    if(s.flags&EFF_COPY_ISSUED){
        TWO_WAIT_NOTE(2,EFO_OK,s);
#ifdef GF2VR_TWO_FLIGHT_EARLY_COPY
        return (s.flags&(EFF_COPY_DRAINED|EFF_RENDER_DRAINED))!=(EFF_COPY_DRAINED|EFF_RENDER_DRAINED);
#else
        return !(s.flags&EFF_COPY_DRAINED);
#endif
    }
    if(s.flags&EFF_CANCELLED || !stereo_xr_output.enabled ||
       __atomic_load_n(&stereo_publication_generation,__ATOMIC_ACQUIRE)!=token.generation){TWO_WAIT_NOTE(3,EFO_REJECTED,s);return 0;}
    unsigned ready=EFF_RENDER_ISSUED|EFF_CPU_RETURNED;
#ifdef GF2VR_PRODUCER_TRANSFER
    if((s.flags&EFF_PRIVATE_PRODUCER) && !(s.flags&EFF_CPU_RETURNED)) {
        r=xfi_copy_tick(&stereo_two_images,&stereo_xr_output,panel,&stereo_two.owner,&stereo_two.backend,token);
        TWO_WAIT_NOTE(6,r,s);return r==EFO_BUSY || r==EFO_REJECTED;
    }
#endif
#ifdef GF2VR_TWO_FLIGHT_EARLY_COPY
    if(!(s.flags&(EFF_RENDER_SUBMITTED|EFF_RENDER_DRAINED))){TWO_WAIT_NOTE(4,EFO_BUSY,s);return 1;}
#else
    ready|=EFF_RENDER_DRAINED;
#endif
    if((s.flags&ready)!=ready){TWO_WAIT_NOTE(4,EFO_BUSY,s);return 1;}
    if(!s.cpu_ok){TWO_WAIT_NOTE(5,EFO_REJECTED,s);return 0;}
    r=xfi_copy_tick(&stereo_two_images,&stereo_xr_output,panel,&stereo_two.owner,&stereo_two.backend,token);
    TWO_WAIT_NOTE(6,r,s);
    return r==EFO_BUSY;
}
#ifdef GF2VR_ASYNC_PRESENTATION
/* XR-only completion pump. At most one API-image owner; other numeric flights
   never borrow its handles. No GPU wait, synthetic drain or newer-pose rewrite.
   Main/render already progress the original backend's exact completion events. */
#ifdef GF2VR_ASYNC_COMPLETION_OBSERVE
/* XR-only numeric observations reuse the pump's existing snapshot. No extra
   owner snapshot, backend-thread reads, GPU call, wait or completion decision. */
static void stereo_async_note(EyeFlightToken token,const EyeFlightSnapshot *s,unsigned phase,int result) {
    static unsigned records,last_phase,last_flags,last_state;static int last_result;
    static U64 last_frame,last_bucket;
    if(records>=128 || !token.frame)return;
    U64 now=GetTickCount64(),age=0;
    if(xfi_same(stereo_two_images.token,token) && stereo_two_images.prepared_tick &&
       now>=stereo_two_images.prepared_tick)age=now-stereo_two_images.prepared_tick;
    unsigned state=__atomic_load_n(&stereo_xr_output.state,__ATOMIC_ACQUIRE);
    unsigned flags=s?s->flags:0;U64 bucket=age/25;
    if(last_frame==token.frame && last_phase==phase && last_result==result &&
       last_flags==flags && last_state==state && last_bucket==bucket)return;
    last_frame=token.frame;last_phase=phase;last_result=result;last_flags=flags;
    last_state=state;last_bucket=bucket;++records;
    DWORD saved=GetLastError();char line[1024],*p=source_prefix(line,"async_completion_boundary");
    p=literal(p,",\"current\":");p=decimal(p,stereo_two_xr.current);
    p=literal(p,",\"flight\":");p=decimal(p,token.frame);
    p=literal(p,",\"generation\":");p=decimal(p,token.generation);
    p=literal(p,",\"epoch\":");p=decimal(p,token.epoch);
    p=literal(p,",\"serial\":");p=decimal(p,s?s->ticket.serial:0);
    p=literal(p,",\"flags\":");p=decimal(p,flags);
    p=literal(p,",\"phase\":");p=decimal(p,phase);
    p=literal(p,",\"result\":");p=decimal(p,(U64)(long long)result);
    p=literal(p,",\"grant_state\":");p=decimal(p,state);
    p=literal(p,",\"prepared_age_ms\":");p=decimal(p,age);
    p=literal(p,",\"released_mask\":");p=decimal(p,stereo_two_images.actual_released);
    p=literal(p,"}\n");source_log(line,p);SetLastError(saved);
}
#define ASYNC_PUMP_RETURN(phase,value) do {int observed_result=(value);stereo_async_note(token,&s,phase,observed_result);return observed_result;} while(0)
#else
#define ASYNC_PUMP_RETURN(phase,value) return (value)
#endif
static int stereo_async_pump(PanelXr *panel,const XrFrameEndInfo *end,int visible) {
    EyeFlightToken token=stereo_two_images.token.frame?stereo_two_images.token:stereo_two_xr.current_token;
    if(!token.frame)return EFO_REJECTED;
    EyeFlightSnapshot s={0};int r=eye_flight_snapshot(&stereo_two.owner,token,&s);if(r!=EFO_OK)ASYNC_PUMP_RETURN(1,r);
    async_throughput_record(7,token,s.ticket.serial,s.flags,r,
        __atomic_load_n(&stereo_xr_output.state,__ATOMIC_ACQUIRE));
    if(s.flags&EFF_FAULT) {
        stereo_two_xr.fault=stereo_two_xr.stopping=1;
        completed_presentation_cancel(&stereo_completed);stereo_hold.valid=0;
        (void)eye_flight_stop(&stereo_two.owner);
        if(stereo_two_images.token.frame)(void)xfi_fault(&stereo_two_images,&stereo_xr_output,&stereo_two.owner);
        ASYNC_PUMP_RETURN(2,EFO_REJECTED);
    }
    int stale=s.flags&(EFF_CANCELLED|EFF_RECLAIMING);
    stale=stale || !visible || !stereo_hold_focused || !stereo_xr_output.enabled ||
        __atomic_load_n(&stereo_publication_generation,__ATOMIC_ACQUIRE)!=token.generation ||
        stereo_eye_runtime.owner.epoch!=token.epoch ||
        stereo_eye_runtime.owner.head_reference_epoch!=token.epoch ||
        stereo_eye_runtime.owner.changing || stereo_eye_runtime.owner.retiring;
    if(s.flags&EFF_DISPLAY_DETACHED)stale=stale || end->displayTime<=s.display_time ||
        (U64)end->displayTime-(U64)s.display_time>=250000000ull;
    if((s.flags&EFF_PRODUCER_RETAINED) && stereo_two_images.prepared_tick) {
        U64 now=GetTickCount64();
        stale=stale || now<stereo_two_images.prepared_tick || now-stereo_two_images.prepared_tick>=250;
    }
    if(stale) {
        if(!stereo_two_images.token.frame)ASYNC_PUMP_RETURN(3,EFO_REJECTED);
        stereo_async_discard=1;
        ASYNC_PUMP_RETURN(3,stereo_async_drain(panel));
    }
    unsigned ready=EFF_RENDER_ISSUED|EFF_CPU_RETURNED;
#ifdef GF2VR_PRODUCER_TRANSFER
    int producer_ready=(s.flags&EFF_PRIVATE_PRODUCER) &&
        (!(s.flags&EFF_CPU_RETURNED) || s.cpu_ok);
    if(!producer_ready && ((s.flags&ready)!=ready || !s.cpu_ok))ASYNC_PUMP_RETURN(4,EFO_BUSY);
#else
    if((s.flags&ready)!=ready || !s.cpu_ok)ASYNC_PUMP_RETURN(4,EFO_BUSY);
#endif
    if(!stereo_two_images.token.frame) {
        r=xfi_prepare(&stereo_two_images,&stereo_xr_output,panel,&stereo_two.owner,token);
        if(r!=EFO_OK)ASYNC_PUMP_RETURN(5,r);
    }
    /* Reservation can precede render submission; it does not prove GPU readiness. */
#ifdef GF2VR_PRODUCER_TRANSFER
    if(!producer_ready && !(s.flags&(EFF_RENDER_SUBMITTED|EFF_RENDER_DRAINED)))ASYNC_PUMP_RETURN(6,EFO_BUSY);
#else
    if(!(s.flags&(EFF_RENDER_SUBMITTED|EFF_RENDER_DRAINED)))ASYNC_PUMP_RETURN(6,EFO_BUSY);
#endif
    ASYNC_PUMP_RETURN(7,xfi_copy_tick(&stereo_two_images,&stereo_xr_output,panel,&stereo_two.owner,&stereo_two.backend,token));
}
#undef ASYNC_PUMP_RETURN
#endif
static int stereo_two_eye_projection(PanelXr *panel,XrEyeInput *input,
        const XrFrameEndInfo *end,XrCompositionLayerProjection *projection,
        XrCompositionLayerProjectionView views[2]) {
    if(!stereo_two_images.token.frame)return EFO_REJECTED;
    EyeFlightSnapshot s;int r=eye_flight_snapshot(&stereo_two.owner,stereo_two_images.token,&s);
    if(r!=EFO_OK)return r;
    /* The source wait deadline is not a copy-completion/release timeout. */
    if((s.flags&EFF_COPY_ISSUED) && !(s.flags&EFF_COPY_DRAINED))return EFO_BUSY;
#ifdef GF2VR_ASYNC_PRESENTATION
    if(stereo_async_enabled && s.flags&EFF_DISPLAY_DETACHED) {
        XrEyeInput original;EyeFlightSnapshot exact;
        r=eye_pose_pool_read(&stereo_two.poses,&stereo_two.owner,stereo_two_images.token,&exact,&original);
        if(r!=EFO_OK)return r;
        r=xfi_detached_projection(&stereo_two_images,&stereo_xr_output,panel,&stereo_two.owner,&original,
            stereo_eye_runtime.owner.space,end,&stereo_publication_generation,projection,views);
        if(r==EFO_OK)*input=original;
        return r;
    }
#endif
    return xfi_projection(&stereo_two_images,&stereo_xr_output,panel,&stereo_two.owner,input,
        stereo_eye_runtime.owner.space,end,&stereo_publication_generation,projection,views);
}
static XrResult stereo_output_xr_end(void *opaque,const XrFrameEndInfo *end,int visible) {
#ifdef GF2VR_BANK_PRESENTATION
    if(stereo_bank_enabled && stereo_two.enabled)return stereo_bank_end(opaque,end,visible);
#endif
#ifdef GF2VR_OVR_DESCRIPTOR
    DWORD ovr_saved_error=GetLastError();int installed=ovr_desc_install();
    if(installed)event("ovr_descriptor_hook",(U64)(long long)installed,GetCurrentThreadId());
    SetLastError(ovr_saved_error);
#endif
    PanelXr *panel=opaque;
    XrFrameEndInfo nonvisual;
    if(stereo_two.enabled && !xr_ordered_frames_visual(&stereo_two_xr.api,stereo_two_xr.current)) {
        nonvisual=*end;nonvisual.layerCount=0;nonvisual.layers=0;end=&nonvisual;visible=0;
    }
    XrCompositionLayerProjection projection={0};XrCompositionLayerProjectionView views[2]={0};
    XrEyeInput input={0};
#ifdef GF2VR_TWO_FLIGHT_OBSERVE
    stereo_two_wait_note=(TwoFlightWaitNote){0};
    int two_projection_result=EFO_REJECTED;
    U64 two_wait_start=GetTickCount64();
#endif
    if(visible) (void)xr_eye_mailbox_read(&stereo_eye_runtime.mailbox,&input);
#ifdef GF2VR_ASYNC_PRESENTATION
    int async_late=stereo_async_enabled && stereo_two_images.detached;
    U64 async_prepared_tick=stereo_two_images.prepared_tick;
    if(stereo_two.enabled && stereo_async_enabled)(void)stereo_async_pump(panel,end,visible);
    if(stereo_async_enabled && (stereo_two_images.fault || stereo_two_xr.fault)) {
        stereo_two_xr.fault=stereo_two_xr.stopping=1;return XR_ERROR_RUNTIME_FAILURE;
    }
#endif
    if(stereo_two.enabled &&
       (stereo_two_xr.current_token.frame || __atomic_load_n(&stereo_publication_generation,__ATOMIC_ACQUIRE))) {
        U64 start=GetTickCount64(),deadline=start+100;
#ifdef GF2VR_ASYNC_PRESENTATION
        if(stereo_async_enabled) {
            /* Preserve the bounded first-entry CPU producer opportunity only.
               Once a bank is prepared, End need not wait for either GPU fence. */
            while(!stereo_completed.valid && !stereo_two_images.token.frame && GetTickCount64()<deadline &&
                  stereo_async_pump(panel,end,visible)==EFO_BUSY)Sleep(1);
            async_late=stereo_two_images.detached;async_prepared_tick=stereo_two_images.prepared_tick;
        } else
#endif
            while(GetTickCount64()<deadline && stereo_two_eye_pending(panel))Sleep(1);
#ifdef GF2VR_PERF_END_SPLIT
        stereo_pacing_eye_wait_ms+=GetTickCount64()-start;
#endif
    }
#ifdef GF2VR_AUX_PANEL
    /* An async panel copy no longer provides the first Unity producer
       opportunity. The offered eye must receive the same bounded wait even
       before last_projection becomes true, or first stereo can never start. */
    if(!stereo_two.enabled && stereo_await_eye_offer(visible,input.valid,stereo_aux.last_projection)) {
#ifdef GF2VR_PERF_END_SPLIT
        U64 eye_wait_start=GetTickCount64();
#endif
        U64 deadline=GetTickCount64()+100;
#ifdef GF2VR_ENTRY_BOOTSTRAP
        while(GetTickCount64()<deadline && stereo_eye_producer_pending(&input)) {
#else
        while(__atomic_load_n(&stereo_publication_generation,__ATOMIC_ACQUIRE) &&
              stereo_xr_output.frame==input.frame_id && GetTickCount64()<deadline) {
            unsigned state=__atomic_load_n(&stereo_xr_output.state,__ATOMIC_ACQUIRE);
            if(state!=XEO_OFFERED && state!=XEO_CLAIMED) break;
#endif
#ifdef GF2VR_PERF_FENCE_OBSERVE
            perf_fence_sample(&stereo_fence_observer);
#endif
#ifdef GF2VR_PERF_FENCE_DELIVERY
            stereo_output_fence_tick();
#endif
            Sleep(1);
        }
#ifdef GF2VR_PERF_END_SPLIT
        stereo_pacing_eye_wait_ms+=GetTickCount64()-eye_wait_start;
#endif
    }
    XrCompositionLayerQuad auxiliary={0};
#endif
#ifdef GF2VR_PERF_PRODUCER_OBSERVE
    EyeGpuTicket trace_ticket={0};U64 trace_frame=0,trace_values[5]={0};
    if(producer_xr_records<64 && __atomic_load_n(&stereo_xr_output.state,__ATOMIC_ACQUIRE)==XEO_DONE &&
       stereo_xr_output.presentable && stereo_xr_output.trace_claim &&
       stereo_xr_output.frame==input.frame_id && producer_sample(stereo_xr_output.ticket.serial)) {
        trace_ticket=stereo_xr_output.ticket;trace_frame=stereo_xr_output.frame;
        trace_values[0]=stereo_xr_output.trace_offer;trace_values[1]=stereo_xr_output.trace_claim;
        trace_values[2]=stereo_xr_output.trace_ready;trace_values[3]=producer_clock();
    }
#endif
    const XrCompositionLayerBaseHeader *layers[2];XrFrameEndInfo combined=*end;
    int projected=0;
    int fresh=0;
#ifdef GF2VR_PRESENTATION_CACHE
    U64 completed_cancel_at_build=__atomic_load_n(&stereo_completed_cancel_clock,__ATOMIC_ACQUIRE);
#endif
#ifdef GF2VR_MAIN_TRANSITION_HOLD
    int held=0;U64 held_from=0;
    if(!visible || !input.valid)stereo_hold.valid=0;
#ifdef GF2VR_PRESENTATION_CACHE
    if(!visible || !input.valid)completed_presentation_cancel(&stereo_completed);
#endif
#endif
    /* This composition owns exactly the existing optional complete panel layer.
       Preserve the original list if it changes beyond that contract. */
    if(end->layerCount<=1 && (!end->layerCount || end->layers)) {
      if(stereo_two.enabled) {
          int ready;
          do {ready=stereo_two_eye_projection(panel,&input,end,&projection,views);
              if(ready==EFO_BUSY
#ifdef GF2VR_ASYNC_PRESENTATION
                 && !stereo_async_enabled
#endif
                 )Sleep(1);
          } while(ready==EFO_BUSY
#ifdef GF2VR_ASYNC_PRESENTATION
                  && !stereo_async_enabled
#endif
                  );
          fresh=ready==EFO_OK;
#ifdef GF2VR_TWO_FLIGHT_OBSERVE
          two_projection_result=ready;
#endif
      } else fresh=xr_eye_output_projection(&stereo_xr_output,panel,&input,
           stereo_eye_runtime.owner.space,end,&stereo_publication_generation,&projection,views);
      if(fresh) {
        for(unsigned i=0;i<2;++i) views[i].fov=stereo_unity_texture_fov(views[i].fov);
      }
#ifdef GF2VR_MAIN_TRANSITION_HOLD
      if(visible && stereo_hold_focused) {
        held_from=__atomic_load_n(&stereo_handoff_from,__ATOMIC_ACQUIRE);
        U64 hold_now=GetTickCount64();
        if(!fresh || transition_hold_wait(&stereo_hold,held_from,hold_now)) {
            held=transition_hold_layer(&stereo_hold,&stereo_xr_output,panel,&input,
                stereo_eye_runtime.owner.space,end,held_from,hold_now,&projection,views);
            if(held)fresh=0; /* prepared new pair drained but not displayed */
        }
#ifdef GF2VR_PRESENTATION_CACHE
        if(stereo_two.enabled && !fresh && !held && completed_presentation_layer(&stereo_completed,
            &stereo_xr_output,panel,&input,stereo_eye_runtime.owner.space,end,
            &stereo_publication_generation,hold_now,&projection,views))held=2;
#endif
      }
      if(fresh || held) {
#else
      if(fresh) {
#endif
        projected=1;
        layers[0]=(const XrCompositionLayerBaseHeader *)&projection;
#ifdef GF2VR_AUX_PANEL
        combined.layerCount=1;combined.layers=layers;
        if(end->layerCount) {
            if(!stereo_aux.ready) { /* No controller route: preserve prior presentation. */
                layers[1]=end->layers[0];combined.layerCount=2;
            } else if(end->layers[0]->type==XR_TYPE_COMPOSITION_LAYER_QUAD && aux_visible(&stereo_aux)) {
                aux_quad(&stereo_aux,(const XrCompositionLayerQuad *)end->layers[0],&auxiliary);
                layers[1]=(const XrCompositionLayerBaseHeader *)&auxiliary;combined.layerCount=2;
            }
        }
#else
        if(end->layerCount) layers[1]=end->layers[0];
        combined.layerCount=1+end->layerCount;combined.layers=layers;
#endif
      }
    }
#ifdef GF2VR_TWO_FLIGHT_OBSERVE
    /* A missing current token bypasses the ordinary projection record. Log
       only the first projected->flat edge, using already captured XR facts. */
    static unsigned two_was_projected;
    if(stereo_two.enabled && two_was_projected && !projected &&
       (!stereo_two_xr.current_token.frame || !visible || !input.valid)) {
        U64 detail=((U64)(visible!=0)<<3)|((U64)(input.valid!=0)<<2)|
            ((U64)(stereo_xr_output.enabled!=0)<<1)|
            (__atomic_load_n(&stereo_publication_generation,__ATOMIC_ACQUIRE)!=0);
        two_observe_record(0,"two_flight_missing_token",stereo_two_xr.current_token,
            stereo_two_xr.current,stereo_xr_output.ticket.serial,0,EFO_REJECTED,0,detail,1);
    }
    two_was_projected=stereo_two.enabled && projected;
    if(stereo_two.enabled && visible && input.valid && stereo_two_xr.current_token.frame) {
        TwoFlightWaitNote n=stereo_two_wait_note;
        U64 detail=((U64)(stereo_two_images.projection_reason&255u)<<56)|
            ((U64)(fresh!=0)<<55)|((U64)(held!=0)<<54)|
            ((U64)(stereo_hold.valid!=0)<<53)|((U64)(stereo_hold_focused!=0)<<52)|
            ((U64)(n.cpu_ok!=0)<<51)|((U64)(visible!=0)<<50)|
            ((U64)(input.valid!=0)<<49)|((U64)(end->layerCount>1)<<48)|
            ((U64)(unsigned)n.result<<16)|
            ((GetTickCount64()-two_wait_start)&65535u);
        two_observe_record(0,"two_flight_projection_boundary",stereo_two_xr.current_token,
            stereo_two_xr.current,stereo_xr_output.ticket.serial,n.phase,
            two_projection_result,n.flags,detail,!projected);
    }
#endif
    int isolated=0;
#ifdef GF2VR_AUX_ISOLATE
    int eligible=projected && combined.layerCount==2 &&
        combined.layers[1]==(const XrCompositionLayerBaseHeader *)&auxiliary;
    isolated=aux_isolation_poll(eligible);
    if(isolated) {combined.layers=&layers[1];combined.layerCount=1;}
    static int previous_isolated=-1;
    if(isolated!=previous_isolated) {
        event("aux_isolation",isolated,GetCurrentThreadId());previous_isolated=isolated;
    }
#endif
#ifdef GF2VR_AUX_ALPHA
    /* Controlled diagnostic: retain projection, quad index, pose and image.
       Source-alpha-only bypasses VDXR's layer>0 alpha preprocessing branch.
       This is not an opaque product presentation policy. */
    int alpha_eligible=projected && combined.layerCount==2 &&
        combined.layers[1]==(const XrCompositionLayerBaseHeader *)&auxiliary;
    int alpha_compare=aux_isolation_poll(alpha_eligible);
    if(alpha_compare) auxiliary.layerFlags=XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
    static int previous_alpha=-1;
    if(alpha_compare!=previous_alpha) {
        event("aux_alpha_compare",alpha_compare,GetCurrentThreadId());previous_alpha=alpha_compare;
    }
#endif
#ifdef GF2VR_AUX_FOV
    /* Diagnostic only: compare canonical FOV with identical upside-down pixels.
       Never change the producer/calibration or the default accepted output. */
    int fov_eligible=projected && combined.layerCount==2 &&
        combined.layers[1]==(const XrCompositionLayerBaseHeader *)&auxiliary;
    int fov_compare=aux_isolation_poll(fov_eligible);
    if(fov_compare) for(unsigned i=0;i<2;++i) views[i].fov=input.fov[i];
    static int previous_fov=-1;
    if(fov_compare!=previous_fov) {
        event("aux_fov_compare",fov_compare,GetCurrentThreadId());previous_fov=fov_compare;
    }
#endif
#ifdef GF2VR_MAIN_TRANSITION_HOLD
    /* F8/focus/invalidation may race the XR composition build. */
    if(held==1 && __atomic_load_n(&stereo_handoff_from,__ATOMIC_ACQUIRE)!=held_from) {
        combined=*end;projected=held=0;
    }
#ifdef GF2VR_PRESENTATION_CACHE
    if(held==2 && (!stereo_hold_focused || !completed_presentation_layer(&stereo_completed,
        &stereo_xr_output,panel,&input,stereo_eye_runtime.owner.space,end,
        &stereo_publication_generation,GetTickCount64(),&projection,views))) {
        combined=*end;projected=held=0;
    }
#endif
#endif
#ifdef GF2VR_CONTROLLER_INPUT
    XrCompositionLayerQuad cursor={0};const XrCompositionLayerBaseHeader *ui_layers[3];
    int cursor_submitted=0;
    unsigned ui_mode=projected?1u:0u;
    if(combined.layerCount<=2&&visible&&controller_runtime_prepare(panel,&combined,stereo_hold_focused,ui_mode,&cursor)) {
        for(unsigned i=0;i<combined.layerCount;++i)ui_layers[i]=combined.layers[i];
        ui_layers[combined.layerCount]=(const XrCompositionLayerBaseHeader *)&cursor;
        combined.layers=ui_layers;++combined.layerCount;cursor_submitted=1;
    }
#endif
#ifdef GF2VR_PERF_END_SPLIT
    U64 runtime_end_start=GetTickCount64();
#endif
#ifdef GF2VR_ASYNC_PRESENTATION
    if(stereo_two.enabled && stereo_async_enabled && !fresh && visible && stereo_hold_focused &&
       !stereo_two_images.token.frame && stereo_two_xr.current_token.frame && stereo_xr_output.enabled &&
       __atomic_load_n(&stereo_publication_generation,__ATOMIC_ACQUIRE)==stereo_two_xr.current_token.generation &&
       stereo_eye_runtime.owner.epoch==stereo_two_xr.current_token.epoch &&
       stereo_eye_runtime.owner.head_reference_epoch==stereo_two_xr.current_token.epoch &&
       !stereo_eye_runtime.owner.changing && !stereo_eye_runtime.owner.retiring) {
        int prepared;
        do {prepared=xfi_prepare_offer(&stereo_two_images,&stereo_xr_output,panel,
            &stereo_two.owner,stereo_two_xr.current_token);if(prepared==EFO_BUSY)Sleep(1);} while(prepared==EFO_BUSY);
        if(stereo_two_images.fault)return XR_ERROR_RUNTIME_FAILURE;
        /* Rejection is ordinary fail-open: do not retain an unprepared offer. */
    }
    if(stereo_two.enabled && stereo_async_enabled && !fresh &&
       xfi_same(stereo_two_images.token,stereo_two_xr.current_token)) {
        int allowed;
        do {allowed=stereo_two_images.pending_offer?
            eye_flight_can_retain_offer(&stereo_two.owner,stereo_two_xr.current_token,end->displayTime):
            eye_flight_can_detach(&stereo_two.owner,stereo_two_xr.current_token,end->displayTime);
            if(allowed==EFO_BUSY)Sleep(1);} while(allowed==EFO_BUSY);
        if(allowed!=EFO_OK) {
            /* Exceptional source/reference cancellation cannot detach eligible
               production. Balance its original frame only after genuine release;
               this is not the ordinary independently clocked completion path. */
            int drained;
            do {drained=stereo_async_drain(panel);
                if(drained!=EFO_OK && !stereo_two_images.fault)Sleep(1);
            } while(drained!=EFO_OK && !stereo_two_images.fault);
            if(drained!=EFO_OK)return XR_ERROR_RUNTIME_FAILURE;
        }
    }
#endif
    if(stereo_two.enabled && stereo_two_images.token.frame && !fresh
#ifdef GF2VR_ASYNC_PRESENTATION
       && !stereo_async_enabled
#endif
       ) {
        /* A timeout may end private render with no XR lease. Once images are
           acquired/claimed, however, only real wait/copy/release clears them. */
        int released;
        do {released=xfi_release(&stereo_two_images,&stereo_xr_output,panel,&stereo_two.owner);
            if(released!=EFO_OK)Sleep(1);} while(released!=EFO_OK);
    }
#ifdef GF2VR_PRESENTATION_CACHE
    /* Release callbacks can reenter invalidation. Recheck after that boundary;
       cached presentation never grants permission to end a live GPU lease. */
    if(held==2 && (!stereo_hold_focused || !completed_presentation_layer(&stereo_completed,
        &stereo_xr_output,panel,&input,stereo_eye_runtime.owner.space,end,
        &stereo_publication_generation,GetTickCount64(),&projection,views))) {
        combined=*end;projected=held=0;
#ifdef GF2VR_CONTROLLER_INPUT
        cursor_submitted=0;
#endif
    }
#endif
    int current_fresh=fresh && !isolated;
#ifdef GF2VR_ASYNC_PRESENTATION
    if(stereo_async_enabled && async_late)current_fresh=0; /* new display != old production */
#endif
    XrResult result=stereo_two.enabled?stereo_two_frame_end(&combined,current_fresh):
        panel->api.EndFrame(panel->session,&combined);
#ifdef GF2VR_PERF_PRODUCER_OBSERVE
    if(trace_ticket.serial) {
        trace_values[4]=producer_clock();++producer_xr_records;
        const char *keys[]={"offer","claim","ready","wait_exit","end_return"};
        producer_record("perf_eye_xr_stages",trace_ticket,trace_frame,keys,trace_values,5);
    }
#endif
#ifdef GF2VR_PERF_END_SPLIT
    stereo_pacing_runtime_end_ms+=GetTickCount64()-runtime_end_start;
#endif
#ifdef GF2VR_PERF_OBSERVE
    if(result==XR_SUCCESS&&fresh&&projected&&!held&&!isolated)++stereo_pacing_fresh;
#ifdef GF2VR_TOUCH_PRIVATE
    if(result==XR_SUCCESS&&fresh&&projected&&!held&&!isolated)touch_p2_fresh();
#endif
#line 1942
#endif
#ifdef GF2VR_CONTROLLER_INPUT
    controller_runtime_end(result,cursor_submitted);
    /* The successfully submitted original panel hit wins Trigger arbitration,
       even if OS injection failed; held UI input must not become navigation. */
    if(result==XR_SUCCESS&&cursor_submitted)
        world_drag_arbitrate(&stereo_aux.drag.frame,
#ifdef GF2VR_CONTROLLER_HAND_ROLES
            stereo_panel_role_hand(),
#else
            0,
#endif
            1,stereo_aux.pointer.hand?0u:1u,1,1);
#endif
#ifdef GF2VR_WORLD_DRAG
    /* Camera-handoff hold is not navigable. Opt-in independently clocked,
       same-generation completed output remains immersive between productions;
       current tracked input is sampled once per real successful display End. */
    U64 drag_generation=__atomic_load_n(&stereo_publication_generation,__ATOMIC_ACQUIRE);
    int drag_world=visible&&fresh&&projected&&!held&&!isolated&&
        result==XR_SUCCESS&&stereo_hold_focused&&drag_generation==stereo_xr_output.ticket.generation;
#ifdef GF2VR_ASYNC_PRESENTATION
    U64 drag_now=GetTickCount64();
    if(stereo_two.enabled && held==2 && visible && projected && !isolated && result==XR_SUCCESS &&
       stereo_hold_focused && input.valid && drag_generation && stereo_completed.valid &&
       stereo_completed.generation==drag_generation && stereo_completed.epoch==input.reference_epoch &&
       stereo_completed.session==panel->session && stereo_completed.space==stereo_eye_runtime.owner.space &&
       stereo_eye_runtime.owner.epoch==input.reference_epoch &&
       stereo_eye_runtime.owner.head_reference_epoch==input.reference_epoch &&
       !stereo_eye_runtime.owner.changing && !stereo_eye_runtime.owner.retiring &&
       drag_now>=stereo_completed.tick && drag_now-stereo_completed.tick<250 &&
       stereo_completed.cancel_clock==__atomic_load_n(&stereo_completed_cancel_clock,__ATOMIC_ACQUIRE))drag_world=1;
#endif
    stereo_world_drag_end(stereo_aux.drag.frame,drag_world,input.reference_epoch,drag_generation);
#endif
#ifdef GF2VR_MAIN_TRANSITION_HOLD
    if(result!=XR_SUCCESS)stereo_hold.valid=0;
    else if(fresh && !isolated && stereo_xr_output.hold_enabled && stereo_hold_focused)
        transition_hold_capture(&stereo_hold,&stereo_xr_output,&input,&projection,GetTickCount64());
#ifdef GF2VR_PRESENTATION_CACHE
    if(result!=XR_SUCCESS)completed_presentation_cancel(&stereo_completed);
    else if(stereo_two.enabled && fresh && !isolated && stereo_hold_focused &&
        completed_cancel_at_build==__atomic_load_n(&stereo_completed_cancel_clock,__ATOMIC_ACQUIRE))
    {
#ifdef GF2VR_ASYNC_PRESENTATION
        if(stereo_async_enabled && async_late)
            (void)completed_presentation_capture_late(&stereo_completed,&stereo_xr_output,&input,&projection,
                __atomic_load_n(&stereo_publication_generation,__ATOMIC_ACQUIRE),async_prepared_tick,
                GetTickCount64(),completed_cancel_at_build,result);
        else
#endif
            (void)completed_presentation_capture(&stereo_completed,&stereo_xr_output,&input,&projection,
                __atomic_load_n(&stereo_publication_generation,__ATOMIC_ACQUIRE),GetTickCount64());
    }
    static unsigned cache_hold_records;
    if(held==2 && (++cache_hold_records<=8 || cache_hold_records%100==0))
        event("completed_presentation_hold",GetTickCount64()-stereo_completed.tick,GetCurrentThreadId());
#endif
    static unsigned prior_held;
    if(held==1 || prior_held)event("main_transition_hold",held==1?GetTickCount64()-stereo_hold.tick:0,GetCurrentThreadId());
    prior_held=(unsigned)(held==1);
#endif
#ifdef GF2VR_AUX_OBSERVE
    aux_observe_end(panel,&combined,projected && !isolated,result);
#endif
#ifdef GF2VR_AUX_PANEL
    /* Diagnostic isolation suppresses submission only; keep the normal
       producer opportunity next frame so releasing F10 can restore stereo. */
    stereo_aux.last_projection=projected && XR_SUCCEEDED(result);
    static unsigned previous_mode=~0u;
    unsigned mode=isolated?3u:(projected?(combined.layerCount>=2&&combined.layers[1]->type==XR_TYPE_COMPOSITION_LAYER_QUAD?2u:1u):0u);
#ifdef GF2VR_TOUCH_PRIVATE
    touch_p2_mode(mode);
#endif
    if(mode!=previous_mode) {event("composite_panel_mode",mode,GetCurrentThreadId());previous_mode=mode;}
#line 2014
#endif
#ifdef GF2VR_MAIN_STEREO
    if(fresh && !isolated) {
        static U64 count;
        if(++count<=8 || count%100==0 || result!=XR_SUCCESS)
            event(result==XR_SUCCESS?"stereo_projection_end_frame":"stereo_projection_end_frame_failed",
                  result==XR_SUCCESS?input.frame_id:(U64)result,GetCurrentThreadId());
    }
#endif
    (void)projected;
    (void)isolated;
    if(stereo_two.enabled && XR_SUCCEEDED(result) && stereo_two_images.token.frame) {
#ifdef GF2VR_ASYNC_PRESENTATION
        if(stereo_async_enabled) {
            int synced=xfi_sync(&stereo_two_images,&stereo_xr_output,&stereo_two.owner);
            if(synced!=EFO_OK)return XR_ERROR_RUNTIME_FAILURE;
            if(stereo_two_images.detached) {
                if(async_late && fresh && !held && !isolated) {
                    async_throughput_record(8,stereo_two_images.token,stereo_xr_output.ticket.serial,0,
                        result,stereo_two_xr.current);
                    int r;do {r=xfi_detached_finalize(&stereo_two_images,&stereo_xr_output,&stereo_two.owner,
                        result==XR_SUCCESS,result==XR_SUCCESS);if(r==EFO_BUSY)Sleep(1);} while(r==EFO_BUSY);
                    if(r!=EFO_OK)return XR_ERROR_RUNTIME_FAILURE;
                }
            } else if(xfi_retire(&stereo_two_images,&stereo_xr_output,1)!=EFO_OK)return XR_ERROR_RUNTIME_FAILURE;
        } else
#endif
        if(xfi_retire(&stereo_two_images,&stereo_xr_output,1)!=EFO_OK)return XR_ERROR_RUNTIME_FAILURE;
    }
    return result;
}
#ifdef GF2VR_BANK_PRESENTATION
#include "bank_presentation_runtime.h"
#endif
#ifdef GF2VR_AUX_PANEL
static int stereo_panel_wanted(void *opaque,const XrFrameState *frame) {
    PanelXr *panel=opaque;
    U64 generation=__atomic_load_n(&stereo_publication_generation,__ATOMIC_ACQUIRE);
#ifdef GF2VR_CONTROLLER_INPUT
    stereo_controller.sample_tick=GetTickCount64();
#endif
    aux_frame(&stereo_aux,panel->space,frame->predictedDisplayTime,generation);
#ifdef GF2VR_CONTROLLER_GAME_MODE
    controller_game_runtime_prepare(frame->shouldRender && stereo_hold_focused,generation,
        stereo_eye_runtime.owner.head_reference_epoch);
#endif
#ifdef GF2VR_AUX_OBSERVE
    if(aux_visible(&stereo_aux) && !__atomic_load_n(&panel_observe_request,__ATOMIC_ACQUIRE))
        __atomic_store_n(&panel_observe_request,panel->frames+1,__ATOMIC_RELEASE);
#endif
    return aux_copy_needed(&stereo_aux,generation);
}
#endif
static void stereo_output_xr_open(void *opaque,void *module) {
    BootstrapState *state=opaque;
#ifdef GF2VR_WORLD_DRAG
    stereo_world_drag_open();
#endif
#ifdef GF2VR_CONTROLLER_INPUT
    controller_runtime_open();
#ifdef GF2VR_CONTROLLER_GAME_MODE
    controller_game_runtime_open();
#endif
#endif
    PFN_xrGetInstanceProcAddr get=0;
#ifdef BOUNDARY_TEST
    get=stereo_eye_test_get;
#endif
    if(!get && module) get=(PFN_xrGetInstanceProcAddr)GetProcAddress(module,"xrGetInstanceProcAddr");
#ifdef GF2VR_AUX_PANEL
    event(aux_open(&stereo_aux,get,state->instance,stereo_eye_runtime.owner.session)?
        "aux_panel_ready":"aux_panel_unavailable",0,GetCurrentThreadId());
#ifdef GF2VR_THUMBSTICK_NAVIGATION
    event("navigation_action_ready",(U64)stereo_aux.navigation.ready|((U64)stereo_aux.navigation.bound_profiles<<8),GetCurrentThreadId());
#endif
#endif
#ifdef GF2VR_RENDER_SCALE
    double scale=0;int status=render_scale_load(GF2VR_RENDER_QUALITY_CONFIG,&scale);
    xr_eye_size_clear(&stereo_xr_size);
    event("vr_render_scale_config_state",(U64)(int64_t)status,GetCurrentThreadId());
    if(status<=0||!xr_eye_size_open_scaled(&stereo_xr_size,get,state->instance,state->system,scale))
        event("eye_size_unavailable",0,GetCurrentThreadId());
    else{union{double value;U64 bits;} requested={.value=scale};unsigned width=0,height=0;
        (void)xr_eye_size_read(&stereo_xr_size,&width,&height);
        event("vr_render_scale_bits",requested.bits,GetCurrentThreadId());
        event("vr_render_extent",((U64)height<<32)|width,GetCurrentThreadId());}
#else
    if(!xr_eye_size_open(&stereo_xr_size,get,state->instance,state->system))
        event("eye_size_unavailable",0,GetCurrentThreadId());
#endif
#line 2073
}
static void stereo_output_xr_frame(void *panel,U64 frame) {
#ifdef GF2VR_NUMERIC_RESIDENCY_PERSIST
    numeric_residency_persist_tick(__atomic_load_n(&stereo_publication_generation,__ATOMIC_ACQUIRE));
#endif
#line 2075
#ifdef GF2VR_BANK_PRESENTATION
    if(stereo_bank_enabled && stereo_two.enabled) {
        if(stereo_two_xr.current)frame=stereo_two_xr.current;
        U64 epoch=__atomic_load_n(&stereo_eye_runtime.mailbox.epoch,__ATOMIC_ACQUIRE);
        if(__atomic_load_n(&stereo_eye_runtime.mailbox.frame,__ATOMIC_ACQUIRE)!=frame)epoch=0;
        xbc_drive(&stereo_xr_config,&stereo_banks,&stereo_bank_cache,panel,&stereo_two.owner,epoch);
        if(stereo_banks.storage.ready)(void)xbc_generation(&stereo_banks,panel,&stereo_two.owner);
        (void)stereo_bank_drain(panel,0);return;
    }
#endif
    if(stereo_two.enabled) {
        /* New route owns frame/leases; never manufacture a legacy eye grant. */
        if(stereo_two_xr.current)frame=stereo_two_xr.current;
        U64 epoch=__atomic_load_n(&stereo_eye_runtime.mailbox.epoch,__ATOMIC_ACQUIRE);
        if(__atomic_load_n(&stereo_eye_runtime.mailbox.frame,__ATOMIC_ACQUIRE)!=frame)epoch=0;
        if(!stereo_two_images.token.frame)
            xr_eye_config_drive(&stereo_xr_config,&stereo_xr_output,panel,epoch);
#ifdef GF2VR_MAIN_TRANSITION_HOLD
        stereo_xr_output.hold_enabled=1;
        if(!epoch || (stereo_hold.valid && stereo_hold.epoch!=epoch))stereo_hold.valid=0;
        /* A deferred flight may have no ready source this frame. Rotate a
           completed protected bank BEFORE that readiness decision, so hold
           presentation can use the inactive bank without acquiring/writing
           any new XR image. Never rotate underneath an attached flight. */
        if(!stereo_two_images.token.frame && stereo_hold.valid &&
           stereo_xr_output.enabled && stereo_xr_output.session==((PanelXr *)panel)->session &&
           stereo_xr_output.bank_protected)
            (void)xr_eye_output_next_bank(&stereo_xr_output,panel);
#endif
        return;
    }
    U64 epoch=__atomic_load_n(&stereo_eye_runtime.mailbox.epoch,__ATOMIC_ACQUIRE);
    if(__atomic_load_n(&stereo_eye_runtime.mailbox.frame,__ATOMIC_ACQUIRE)!=frame) epoch=0;
    xr_eye_config_drive(&stereo_xr_config,&stereo_xr_output,panel,epoch);
#ifdef GF2VR_MAIN_TRANSITION_HOLD
    stereo_xr_output.hold_enabled=1;
    if(!epoch || (stereo_hold.valid && stereo_hold.epoch!=epoch))stereo_hold.valid=0;
#endif
    xr_eye_output_frame(&stereo_xr_output,panel,frame,epoch);
}
static void stereo_output_xr_invalidate(void *panel) {
#ifdef GF2VR_MAIN_TRANSITION_HOLD
    /* Ordinary panel-loop prediction invalidation is not source/reference
       cancellation. The new route keeps only the completed presentation bank;
       focus/off/reference/End failure/close still invalidate it explicitly. */
    if(!stereo_two.enabled)stereo_hold.valid=0;
#endif
    if(!stereo_two.enabled)(void)xr_eye_output_reclaim(&stereo_xr_output,panel);
}
static int stereo_output_xr_close(void *panel) {
    while(!stereo_two_xr_close(panel))Sleep(1);
#ifdef GF2VR_CONTROLLER_INPUT
    while(!controller_runtime_close())Sleep(10);
#ifdef GF2VR_CONTROLLER_GAME_MODE
    while(!controller_game_runtime_close())Sleep(10);
#endif
#endif
#ifdef GF2VR_WORLD_DRAG
    stereo_world_drag_reset();
#endif
#ifdef GF2VR_THUMBSTICK_NAVIGATION
    stereo_navigation_cancel();
#endif
#ifdef GF2VR_MAIN_TRANSITION_HOLD
    stereo_hold.valid=0;
    stereo_hold_focused=0;
#ifdef GF2VR_PRESENTATION_CACHE
    completed_presentation_cancel(&stereo_completed);
#endif
#endif
#ifdef GF2VR_AUX_PANEL
    aux_reset_visibility(&stereo_aux);
#endif
    xr_eye_size_clear(&stereo_xr_size);
    /* Returning to bootstrap would destroy the parent instance. Keep this XR
       worker/session alive while destinations may still be used; opaque fault
       quarantines here, with no game-thread join or timeout destruction. */
#ifdef GF2VR_BANK_PRESENTATION
    if(stereo_bank_enabled) {
        while(!xbc_close(&stereo_xr_config,&stereo_banks,&stereo_bank_cache,panel,&stereo_two.owner))Sleep(10);
    } else
#endif
        while(!xr_eye_config_close(&stereo_xr_config,&stereo_xr_output,panel)) Sleep(10);
    return 1;
}
static int stereo_reserved_matches(const SourceRenderFrame *f) {
    return f && f->serial && stereo_output.reserved.serial==f->serial &&
        stereo_output.reserved.generation==f->check.ticket.source_generation &&
        stereo_output.reserved.epoch==f->epoch && stereo_output.destinations[0] && stereo_output.destinations[1];
}
/* Only before event issuance (or for a never-injected root). Cancels destination
   use, never proves that engine source-eye use has drained. */
static int stereo_reservation_cancel(void) {
    if(!stereo_output.reserved.serial) return 1;
    if(__atomic_load_n(&stereo_interval.state,__ATOMIC_ACQUIRE)!=ERI_IDLE && stereo_interval.refs[4] &&
       (__atomic_load_n(&stereo_interval.end_issued,__ATOMIC_ACQUIRE) ||
        (__atomic_load_n(&stereo_interval.begin_issued,__ATOMIC_ACQUIRE) &&
         !__atomic_load_n(&stereo_interval.abandoned,__ATOMIC_ACQUIRE)))) return 0;
    if(__atomic_load_n(&stereo_output_event.state,__ATOMIC_ACQUIRE)!=EEC_IDLE ||
       !xr_eye_output_complete(&stereo_xr_output,stereo_output.reserved)) return 0;
    stereo_output.reserved=(EyeGpuTicket){0};
    stereo_output.destinations[0]=stereo_output.destinations[1]=0;return 1;
}
static int stereo_frame_output_current(void *opaque) {
    SourceRenderFrame *f=opaque;
    DrawQueueIdentity now={0};
    return stereo_output.enabled && stereo_reserved_matches(f) && source_render_frame_current(f) &&
        __atomic_load_n(&panel_device_ready,__ATOMIC_ACQUIRE) &&
        !__atomic_load_n(&panel_device_changed,__ATOMIC_ACQUIRE) &&
        xr_eye_config_ready(&stereo_xr_config,&stereo_frame_description,f->epoch) &&
        stereo_draw_order.serial==f->serial && stereo_draw_order.ready &&
        draw_runtime_identity(&now) && draw_queue_same(stereo_draw_order.entry,now);
}
static int stereo_interval_frame_current(void *opaque) {
    SourceRenderFrame *f=opaque;
    return stereo_frame_output_current(f) &&
        __atomic_load_n(&stereo_interval.state,__ATOMIC_ACQUIRE)==ERI_QUEUED &&
        stereo_interval.ticket.serial==f->serial &&
        stereo_interval.ticket.generation==f->check.ticket.source_generation &&
        stereo_interval.ticket.epoch==f->epoch;
}
static int stereo_interval_entry(void) {
    unsigned char code[5]={0};
    return unity && panel_render_tid && draw_runtime_read(0,(U64)unity+0x94d840,code,5) &&
        code[0]==0xe9 && code[1]==0x4b && code[2]==0xcc && code[3]==0xd2 && code[4]==0xff;
}
/* Run after preparation but BEFORE forwarding eyes into the original pipeline.
   Waiting for configuration leaves the checked scope original-only; ordinary
   root finish then releases the never-injected borrow without a GPU fence. */
static int stereo_describe(void *device,void *left,void *right,EyeGpuDescription *out) {
#ifdef GF2VR_PREPARE_OBSERVE
    static unsigned reported;
    EyeGpuObservation o={0};
    int ok=eye_gpu_describe_observed(device,left,right,out,&o);
    if(!ok && prepare_active && !reported) {
        reported=1;DWORD saved=GetLastError(),thread=GetCurrentThreadId();
        event("eye_descriptor_root",stereo_pipeline.render.serial,thread);
        event("eye_descriptor_reject",((U64)o.stage<<32)|((U64)o.eye<<16)|o.reject_mask,thread);
        event("eye_descriptor_device",((U64)o.device_flags<<32)|o.result,thread);
        for(unsigned i=0;i<2;++i) if(o.valid_mask&(1u<<i)) {
            EyeGpuObservedTexture *t=&o.textures[i];
            event("eye_descriptor_index",i,thread);
            event("eye_descriptor_extent",((U64)t->width<<32)|t->height,thread);
            event("eye_descriptor_shape",((U64)t->mips<<32)|t->array,thread);
            event("eye_descriptor_format",((U64)t->format<<32)|t->bind,thread);
            event("eye_descriptor_samples",((U64)t->samples<<32)|t->quality,thread);
            event("eye_descriptor_access",((U64)t->usage<<32)|t->cpu,thread);
            event("eye_descriptor_misc",t->misc,thread);
        }
        SetLastError(saved);
    }
    return ok;
#else
    return eye_gpu_describe(device,left,right,out);
#endif
}
static int stereo_preflight(void *device,void *left,void *right,void *dl,void *dr) {
#ifdef GF2VR_PREPARE_OBSERVE
    static unsigned reported;
    EyeGpuObservation o={0};
    int ok=eye_gpu_preflight_observed(device,left,right,dl,dr,&o);
    if(!ok && prepare_active && !reported) {
        reported=1;DWORD saved=GetLastError(),thread=GetCurrentThreadId();
        event("eye_preflight_root",stereo_pipeline.render.serial,thread);
        event("eye_preflight_reject",((U64)o.stage<<32)|((U64)o.eye<<16)|o.reject_mask,thread);
        event("eye_preflight_device",((U64)o.device_flags<<32)|o.result,thread);
        for(unsigned i=0;i<4;++i) if(o.valid_mask&(1u<<i)) {
            EyeGpuObservedTexture *t=&o.textures[i];
            event("eye_preflight_index",i,thread);
            event("eye_preflight_extent",((U64)t->width<<32)|t->height,thread);
            event("eye_preflight_shape",((U64)t->mips<<32)|t->array,thread);
            event("eye_preflight_format",((U64)t->format<<32)|t->bind,thread);
            event("eye_preflight_samples",((U64)t->samples<<32)|t->quality,thread);
            event("eye_preflight_access",((U64)t->usage<<32)|t->cpu,thread);
            event("eye_preflight_misc",t->misc,thread);
        }
        SetLastError(saved);
    }
    return ok;
#else
    return eye_gpu_preflight(device,left,right,dl,dr);
#endif
}
#ifdef GF2VR_COPY_GPU_OBSERVE
__declspec(dllimport) int __stdcall QueryPerformanceFrequency(long long *);
#ifdef BOUNDARY_TEST
static unsigned copy_gpu_test_published,copy_gpu_test_valid,copy_gpu_test_invalid;
#endif
static char *copy_gpu_decimal(char *p,U64 value) {
    char digits[20];unsigned n=0;
    do {digits[n++]=(char)('0'+value%10);value/=10;}while(value);
    while(n)*p++=digits[--n];return p;
}
static void stereo_copy_gpu_observe_tick(void) {
    static unsigned cursor;DWORD saved=GetLastError();
    for(unsigned i=0;i<4;++i) {
        CopyGpuObservation r;if(!copy_gpu_observe_take(&cursor,&r))break;
        char line[1500],*p=source_prefix(line,"copy_gpu_boundary");
#define CGO_WRITE(name,value) do {p=literal(p,",\"" name "\":");p=copy_gpu_decimal(p,(U64)(value));}while(0)
        CGO_WRITE("serial",r.ticket.serial);CGO_WRITE("generation",r.ticket.generation);CGO_WRITE("epoch",r.ticket.epoch);
        CGO_WRITE("gpu_begin",r.gpu_begin);CGO_WRITE("gpu_end",r.gpu_end);CGO_WRITE("gpu_frequency",r.gpu_frequency);
        CGO_WRITE("cpu_begin",r.cpu_begin);CGO_WRITE("cpu_end",r.cpu_end);CGO_WRITE("cpu_read",r.cpu_read);
        long long frequency=0;QueryPerformanceFrequency(&frequency);CGO_WRITE("cpu_frequency",frequency>0?frequency:0);
        CGO_WRITE("render_thread",r.thread);CGO_WRITE("device",r.device);CGO_WRITE("context",r.context);
        CGO_WRITE("width",r.width);CGO_WRITE("height",r.height);CGO_WRITE("format",r.format);
        CGO_WRITE("copy_mask",r.copy_mask);CGO_WRITE("upright",r.upright);
        CGO_WRITE("status",r.status);CGO_WRITE("result",r.result);CGO_WRITE("pending_reads",r.pending_reads);
#undef CGO_WRITE
        p=literal(p,"}\n");source_log(line,p);
#ifdef BOUNDARY_TEST
        ++copy_gpu_test_published;if(r.status==CGO_VALID)++copy_gpu_test_valid;else ++copy_gpu_test_invalid;
#endif
    }
    SetLastError(saved);
}
#endif
#ifdef GF2VR_COPY_PRESENT_OBSERVE
__declspec(dllimport) int __stdcall QueryPerformanceFrequency(long long *);
#ifdef BOUNDARY_TEST
static unsigned copy_present_test_published;
#endif
static char *copy_present_decimal(char *p,U64 value) {
    char digits[20];unsigned n=0;
    do {digits[n++]=(char)('0'+value%10);value/=10;}while(value);
    while(n)*p++=digits[--n];return p;
}
static void stereo_copy_present_observe_tick(void) {
    static unsigned cursor;DWORD saved=GetLastError();
    for(unsigned i=0;i<8;++i) {
        CopyPresentObservation r;if(!copy_present_observe_take(&cursor,&r))break;
        char line[1000],*p=source_prefix(line,"copy_present_boundary");
#define CPO_WRITE(name,value) do {p=literal(p,",\"" name "\":");p=copy_present_decimal(p,(U64)(value));}while(0)
        CPO_WRITE("serial",r.ticket.serial);CPO_WRITE("generation",r.ticket.generation);CPO_WRITE("epoch",r.ticket.epoch);
        CPO_WRITE("qpc",r.qpc);CPO_WRITE("present",r.present);CPO_WRITE("device",r.device);CPO_WRITE("context",r.context);
        CPO_WRITE("value",r.value);CPO_WRITE("observer_thread",r.thread);CPO_WRITE("edge",r.edge);CPO_WRITE("phase",r.phase);
        long long frequency=0;QueryPerformanceFrequency(&frequency);CPO_WRITE("cpu_frequency",frequency>0?frequency:0);
#undef CPO_WRITE
        p=literal(p,"}\n");source_log(line,p);
#ifdef BOUNDARY_TEST
        ++copy_present_test_published;
#endif
    }
    SetLastError(saved);
}
#endif
#include "two_flight_root_observe.h"
#ifdef GF2VR_BANK_EYE_WRITE_OBSERVE
#include "bank_eye_write_observe.h"
#endif
#ifdef GF2VR_BANK_GPU_OBSERVE
#include "bank_gpu_observe.h"
#endif
#ifdef GF2VR_BANK_FENCE_OBSERVE
#include "bank_fence_observe.h"
__declspec(dllimport) int __stdcall QueryPerformanceFrequency(long long *);
#if defined(GF2VR_BANK_DISPATCH_OBSERVE) || defined(GF2VR_BANK_ADMISSION_OBSERVE)
/* Bootstrap decimal() is uint32. Preserve full ticket/QPC/value widths here. */
static char *bank_dispatch_decimal(char *p,U64 value) {
    char digits[20];unsigned count=0;
    do {digits[count++]=(char)('0'+value%10);value/=10;}while(value);
    while(count)*p++=digits[--count];return p;
}
#define BANK_NOTE_DECIMAL bank_dispatch_decimal
#else
#define BANK_NOTE_DECIMAL decimal
#endif
static void stereo_bank_fence_observe_tick(void) {
    static unsigned cursor;
    DWORD saved=GetLastError();
    for(unsigned i=0;i<8;++i) {
        BankFenceObservation r;if(!bank_fence_observe_take(&cursor,&r))break;
        char line[1100],*p=source_prefix(line,"bank_fence_boundary");
        p=literal(p,",\"serial\":");p=BANK_NOTE_DECIMAL(p,r.serial);
        p=literal(p,",\"generation\":");p=BANK_NOTE_DECIMAL(p,r.generation);
        p=literal(p,",\"epoch\":");p=BANK_NOTE_DECIMAL(p,r.epoch);
        p=literal(p,",\"qpc\":");p=BANK_NOTE_DECIMAL(p,r.qpc);
        long long frequency=0;QueryPerformanceFrequency(&frequency);
        p=literal(p,",\"frequency\":");p=BANK_NOTE_DECIMAL(p,frequency>0?(U64)frequency:0);
        p=literal(p,",\"observer_thread\":");p=decimal(p,r.thread);
        p=literal(p,",\"phase\":");p=decimal(p,r.phase);
        p=literal(p,",\"drain_only\":");p=decimal(p,r.drain_only);
        p=literal(p,",\"tried_mask\":");p=decimal(p,r.tried);
        p=literal(p,",\"value\":");p=BANK_NOTE_DECIMAL(p,r.value);
        const char *names[]={",\"device_hr\":",",\"context_hr\":",",\"create_hr\":",",\"signal_hr\":"};
        for(unsigned j=0;j<4;++j){p=literal(p,names[j]);p=decimal(p,r.hr[j]);}
        p=literal(p,"}\n");source_log(line,p);
    }
    SetLastError(saved);
}
#ifdef GF2VR_BANK_GPU_OBSERVE
static void stereo_bank_gpu_observe_tick(void) {
    static unsigned cursor;
    DWORD saved=GetLastError();
    for(unsigned i=0;i<4;++i) {
        BankGpuObservation r;if(!bank_gpu_observe_take(&cursor,&r))break;
        char line[1500],*p=source_prefix(line,"bank_gpu_boundary");
#define BGO_WRITE(name,value) do {p=literal(p,",\"" name "\":");p=bank_dispatch_decimal(p,(U64)(value));}while(0)
        BGO_WRITE("serial",r.ticket.serial);BGO_WRITE("generation",r.ticket.generation);BGO_WRITE("epoch",r.ticket.epoch);
        BGO_WRITE("gpu_begin",r.gpu_begin);BGO_WRITE("gpu_end",r.gpu_end);BGO_WRITE("gpu_frequency",r.gpu_frequency);
        BGO_WRITE("cpu_begin",r.cpu_begin);BGO_WRITE("cpu_end",r.cpu_end);BGO_WRITE("cpu_read",r.cpu_read);
        long long frequency=0;QueryPerformanceFrequency(&frequency);BGO_WRITE("cpu_frequency",frequency>0?frequency:0);
        BGO_WRITE("render_thread",r.thread);BGO_WRITE("device",r.device);BGO_WRITE("context",r.context);
        BGO_WRITE("width",r.width);BGO_WRITE("height",r.height);BGO_WRITE("format",r.format);
        BGO_WRITE("draw_mask",r.draw_mask);BGO_WRITE("clear_mask",r.clear_mask);
        BGO_WRITE("status",r.status);BGO_WRITE("result",r.result);BGO_WRITE("pending_reads",r.pending_reads);
#undef BGO_WRITE
        p=literal(p,"}\n");source_log(line,p);
    }
    SetLastError(saved);
}
#endif
#endif
#ifdef GF2VR_BANK_EYE_WRITE_OBSERVE
static void stereo_bank_eye_write_observe_tick(void) {
    static unsigned cursor;DWORD saved=GetLastError();
    for(unsigned i=0;i<4;++i) {
        BankEyeWriteObservation r;if(!bank_eye_write_observe_take(&cursor,&r))break;
        char line[2200],*p=source_prefix(line,"bank_eye_write_boundary");
#define BEW_WRITE(name,value) do {p=literal(p,",\"" name "\":");p=bank_dispatch_decimal(p,(U64)(value));}while(0)
        BEW_WRITE("serial",r.ticket.serial);BEW_WRITE("generation",r.ticket.generation);BEW_WRITE("epoch",r.ticket.epoch);
        BEW_WRITE("gpu_begin",r.gpu_begin);BEW_WRITE("gpu_end",r.gpu_end);BEW_WRITE("gpu_frequency",r.gpu_frequency);
        BEW_WRITE("gpu_left",r.gpu_eye[0]);BEW_WRITE("gpu_right",r.gpu_eye[1]);
        BEW_WRITE("cpu_begin",r.cpu_begin);BEW_WRITE("cpu_end",r.cpu_end);BEW_WRITE("cpu_read",r.cpu_read);
        BEW_WRITE("cpu_left",r.cpu_eye[0]);BEW_WRITE("cpu_right",r.cpu_eye[1]);
        long long frequency=0;QueryPerformanceFrequency(&frequency);BEW_WRITE("cpu_frequency",frequency>0?frequency:0);
        BEW_WRITE("render_thread",r.thread);BEW_WRITE("device",r.device);BEW_WRITE("context",r.context);
        BEW_WRITE("width",r.width);BEW_WRITE("height",r.height);BEW_WRITE("format",r.format);
        BEW_WRITE("draw_mask",r.draw_mask);BEW_WRITE("clear_mask",r.clear_mask);
        BEW_WRITE("status",r.status);BEW_WRITE("result",r.result);BEW_WRITE("pending_reads",r.pending_reads);
        BEW_WRITE("seen_mask",r.seen_mask);BEW_WRITE("reclear_mask",r.reclear_mask);BEW_WRITE("first_mask",r.first_mask);
        BEW_WRITE("left_draws",r.draws[0]);BEW_WRITE("right_draws",r.draws[1]);BEW_WRITE("other_draws",r.draws[2]);
#undef BEW_WRITE
        p=literal(p,"}\n");source_log(line,p);
    }
    SetLastError(saved);
}
#endif
#ifdef GF2VR_ROOT_INTERVAL_TIMING_OBSERVE
static RootIntervalTag rit_main_tag(void) {
    /* Already selected main-root scalar ownership only. Never current(). */
    if(!stereo_live || stereo_live->pair_count!=EFO_CAPACITY ||
        stereo_two.root_slot>=EFO_CAPACITY || !stereo_two.root_serial)return (RootIntervalTag){0};
    EyeFlightSnapshot *f=&stereo_two.slots[stereo_two.root_slot].admitted;
    if(f->ticket.serial!=stereo_two.root_serial)return (RootIntervalTag){0};
    return (RootIntervalTag){(U64)&stereo_two.owner,f->token.frame,
        f->token.generation,f->token.epoch,f->ticket.serial};
}
#endif
#ifdef GF2VR_SOURCE_RETIRE_OBSERVE
#include "source_retire_observe.h"
#endif
#include "two_flight_pipeline.h"
#ifdef GF2VR_ACTIVITY_CANVAS_SCOPE
#ifdef GF2VR_ACTIVITY_UI_REGISTRATION_OBSERVE
#include "activity_ui_registration_observe.h"
#endif
#ifdef GF2VR_ACTIVITY_UI_SPATIAL_OBSERVE
#ifdef GF2VR_ACTIVITY_UI_REGISTRATION_OBSERVE
#error Select only the current bounded Activity UI observation.
#endif
#include "activity_ui_spatial_observe.h"
#endif
#ifdef GF2VR_ACTIVITY_UI_NATIVE_REGISTER_OBSERVE
#include "activity_ui_native_register_observe.h"
#endif
#include "activity_canvas_scope_runtime.h"
#endif
#ifdef GF2VR_PRODUCER_SCOPE_OBSERVE
#ifdef GF2VR_ACTIVITY_UI_DRAW_OBSERVE
#include "ui_draw_boundary_runtime.h"
#endif
#include "producer_scope_observe.h"
#endif
static void stereo_frame_configure(SourceRenderFrame *f) {
    stereo_frame_description=(EyeGpuDescription){0};
    stereo_native_frame.ticket=(EyeGpuTicket){0};stereo_native_frame.targets=(EyeNativeTargets){0};
    f->scope->current=stereo_frame_output_current;
    EyeNativeTargets native={0};
    GF2VR_PREPARE_STEP(810);
    if(!stereo_output.enabled || stereo_output.reserved.serial || !source_render_frame_current(f) ||
       !__atomic_load_n(&panel_device_ready,__ATOMIC_ACQUIRE) ||
       __atomic_load_n(&panel_device_changed,__ATOMIC_ACQUIRE)) return;
    /* Establish the queue contract before any Camera[] replacement. Failed
       admission leaves original cameras untouched and the borrow uninjected. */
    stereo_draw_order.serial=f->serial;
    stereo_draw_order.ready=draw_runtime_identity(&stereo_draw_order.entry);
    GF2VR_PREPARE_STEP(811);
    if(!stereo_draw_order.ready) return;
    GF2VR_PREPARE_STEP(812);
    if(eye_native_read(&stereo_output.native,f->eyes,f->check.ticket.frame_id,
        source_render_frame_current,f,&native) && (GF2VR_PREPARE_STEP(813),source_render_frame_current(f)) &&
       (GF2VR_PREPARE_STEP(814),stereo_describe(panel_device,native.resource[0],native.resource[1],&stereo_frame_description)) &&
       eye_native_format(&native,&stereo_frame_description.format) &&
       (GF2VR_PREPARE_STEP(815),source_render_frame_current(f)) && !__atomic_load_n(&panel_device_changed,__ATOMIC_ACQUIRE) &&
       (GF2VR_PREPARE_STEP(816),xr_eye_config_request(&stereo_xr_config,&stereo_frame_description,f->epoch))) {
        GF2VR_PREPARE_STEP(817);
        stereo_native_frame.ticket=(EyeGpuTicket){f->serial,f->check.ticket.source_generation,f->epoch};
        stereo_native_frame.targets=native;
        EyeGpuTicket lease={f->serial,f->check.ticket.source_generation,f->epoch};
        if(xr_eye_output_claim(&stereo_xr_output,f->check.ticket.frame_id,lease,stereo_output.destinations)) {
            stereo_output.reserved=lease;
            GF2VR_PREPARE_STEP(818);
            if(!stereo_frame_output_current(f) ||
               (GF2VR_PREPARE_STEP(819),!stereo_preflight(panel_device,native.resource[0],native.resource[1],
                                  stereo_output.destinations[0],stereo_output.destinations[1])))
                (void)stereo_reservation_cancel();
            else GF2VR_PREPARE_STEP(820);
        }
    }
    if(stereo_interval_enabled) {
        if(__atomic_load_n(&stereo_interval.state,__ATOMIC_ACQUIRE)==ERI_IDLE)
            stereo_interval_xr_returned=0;
        if(stereo_frame_output_current(f) && (GF2VR_PREPARE_STEP(821),stereo_interval_entry()) &&
           (GF2VR_PREPARE_STEP(822),stereo_interval.last_id<=0x7ffffffd) &&
           (GF2VR_PREPARE_STEP(823),eye_interval_prepare_initialized_copy(&stereo_interval,stereo_interval.last_id+1,stereo_interval.last_id+2,
               stereo_native_frame.ticket,panel_render_tid,1,panel_device,panel_context,
               native.resource[0],native.resource[1],stereo_output.destinations[0],stereo_output.destinations[1],stereo_frame_description.format)))
            {GF2VR_PREPARE_STEP(824);stereo_interval_child=stereo_draw_order.entry.child;}
        /* Failed preparation makes Camera[] replacement ineligible. */
        f->scope->current=stereo_interval_frame_current;
    }
}
#ifdef BOUNDARY_TEST
/* Fixture stops after real root/ticket eligibility, before managed/native APIs. */
static void (*stereo_output_test_root)(U64);
#endif
static void stereo_output_callback(int id) {
    DWORD saved=GetLastError();
    if(__atomic_load_n(&stereo_output_event.state,__ATOMIC_ACQUIRE)==EEC_QUEUED &&
       __atomic_load_n(&stereo_output_event.issued,__ATOMIC_ACQUIRE) &&
       id==stereo_output_event.event_id &&
       GetCurrentThreadId()==stereo_output_event.render_thread &&
       (U64)draw_runtime_graphics()==stereo_output_child)
        eye_event_callback(&stereo_output_event,id);
    SetLastError(saved);
}
static void stereo_interval_callback(int id) {
    unsigned state=__atomic_load_n(&stereo_interval.state,__ATOMIC_ACQUIRE);
    if((state!=ERI_QUEUED && state!=ERI_ACTIVE) ||
       GetCurrentThreadId()!=stereo_interval.render_thread) return;
    DWORD saved=GetLastError();
    if((U64)draw_runtime_graphics()==stereo_interval_child) {
        static unsigned install_attempted;
        if(id==stereo_interval.begin_id && state==ERI_QUEUED &&
           __atomic_load_n(&stereo_interval.begin_issued,__ATOMIC_ACQUIRE) &&
           !__atomic_load_n(&stereo_interval.abandoned,__ATOMIC_ACQUIRE) && !install_attempted) {
            install_attempted=1;(void)eye_target_hooks_install(stereo_interval.refs[1]);
#if defined(GF2VR_MAIN_STEREO)
            event("eye_dispatch_bridge_ready",eye_dispatch_install(stereo_interval_child,stereo_interval.refs[1]),GetCurrentThreadId());
#endif
#ifdef GF2VR_PREPARE_OBSERVE
            event("native_draw_hook_ready",native_draw_install(stereo_interval_child,(U64)stereo_interval.refs[1]),GetCurrentThreadId());
            event("native_clear_hook_ready",clear_install(stereo_interval_child),GetCurrentThreadId());
#endif
        }
#ifdef GF2VR_PREPARE_OBSERVE
        stereo_interval.watch.observe=1;
        /* Offline record callback6dc4f0 -> wrapper slot2a0 -> packet2797 ->
           consumer5151df names this exact child slot. Numeric read only on
           the already checked render child; no function invocation or patch. */
        static unsigned child_reported;
        if(!child_reported && id==stereo_interval.begin_id) {
            child_reported=1;U64 table=0,draw=0;unsigned valid=0;
            if(draw_runtime_read(0,stereo_interval_child,&table,8) && table) {
                valid=1;
                if(table<=~(U64)0-0x2a0-8 && draw_runtime_read(0,table+0x2a0,&draw,8)) valid|=2;
            }
            event("eye_child_root",stereo_interval.ticket.serial,GetCurrentThreadId());
            event("eye_child_read_mask",valid,GetCurrentThreadId());
            event("eye_child_table",table,GetCurrentThreadId());
            event("eye_child_draw_function",draw,GetCurrentThreadId());
        }
#endif
#ifdef GF2VR_PREPARE_OBSERVE
        if(id==stereo_interval.end_id) native_draw_observation.active=0;
#endif
#ifdef GF2VR_PERF_PRODUCER_OBSERVE
        U64 callback_start=0;
        if(producer_render_records<64 && producer_sample(stereo_interval.ticket.serial))callback_start=producer_clock();
#endif
        eye_interval_callback(&stereo_interval,id);
#ifdef GF2VR_PERF_PRODUCER_OBSERVE
        if(callback_start && id==stereo_interval.begin_id &&
           __atomic_load_n(&stereo_interval.state,__ATOMIC_ACQUIRE)==ERI_ACTIVE) {
            producer_render.ticket=stereo_interval.ticket;producer_render.begin=callback_start;
            producer_render.copy=producer_render.first_poll=producer_render.last_pending=0;
            producer_render.polls=producer_render.pending=0;
            producer_render.ready=producer_render.copied=producer_render.report=0;
        }
        if(callback_start && id==stereo_interval.end_id &&
           producer_render.ticket.serial==stereo_interval.ticket.serial &&
           __atomic_load_n(&stereo_interval.state,__ATOMIC_ACQUIRE)==ERI_PENDING)
            producer_render.copy=producer_clock();
#endif
#ifdef GF2VR_PERF_FENCE_OBSERVE
        if(id==stereo_interval.end_id && stereo_fence_records<64 &&
           producer_sample(stereo_interval.ticket.serial) &&
           __atomic_load_n(&stereo_interval.state,__ATOMIC_ACQUIRE)==ERI_PENDING) {
            if(perf_fence_begin(&stereo_fence_observer,stereo_interval.refs[0],
                stereo_interval.refs[1],stereo_interval.ticket))++stereo_fence_records;
        }
#endif
#ifdef GF2VR_PREPARE_OBSERVE
        if(id==stereo_interval.begin_id && stereo_interval.watch.state==ETW_ACTIVE && stereo_interval.attached)
            native_draw_begin(stereo_interval.ticket.serial);
        static unsigned begins;
        if(id==stereo_interval.begin_id && begins<16) {
            ++begins;
            event("eye_watch_begin_root",stereo_interval.ticket.serial,GetCurrentThreadId());
            event("eye_watch_begin_state",((U64)stereo_interval.watch.state<<32)|stereo_interval.attached,GetCurrentThreadId());
        }
#endif
    }
    SetLastError(saved);
}
static void stereo_interval_gpu_done(const EyeIntervalResult *result) {
#ifdef GF2VR_PERF_PRODUCER_OBSERVE
    U64 ready_stamp=producer_clock();
#endif
#ifdef GF2VR_PERF_FENCE_OBSERVE
    stereo_fence_report_ready=perf_fence_finish(&stereo_fence_observer,result->ticket,&stereo_fence_report);
#endif
    /* Render -> XR handoff may happen before the next Unity tick. XR still
       validates its exact prediction/epoch and current source generation at
       EndFrame. Main retains all engine borrows until interval DONE/take. */
    int visible=result->copied &&
        __atomic_load_n(&stereo_publication_generation,__ATOMIC_ACQUIRE)==result->ticket.generation &&
        !__atomic_load_n(&panel_device_changed,__ATOMIC_ACQUIRE);
    stereo_interval_xr_returned=xr_eye_output_finish(&stereo_xr_output,result->ticket,visible);
#ifdef GF2VR_PERF_FENCE_OBSERVE
    /* Defer file I/O until poll publishes DONE, like producer stage records. */
#endif
#ifdef GF2VR_PERF_PRODUCER_OBSERVE
    if(producer_render_records<64 && producer_render.ticket.serial==result->ticket.serial &&
       producer_render.ticket.generation==result->ticket.generation && producer_render.ticket.epoch==result->ticket.epoch) {
        /* Publish interval DONE before any diagnostic file I/O, so logging
           cannot itself extend the main-owner wait for this interval. */
        producer_render.ready=ready_stamp;producer_render.copied=result->copied;producer_render.report=1;
    }
#endif
#ifdef GF2VR_PREPARE_OBSERVE
    native_draw_report(result->ticket.serial);
    clear_report(result->ticket.serial);
    static unsigned observed;
    if(observed<16) {
        ++observed;
        event("eye_watch_done_root",result->ticket.serial,GetCurrentThreadId());
        event("eye_watch_calls",((U64)stereo_interval.watch.draw_calls<<32)|stereo_interval.watch.clear_calls,GetCurrentThreadId());
        event("eye_watch_raw_empty",((U64)stereo_interval.watch.raw_draw_calls<<32)|stereo_interval.watch.empty_draw_calls,GetCurrentThreadId());
        event("eye_watch_predicated",stereo_interval.watch.predicated,GetCurrentThreadId());
        event("eye_watch_coverage",((U64)result->draw_mask<<32)|result->clear_mask,GetCurrentThreadId());
    }
#endif
#ifdef GF2VR_MAIN_STEREO
    static U64 count;
    if(++count<=8 || count%100==0) {
        char line[768],*p=source_prefix(line,"stereo_interval_complete");
        p=literal(p,",\"serial\":");p=decimal(p,result->ticket.serial);
        p=literal(p,",\"generation\":");p=decimal(p,result->ticket.generation);
        p=literal(p,",\"epoch\":");p=decimal(p,result->ticket.epoch);
        p=literal(p,",\"draw_mask\":");p=decimal(p,result->draw_mask);
        p=literal(p,",\"clear_mask\":");p=decimal(p,result->clear_mask);
        p=literal(p,",\"copied\":");p=decimal(p,result->copied);
        p=literal(p,",\"xr_returned\":");p=decimal(p,stereo_interval_xr_returned);
        p=literal(p,"}\n");source_log(line,p);
    }
#endif
}
static void stereo_output_render_tick(void) {
    stereo_two_render_tick();
#ifdef GF2VR_PERF_PRODUCER_OBSERVE
    int trace_poll=GetCurrentThreadId()==stereo_interval.render_thread && producer_render_records<64 &&
        __atomic_load_n(&stereo_interval.state,__ATOMIC_ACQUIRE)==ERI_PENDING &&
        producer_render.ticket.serial==stereo_interval.ticket.serial;
    if(trace_poll) {
        ++producer_render.polls;
        if(!producer_render.first_poll)producer_render.first_poll=producer_clock();
    }
#endif
    eye_interval_poll_notify(&stereo_interval,stereo_interval_gpu_done);
#ifdef GF2VR_PERF_FENCE_OBSERVE
    if(GetCurrentThreadId()==stereo_interval.render_thread && stereo_fence_report_ready) {
        stereo_fence_report_ready=0;
        const char *keys[]={"signal","last_pending","first_complete","render_ready","samples","stage","result"};
        U64 values[]={stereo_fence_report.signal,stereo_fence_report.last_pending,stereo_fence_report.first_complete,
            stereo_fence_report.render_ready,stereo_fence_report.samples,stereo_fence_report.stage,stereo_fence_report.result};
        producer_record("perf_fence_complete",stereo_fence_report.ticket,0,keys,values,7);
    }
#endif
#ifdef GF2VR_PERF_PRODUCER_OBSERVE
    if(trace_poll && __atomic_load_n(&stereo_interval.state,__ATOMIC_ACQUIRE)==ERI_PENDING) {
        ++producer_render.pending;producer_render.last_pending=producer_clock();
    }
    if(GetCurrentThreadId()==stereo_interval.render_thread && producer_render.report) {
        producer_render.report=0;++producer_render_records;
        const char *keys[]={"begin","copy_queued","first_poll","last_pending","ready","polls","pending","copied"};
        U64 values[]={producer_render.begin,producer_render.copy,producer_render.first_poll,
            producer_render.last_pending,producer_render.ready,producer_render.polls,producer_render.pending,producer_render.copied};
        producer_record("perf_eye_render_stages",producer_render.ticket,0,keys,values,8);
    }
#endif
    eye_event_poll(&stereo_output_event);
#ifdef GF2VR_DRAW_ORDER_OBSERVE
    draw_order_poll();
#endif
}
#ifdef GF2VR_PERF_FENCE_DELIVERY
static void stereo_output_fence_tick(void) {
    eye_interval_fence_notify(&stereo_interval,stereo_interval_gpu_done);
}
#endif
#ifdef GF2VR_PERF_REVOKE_OBSERVE
/* Main-owned snapshots only. No eligibility/ownership decisions use telemetry. */
static void stereo_revoke_record(const char *name,unsigned line,unsigned state,U64 generation) {
    static unsigned count;
    if(GetCurrentThreadId()!=stereo_pipeline.owner_thread || count>=64) return;
    ++count;DWORD saved=GetLastError();
    char buffer[1024],*p=source_prefix(buffer,name);
    p=literal(p,",\"line\":");p=decimal(p,line);
    p=literal(p,",\"manual\":");p=decimal(p,state);
    p=literal(p,",\"generation\":");p=decimal(p,generation);
    p=literal(p,",\"serial\":");p=decimal(p,stereo_pipeline.render.serial);
    p=literal(p,",\"frame_serial\":");p=decimal(p,stereo_source_frame.serial);
    p=literal(p,",\"observed\":");p=decimal(p,stereo_source_observed);
    p=literal(p,",\"admission_stage\":");p=decimal(p,revoke_admission_stage);
    p=literal(p,",\"prepare_stage\":");p=decimal(p,revoke_prepare_stage);
    p=literal(p,",\"interval_state\":");p=decimal(p,__atomic_load_n(&stereo_interval.state,__ATOMIC_ACQUIRE));
    p=literal(p,"}\n");source_log(buffer,p);SetLastError(saved);
}
static void stereo_revoke_trace(void *grant,unsigned line) {
    MainTestGrant *g=grant;
    if(g && g->state!=MTG_OFF)
        stereo_revoke_record("perf_grant_revoke",line,g->state,
            __atomic_load_n(&stereo_publication_generation,__ATOMIC_ACQUIRE));
}
static void stereo_clear_trace(void *source) {
    SourceGeneration *s=source;
    if(s && s->selection.eligible)
        stereo_revoke_record("perf_source_clear",0,stereo_source.manual?stereo_source.manual->state:0,
            s->selection.generation);
}
#endif
static int stereo_output_matches(EyeGpuTicket t,const SourceRenderFrame *f) {
    return f->serial && t.serial==f->serial && t.generation==f->check.ticket.source_generation &&
        t.epoch==f->epoch && f->root_done && f->injected && f->borrowed && !f->busy;
}
#ifdef GF2VR_PERF_PREPARE_RECOVERY
static void stereo_preparation_record(const char *name,U64 generation,unsigned phase) {
    static unsigned count;
    if(GetCurrentThreadId()!=stereo_pipeline.owner_thread || count>=16) return;
    ++count;DWORD saved=GetLastError();
    char line[512],*p=source_prefix(line,name);
    p=literal(p,",\"serial\":");p=decimal(p,stereo_pipeline.render.serial);
    p=literal(p,",\"generation\":");p=decimal(p,generation);
    p=literal(p,",\"phase\":");p=decimal(p,phase);
    p=literal(p,"}\n");source_log(line,p);SetLastError(saved);
}
static void stereo_refresh_failure(void *owner,unsigned phase) {
    CameraDataOwner *o=owner;
    stereo_preparation_record("perf_refresh_failure",o->generation,phase);
}
static void stereo_preparation_reconciled(void *preparation) {
    SourceEyePreparation *p=preparation;
    stereo_preparation_record("perf_preparation_reconciled",p->generation,0);
}
#endif
#ifdef GF2VR_PERF_COMPLETION_SERVICE
static void stereo_output_render_tail(void) {
#ifdef GF2VR_PERF_SERVICE_OBSERVE
    static unsigned tail_records;
    EyeGpuTicket trace={0};U64 before=0,stamp=0,flags=0;
    if(GetCurrentThreadId()==stereo_interval.render_thread && tail_records<64 &&
       __atomic_load_n(&stereo_interval.state,__ATOMIC_ACQUIRE)==ERI_PENDING &&
       producer_sample(stereo_interval.ticket.serial)) {
        trace=stereo_interval.ticket;before=ERI_PENDING;stamp=producer_clock();
        flags=(stereo_interval.gpu.thread==GetCurrentThreadId()?1u:0u)|
            (stereo_interval.gpu.state==EGC_PENDING?2u:0u)|
            (stereo_interval.gpu.context?4u:0u)|(stereo_interval.gpu.device?8u:0u);
    }
#endif
    eye_interval_service_notify(&stereo_interval,stereo_interval_gpu_done);
#ifdef GF2VR_PERF_SERVICE_OBSERVE
    U64 stopped=stamp?producer_clock():0;
    unsigned after=__atomic_load_n(&stereo_interval.state,__ATOMIC_ACQUIRE);
#endif
    stereo_output_render_tick();
#ifdef GF2VR_PERF_SERVICE_OBSERVE
    if(stamp) {
        ++tail_records;
        const char *keys[]={"service_enter","service_exit","before","after","guards"};
        U64 values[]={stamp,stopped,before,after,flags};
        producer_record("perf_eye_service_outcome",trace,0,keys,values,5);
    }
#endif
}
#endif
static int stereo_draw_current(U64 serial,DrawQueueIdentity *now) {
    return serial && stereo_draw_order.serial==serial && stereo_draw_order.ready &&
        stereo_submit_scope.returned_serial==serial &&
        draw_runtime_identity(now) && draw_queue_same(stereo_draw_order.entry,*now);
}
static void stereo_output_root_finished(U64 serial) {
    SourceRenderFrame *f=&stereo_source_frame;
    if(GetCurrentThreadId()!=stereo_pipeline.owner_thread || !stereo_output.enabled || stereo_output.busy ||
       stereo_output.completed.serial || !f->cpu_ok || !stereo_output_matches(stereo_output.submitted,f) ||
       f->serial!=serial) return;
#ifdef BOUNDARY_TEST
    if(stereo_output_test_root) {stereo_output_test_root(serial);return;}
#endif
    if(stereo_interval_enabled) return; /* Interval observations are not a submitted provider. */
    if(!source_render_frame_current(f) ||
       __atomic_load_n(&stereo_output_event.state,__ATOMIC_ACQUIRE)!=EEC_IDLE ||
       !__atomic_load_n(&panel_device_ready,__ATOMIC_ACQUIRE) ||
       __atomic_load_n(&panel_device_changed,__ATOMIC_ACQUIRE) || !panel_render_tid ||
       !unity || unity[0x94d840]!=0xe9 || *(unsigned *)(unity+0x94d841)!=0xffd2cc4bU ||
       stereo_output_event.last_event_id==0x7fffffff) return;
    DWORD saved=GetLastError();stereo_output.busy=1;
    DrawQueueIdentity order={0};
    EyeNativeTargets native={0};
    if(stereo_draw_current(serial,&order) &&
       eye_native_read(&stereo_output.native,f->eyes,f->check.ticket.frame_id,
                       source_render_frame_current,f,&native) &&
       source_render_frame_current(f) && stereo_native_same(f,&native) &&
       stereo_output_matches(stereo_output.submitted,f) &&
       !__atomic_load_n(&panel_device_changed,__ATOMIC_ACQUIRE) &&
       stereo_reserved_matches(f)) {
        int id=stereo_output_event.last_event_id+1;
        if(eye_gpu_preflight(panel_device,native.resource[0],native.resource[1],
                             stereo_output.destinations[0],stereo_output.destinations[1]) &&
           source_render_frame_current(f) && stereo_draw_current(serial,&order) &&
           !__atomic_load_n(&panel_device_changed,__ATOMIC_ACQUIRE) &&
           eye_event_prepare(&stereo_output_event,id,stereo_output.submitted,panel_render_tid,1,
                            panel_device,panel_context,native.resource[0],native.resource[1],
                            stereo_output.destinations[0],stereo_output.destinations[1])) {
            /* Same checked callback+ID entry used by accepted panel setup.
               The submitted ticket above is a separate prerequisite. */
            stereo_output_child=order.child;
            eye_event_issue(&stereo_output_event,(void (*)(void (*)(int),int))(unity+0x94d840),stereo_output_callback);
        } else {
            /* No event/GPU destination use occurred. Source borrow still needs
               actual engine drain and is deliberately not released here. */
            (void)stereo_reservation_cancel();
        }
    }
    stereo_output.busy=0;SetLastError(saved);
}
/* Ordered retirement can reclaim an injected pair without claiming that both
   eyes drew fresh pixels. Failed/ambiguous roots remain quarantined. */
static void stereo_output_drain_root(U64 serial) {
    SourceRenderFrame *f=&stereo_source_frame;
    if(stereo_interval_enabled) return; /* One drain owner per source pair. */
    if(!stereo_output.enabled || stereo_output.busy || stereo_output.completed.serial ||
       GetCurrentThreadId()!=stereo_pipeline.owner_thread || f->serial!=serial || !f->cpu_ok ||
       !f->root_done || !f->injected || !source_render_frame_current(f) ||
       __atomic_load_n(&stereo_output_event.state,__ATOMIC_ACQUIRE)!=EEC_IDLE ||
       !__atomic_load_n(&panel_device_ready,__ATOMIC_ACQUIRE) ||
       __atomic_load_n(&panel_device_changed,__ATOMIC_ACQUIRE) || !panel_render_tid ||
       stereo_output_event.last_event_id==0x7fffffff) return;
    DWORD saved=GetLastError();stereo_output.busy=1;
    DrawQueueIdentity order={0};EyeNativeTargets native={0};unsigned char issue[5]={0};
    if(stereo_draw_current(serial,&order) &&
       draw_runtime_read(0,(U64)unity+0x94d840,issue,5) && issue[0]==0xe9 &&
       issue[1]==0x4b && issue[2]==0xcc && issue[3]==0xd2 && issue[4]==0xff &&
       eye_native_read(&stereo_output.native,f->eyes,f->check.ticket.frame_id,
                       source_render_frame_current,f,&native) &&
       stereo_native_same(f,&native) && stereo_draw_current(serial,&order) && source_render_frame_current(f) &&
       !__atomic_load_n(&panel_device_changed,__ATOMIC_ACQUIRE)) {
        EyeGpuTicket ticket={serial,f->check.ticket.source_generation,f->epoch};
        if(eye_event_prepare_drain(&stereo_output_event,stereo_output_event.last_event_id+1,
            ticket,panel_render_tid,1,panel_device,panel_context,native.resource[0],native.resource[1])) {
            stereo_output_child=order.child;
            eye_event_issue(&stereo_output_event,(void (*)(void (*)(int),int))(unity+0x94d840),stereo_output_callback);
        }
    }
    stereo_output.busy=0;SetLastError(saved);
}
static void stereo_output_owner_tick(void) {
    if(GetCurrentThreadId()!=stereo_pipeline.owner_thread || stereo_output.busy) return;
#ifdef GF2VR_PERF_FENCE_DELIVERY
    stereo_output_fence_tick();
#endif
#if defined(GF2VR_REENTRY_OBSERVE) || defined(GF2VR_PERF_REVOKE_OBSERVE)
    /* FAULT release-publishes an immutable quarantined render-owner snapshot.
       Do not read these fields while a callback/poll can still mutate them. */
    static unsigned fault_reported;
    if(!fault_reported && __atomic_load_n(&stereo_interval.state,__ATOMIC_ACQUIRE)==ERI_FAULT) {
        fault_reported=1;DWORD saved=GetLastError();
        char line[1024],*p=source_prefix(line,"reentry_interval_fault");
        p=literal(p,",\"serial\":");p=decimal(p,stereo_interval.ticket.serial);
        p=literal(p,",\"generation\":");p=decimal(p,stereo_interval.ticket.generation);
        p=literal(p,",\"epoch\":");p=decimal(p,stereo_interval.ticket.epoch);
        p=literal(p,",\"reason\":");p=decimal(p,stereo_interval.fault_reason);
        p=literal(p,",\"gpu_state\":");p=decimal(p,stereo_interval.gpu.state);
        p=literal(p,",\"watch_state\":");p=decimal(p,stereo_interval.watch.state);
        p=literal(p,",\"begin\":");p=decimal(p,__atomic_load_n(&stereo_interval.begin_issued,__ATOMIC_ACQUIRE));
        p=literal(p,",\"end\":");p=decimal(p,__atomic_load_n(&stereo_interval.end_issued,__ATOMIC_ACQUIRE));
        p=literal(p,",\"abandoned\":");p=decimal(p,__atomic_load_n(&stereo_interval.abandoned,__ATOMIC_ACQUIRE));
        p=literal(p,",\"draw_mask\":");p=decimal(p,stereo_interval.draw_mask);
        p=literal(p,",\"clear_mask\":");p=decimal(p,stereo_interval.clear_mask);
        p=literal(p,",\"copied\":");p=decimal(p,stereo_interval.copied);
        p=literal(p,"}\n");source_log(line,p);SetLastError(saved);
    }
#endif
    if(__atomic_load_n(&stereo_interval.state,__ATOMIC_ACQUIRE)==ERI_DONE) {
        if(!stereo_output_matches(stereo_interval.ticket,&stereo_source_frame)) return;
        DWORD saved=GetLastError();stereo_output.busy=1;
        unsigned returned=stereo_interval_xr_returned;
#ifdef GF2VR_PERF_FENCE_TIMING
        EyeGpuTiming timing=stereo_interval.timing;
#endif
#ifdef GF2VR_PERF_PRODUCER_OBSERVE
        U64 owner_stamp=producer_clock();
#endif
        if(eye_interval_take(&stereo_interval,&stereo_interval_result)) {
#ifdef GF2VR_PERF_FENCE_TIMING
#ifdef GF2VR_PERF_ROOT_FENCE_ORDER
            if(perf_fence_timing.root_serial==stereo_interval_result.ticket.serial && perf_fence_timing.root_at) {
                unsigned cls=perf_root_fence_class(perf_fence_timing.root_at,timing.last_pending,timing.first_complete);
                ++perf_fence_timing.root_count;
                if(cls==1) ++perf_fence_timing.root_pending_after;
                else if(cls==2) ++perf_fence_timing.root_inside_bracket;
                else if(cls==3) ++perf_fence_timing.root_complete_before;
                else ++perf_fence_timing.root_unmeasured;
                perf_fence_timing.root_serial=perf_fence_timing.root_at=0;
            }
#endif
            if(timing.signal && timing.first_complete>=timing.signal) {
                U64 duration=timing.first_complete-timing.signal;
                if(!perf_fence_timing.count || duration<perf_fence_timing.min_complete)
                    perf_fence_timing.min_complete=duration;
                if(duration>perf_fence_timing.max_complete)perf_fence_timing.max_complete=duration;
                perf_fence_timing.sum_complete+=duration;
                if(timing.last_pending && timing.first_complete>=timing.last_pending) {
                    U64 bracket=timing.first_complete-timing.last_pending;
                    if(!perf_fence_timing.bracket_count || bracket<perf_fence_timing.min_bracket)
                        perf_fence_timing.min_bracket=bracket;
                    if(bracket>perf_fence_timing.max_bracket)perf_fence_timing.max_bracket=bracket;
                    perf_fence_timing.sum_bracket+=bracket;
                    ++perf_fence_timing.bracket_count;
                }
                perf_fence_timing.polls+=timing.polls;
                perf_fence_timing.pending+=timing.pending;
                ++perf_fence_timing.count;
            } else ++perf_fence_timing.fallback;
#endif
#ifdef GF2VR_PERF_PRODUCER_OBSERVE
            if(producer_owner_records<64 && producer_sample(stereo_interval_result.ticket.serial)) {
                ++producer_owner_records;const char *keys[]={"take"};
                producer_record("perf_eye_owner_take",stereo_interval_result.ticket,0,keys,&owner_stamp,1);
            }
#endif
            if(source_render_frame_gpu_drained(&stereo_source_frame,stereo_interval_result.ticket.serial))
                stereo_native_forget(stereo_interval_result.ticket.serial);
            stereo_output.completed=stereo_interval_result.ticket;
            /* A delayed/retried main-owner return never revives visibility. */
            if(returned || xr_eye_output_finish(&stereo_xr_output,stereo_output.completed,0)) {
                stereo_output.reserved=(EyeGpuTicket){0};stereo_output.completed=(EyeGpuTicket){0};
                stereo_output.destinations[0]=stereo_output.destinations[1]=0;
            }
        }
        stereo_output.busy=0;SetLastError(saved);return;
    }
    /* A refused XR handoff retains its exact ticket. A later owner tick may
       return that lease, but must never revive its old presentation eligibility. */
    if(stereo_output.completed.serial) {
        DWORD saved=GetLastError();
        if(xr_eye_output_finish(&stereo_xr_output,stereo_output.completed,0)) {
            stereo_output.reserved=(EyeGpuTicket){0};
            stereo_output.completed=(EyeGpuTicket){0};
            stereo_output.destinations[0]=stereo_output.destinations[1]=0;
        }
        SetLastError(saved);return;
    }
    if(
       __atomic_load_n(&stereo_output_event.state,__ATOMIC_ACQUIRE)!=EEC_DONE) return;
    /* A retired source/XR epoch can still drain, but it cannot release a different
       frame. Do not compare against current selection to reject safe old drain. */
    if(!stereo_output_matches(stereo_output_event.ticket,&stereo_source_frame)) return;
    DWORD saved=GetLastError();EyeGpuTicket result={0};stereo_output.busy=1;
    if(stereo_output_event.drain_only) {
        if(eye_event_take_drain(&stereo_output_event,&result)) {
            if(source_render_frame_gpu_drained(&stereo_source_frame,result.serial)) stereo_native_forget(result.serial);
            if(stereo_output.submitted.serial==result.serial &&
               stereo_output.submitted.generation==result.generation &&
               stereo_output.submitted.epoch==result.epoch)
                stereo_output.submitted=(EyeGpuTicket){0};
        }
        stereo_output.busy=0;SetLastError(saved);return;
    }
    /* Safe resource drain may outlive its source; only current successful
       output may authorize a layer. Check before returning the eye borrow. */
    int visible=0;
    if(eye_event_take(&stereo_output_event,&result)) {
        visible=stereo_source_frame.cpu_ok && source_render_frame_current(&stereo_source_frame) &&
            !__atomic_load_n(&panel_device_changed,__ATOMIC_ACQUIRE);
        stereo_output.completed=result;
        if(source_render_frame_gpu_drained(&stereo_source_frame,result.serial)) stereo_native_forget(result.serial);
        if(stereo_output.submitted.serial==result.serial &&
           stereo_output.submitted.generation==result.generation &&
           stereo_output.submitted.epoch==result.epoch)
            stereo_output.submitted=(EyeGpuTicket){0};
    }
    if(stereo_output.completed.serial && xr_eye_output_finish(&stereo_xr_output,stereo_output.completed,visible)) {
        stereo_output.reserved=(EyeGpuTicket){0};
        stereo_output.completed=(EyeGpuTicket){0};
        stereo_output.destinations[0]=stereo_output.destinations[1]=0;
    }
    stereo_output.busy=0;SetLastError(saved);
}
#ifdef GF2VR_PERF_ROOT_CADENCE
static struct {
    U64 start,roots,retained,offered,armed,injected,intervals[ERI_FAULT+1];
    unsigned reports,active;
} perf_root;
#endif
static void stereo_root_begin(U64 call,int sample) {
#ifdef GF2VR_PRODUCER_PREPARE_OWNERSHIP
    /* Quiescent cold bridge preparation precedes camera_pipeline_begin and
       private reservation/borrow; per-frame Arm never performs installation. */
    producer_scope_preflight();
#endif
#ifdef GF2VR_TWO_FLIGHT_ROOT_OBSERVE
    if(GetCurrentThreadId()==stereo_pipeline.owner_thread && stereo_live &&
       stereo_live->pair_count==EFO_CAPACITY && stereo_source.manual &&
       stereo_source.manual->state==MTG_ACTIVE && two_root_note.reports<24)
        two_root_begin();
#endif
#ifdef GF2VR_PERF_ROOT_CADENCE
    /* Main-owner snapshots only. XR/interval state is read atomically; no
       managed calls, graphics commands, waits or cross-thread payload reads. */
    perf_root.active=GetCurrentThreadId()==stereo_pipeline.owner_thread &&
        stereo_source.enabled && stereo_source.manual &&
        stereo_source.manual->state==MTG_ACTIVE && perf_root.reports<24;
    if(!perf_root.active && GetCurrentThreadId()==stereo_pipeline.owner_thread && perf_root.start) {
        unsigned reports=perf_root.reports;
        perf_root=(__typeof__(perf_root)){0};perf_root.reports=reports;
#ifdef GF2VR_PERF_FENCE_TIMING
        perf_fence_timing=(__typeof__(perf_fence_timing)){0};
#endif
    }
    if(perf_root.active) {
        U64 now=GetTickCount64();if(!perf_root.start)perf_root.start=now;
        ++perf_root.roots;
        perf_root.retained+=stereo_source_frame.serial!=0;
        perf_root.offered+=__atomic_load_n(&stereo_xr_output.state,__ATOMIC_ACQUIRE)==XEO_OFFERED;
        unsigned state=__atomic_load_n(&stereo_interval.state,__ATOMIC_ACQUIRE);
        if(state<=ERI_FAULT)++perf_root.intervals[state];
#ifdef GF2VR_PERF_ROOT_FENCE_ORDER
        if(state==ERI_PENDING && stereo_source_frame.serial &&
           stereo_interval.ticket.serial==stereo_source_frame.serial &&
           perf_fence_timing.root_serial!=stereo_source_frame.serial) {
            DWORD saved=GetLastError();long long stamp=0;
            if(QueryPerformanceCounter(&stamp)) {
                perf_fence_timing.root_serial=stereo_source_frame.serial;
                perf_fence_timing.root_at=(U64)stamp;
            }
            SetLastError(saved);
        }
#endif
    }
#endif
#ifdef GF2VR_PERF_SERVICE_OBSERVE
    static unsigned root_records;
    /* Main-owned frame/reservation; interval exposes only its atomic state.
       Inspect before beginning a new scope, never retire from this probe. */
    if(GetCurrentThreadId()==stereo_pipeline.owner_thread && root_records<64 &&
       stereo_source.enabled && stereo_source.manual && stereo_source.manual->state==MTG_ACTIVE && producer_sample(call)) {
        ++root_records;
        const char *keys[]={"root_enter","retained_serial","interval_state","reserved_serial"};
        U64 values[]={producer_clock(),stereo_source_frame.serial,
            __atomic_load_n(&stereo_interval.state,__ATOMIC_ACQUIRE),stereo_output.reserved.serial};
        producer_record("perf_eye_root_admission",(EyeGpuTicket){call,0,0},0,keys,values,4);
    }
#endif
    (void)sample;stereo_cpu_returned=0;
    if(camera_pipeline_begin(&stereo_pipeline,GetCurrentThreadId(),call)) {
        stereo_two.root_serial=0;stereo_two.root_slot=EFO_CAPACITY;
#ifdef GF2VR_COMPOSITION_OBSERVE
        composition_request_serial=0;
#endif
#ifdef GF2VR_ORIENTATION_OBSERVE
        orientation_begin(call);
#endif
#ifdef GF2VR_CAMERA_PLACEMENT_OBSERVE
        placement_begin(call);
#endif
        stereo_prepare_attempted=0;
        stereo_draw_order.serial=0;stereo_draw_order.ready=0;
        stereo_source_observed=0;
#ifdef GF2VR_ACTIVITY_PRIVATE_BATCH
        activity_private_batch_begin(&stereo_private_batch);
#endif
#ifdef GF2VR_SUBMIT_BOUNDARY
        submit_boundary=(SubmitBoundary){0};
#endif
        stereo_submit_scope.context=0;stereo_submit_scope.serial=call;stereo_submit_scope.returned_serial=0;
        stereo_submit_scope.depth=stereo_submit_scope.submit_depth=stereo_submit_scope.matches=stereo_submit_scope.ambiguous=0;
    }
}
static void stereo_root_end(int failed) {
    U64 serial=stereo_pipeline.render.serial;
    unsigned injected=stereo_pipeline.render.injected;
#ifdef GF2VR_CAMERA_PLACEMENT_OBSERVE
    if(GetCurrentThreadId()==stereo_pipeline.owner_thread)
        placement_end(serial,injected,failed);
#endif
#ifdef GF2VR_PERF_ROOT_CADENCE
    if(perf_root.active) {
        perf_root.armed+=stereo_pipeline.armed!=0;
        perf_root.injected+=injected!=0;
        U64 now=GetTickCount64();
        if(now>=perf_root.start && now-perf_root.start>=2000) {
            DWORD saved=GetLastError();char line[2000],*p=source_prefix(line,"perf_root_cadence");
            p=literal(p,",\"elapsed_ms\":");p=decimal(p,now-perf_root.start);
            p=literal(p,",\"roots\":");p=decimal(p,perf_root.roots);
            p=literal(p,",\"retained\":");p=decimal(p,perf_root.retained);
            p=literal(p,",\"offered\":");p=decimal(p,perf_root.offered);
            p=literal(p,",\"armed\":");p=decimal(p,perf_root.armed);
            p=literal(p,",\"injected\":");p=decimal(p,perf_root.injected);
            p=literal(p,",\"interval_idle\":");p=decimal(p,perf_root.intervals[ERI_IDLE]);
            p=literal(p,",\"interval_pending\":");p=decimal(p,perf_root.intervals[ERI_PENDING]);
            p=literal(p,",\"interval_done\":");p=decimal(p,perf_root.intervals[ERI_DONE]);
#ifdef GF2VR_PERF_FENCE_TIMING
            long long frequency=0;
            (void)QueryPerformanceFrequency(&frequency);
            p=literal(p,",\"qpc_frequency\":");p=decimal(p,frequency>0?(U64)frequency:0);
            p=literal(p,",\"fence_count\":");p=decimal(p,perf_fence_timing.count);
            p=literal(p,",\"fence_fallback\":");p=decimal(p,perf_fence_timing.fallback);
            p=literal(p,",\"signal_complete_sum\":");p=decimal(p,perf_fence_timing.sum_complete);
            p=literal(p,",\"signal_complete_min\":");p=decimal(p,perf_fence_timing.min_complete);
            p=literal(p,",\"signal_complete_max\":");p=decimal(p,perf_fence_timing.max_complete);
            p=literal(p,",\"pending_complete_sum\":");p=decimal(p,perf_fence_timing.sum_bracket);
            p=literal(p,",\"pending_complete_count\":");p=decimal(p,perf_fence_timing.bracket_count);
            p=literal(p,",\"pending_complete_min\":");p=decimal(p,perf_fence_timing.min_bracket);
            p=literal(p,",\"pending_complete_max\":");p=decimal(p,perf_fence_timing.max_bracket);
            p=literal(p,",\"fence_polls\":");p=decimal(p,perf_fence_timing.polls);
            p=literal(p,",\"fence_pending\":");p=decimal(p,perf_fence_timing.pending);
#ifdef GF2VR_PERF_ROOT_FENCE_ORDER
            p=literal(p,",\"root_order_count\":");p=decimal(p,perf_fence_timing.root_count);
            p=literal(p,",\"root_pending_after\":");p=decimal(p,perf_fence_timing.root_pending_after);
            p=literal(p,",\"root_inside_bracket\":");p=decimal(p,perf_fence_timing.root_inside_bracket);
            p=literal(p,",\"root_complete_before\":");p=decimal(p,perf_fence_timing.root_complete_before);
            p=literal(p,",\"root_unmeasured\":");p=decimal(p,perf_fence_timing.root_unmeasured);
#endif
#endif
            p=literal(p,"}\n");source_log(line,p);SetLastError(saved);
            unsigned reports=perf_root.reports+1;
            perf_root=(__typeof__(perf_root)){0};perf_root.reports=reports;
#ifdef GF2VR_PERF_FENCE_TIMING
#ifdef GF2VR_PERF_ROOT_FENCE_ORDER
            U64 root_serial=perf_fence_timing.root_serial,root_at=perf_fence_timing.root_at;
#endif
            perf_fence_timing=(__typeof__(perf_fence_timing)){0};
#ifdef GF2VR_PERF_ROOT_FENCE_ORDER
            perf_fence_timing.root_serial=root_serial;perf_fence_timing.root_at=root_at;
#endif
#endif
        }
    }
    perf_root.active=0;
#endif
#ifdef GF2VR_ORIENTATION_OBSERVE
    if(GetCurrentThreadId()==stereo_pipeline.owner_thread) {
        orientation_matrix_observe(&stereo_source_frame,serial,injected && !failed);
        orientation_end(serial,injected && !failed);
    }
#endif
    /* No accepted source in this actual render opportunity: do not carry a
       pending key request across a later scene/root. ACTIVE retains existing
       source replacement/failure revocation rules below. */
#ifdef GF2VR_ACTIVITY_PRIVATE_BATCH
    unsigned independent_private=0;
    if(GetCurrentThreadId()==stereo_pipeline.owner_thread)
        independent_private=activity_private_batch_independent(&stereo_private_batch,failed);
    if(!independent_private && GetCurrentThreadId()==stereo_pipeline.owner_thread && stereo_source.manual &&
#else
    if(GetCurrentThreadId()==stereo_pipeline.owner_thread && stereo_source.manual &&
#endif
       stereo_source.manual->state==MTG_REQUESTED) {
#ifdef GF2VR_MAIN_STEREO
        event("main_f8_unconsumed_root",serial,GetCurrentThreadId());
#endif
        stereo_manual_revoke();
    }
    if(stereo_source.generation && GetCurrentThreadId()==stereo_source.generation->owner_thread &&
#ifdef GF2VR_ACTIVITY_PRIVATE_BATCH
       (failed || (!stereo_source_observed && !independent_private))
#else
       (failed || !stereo_source_observed)
#endif
       ) {
        source_generation_invalidate(stereo_source.generation);
        if(stereo_source.manual) main_test_revoke(stereo_source.manual);
    }
    /* Unwind may skip native after-return paths. Drop borrowed context and
       reject incomplete/reentrant forwarding before any later managed work. */
#ifdef GF2VR_SUBMIT_BOUNDARY
    if(injected && GetCurrentThreadId()==stereo_pipeline.owner_thread) {
        static unsigned good_samples,bad_samples;
        unsigned bad=failed || stereo_submit_scope.depth || stereo_submit_scope.submit_depth ||
            stereo_submit_scope.ambiguous || stereo_submit_scope.returned_serial!=serial || !stereo_draw_order.ready;
        if((bad && bad_samples<8) || (!bad && good_samples<2)) {
            if(bad) ++bad_samples;else ++good_samples;
            DWORD saved=GetLastError();char line[1400],*p=source_prefix(line,"submit_boundary");
            p=literal(p,",\"serial\":");p=decimal(p,serial);
            p=literal(p,",\"generation\":");p=decimal(p,stereo_source_frame.check.ticket.source_generation);
            p=literal(p,",\"bad\":");p=decimal(p,bad);
            p=literal(p,",\"failed\":");p=decimal(p,failed!=0);
            p=literal(p,",\"render_depth\":");p=decimal(p,stereo_submit_scope.depth);
            p=literal(p,",\"submit_depth\":");p=decimal(p,stereo_submit_scope.submit_depth);
            p=literal(p,",\"render_calls\":");p=decimal(p,submit_boundary.render_calls);
            p=literal(p,",\"selected_renders\":");p=decimal(p,submit_boundary.selected_renders);
            p=literal(p,",\"submit_calls\":");p=decimal(p,submit_boundary.calls);
            p=literal(p,",\"selected_submits\":");p=decimal(p,submit_boundary.selected_calls);
            p=literal(p,",\"mismatches\":");p=decimal(p,submit_boundary.context_mismatches);
            p=literal(p,",\"matches\":");p=decimal(p,stereo_submit_scope.matches);
            p=literal(p,",\"ambiguous\":");p=decimal(p,stereo_submit_scope.ambiguous);
            p=literal(p,",\"reason_mask\":");p=decimal(p,submit_boundary.reason_mask);
            p=literal(p,",\"returned\":");p=decimal(p,stereo_submit_scope.returned_serial==serial);
            p=literal(p,",\"queue_ready\":");p=decimal(p,stereo_draw_order.ready);
            p=literal(p,",\"expected_context\":");p=hexadecimal(p,submit_boundary.expected_context);
            p=literal(p,",\"observed_context\":");p=hexadecimal(p,submit_boundary.observed_context);
            p=literal(p,"}\n");source_log(line,p);SetLastError(saved);
        }
    }
#endif
    if(failed || stereo_submit_scope.depth || stereo_submit_scope.submit_depth || stereo_submit_scope.ambiguous)
        stereo_submit_scope.returned_serial=0;
#ifdef GF2VR_PRODUCER_SCOPE_OBSERVE
    producer_scope_finish(serial,failed);
#endif
    stereo_submit_scope.context=0;stereo_submit_scope.depth=stereo_submit_scope.submit_depth=0;
    if(stereo_live && stereo_live->pair_count==EFO_CAPACITY) {
        stereo_two_root_finished(serial,failed);
        (void)camera_pipeline_end(&stereo_pipeline,GetCurrentThreadId(),serial,!failed);
        stereo_draw_order.serial=0;stereo_draw_order.ready=0;stereo_forward_error=0;
#ifdef GF2VR_MAIN_TAIL_COMPLETION
        stereo_two_completion_tail();
#endif
#ifdef GF2VR_NAVIGATION_BLOCK_OBSERVE
        if(GetCurrentThreadId()==stereo_pipeline.owner_thread)navigation_block_flush(failed);
#endif
        return;
    }
    if(stereo_source_frame.armed && stereo_source_frame.serial==serial && !stereo_source_frame.root_done)
        stereo_cpu_returned=source_render_frame_finish(&stereo_source_frame,GetCurrentThreadId(),serial,!failed);
    else stereo_cpu_returned=camera_pipeline_end(&stereo_pipeline,GetCurrentThreadId(),serial,!failed);
    if(!injected && stereo_output.reserved.serial==serial) (void)stereo_reservation_cancel();
    if(!injected) stereo_native_forget(serial);
    if(stereo_interval_enabled && stereo_interval.ticket.serial==serial &&
       __atomic_load_n(&stereo_interval.state,__ATOMIC_ACQUIRE)!=ERI_IDLE) {
        if(!__atomic_load_n(&stereo_interval.begin_issued,__ATOMIC_ACQUIRE))
            (void)eye_interval_cancel(&stereo_interval,stereo_interval.ticket);
        else {
            DrawQueueIdentity tail={0};
            int ordered=!failed && injected && stereo_draw_current(serial,&tail) && stereo_interval_entry();
            int copy=ordered && source_render_frame_current(&stereo_source_frame) &&
                stereo_reserved_matches(&stereo_source_frame) &&
                !__atomic_load_n(&panel_device_changed,__ATOMIC_ACQUIRE);
            int issued=ordered && (copy?eye_interval_issue_end:eye_interval_issue_retire)(
                &stereo_interval,stereo_interval.ticket,1,
                (void (*)(void (*)(int),int))(unity+0x94d840),stereo_interval_callback);
            if(!issued) {
#ifdef GF2VR_REENTRY_OBSERVE
                static unsigned abandon_samples;
                if(abandon_samples++<8) {
                    DWORD saved=GetLastError();
                    event("reentry_abandon_serial",serial,GetCurrentThreadId());
                    /* Only existing results/owner-local state: never repeat
                       queue/engine queries to collect this diagnostic. */
                    U64 flags=(U64)(failed!=0)|((U64)(injected!=0)<<1)|
                        ((U64)(ordered!=0)<<2)|((U64)(copy!=0)<<3)|
                        ((U64)stereo_draw_order.ready<<4)|
                        ((U64)(stereo_submit_scope.returned_serial==serial)<<5)|
                        ((U64)__atomic_load_n(&stereo_interval.state,__ATOMIC_ACQUIRE)<<8);
                    event("reentry_abandon_flags",flags,GetCurrentThreadId());SetLastError(saved);
                }
#endif
                eye_interval_abandon(&stereo_interval,stereo_interval.ticket);
            }
        }
    }
    /* end clears render.serial; only the captured root can authorize issuance.
       A later original-only root must not schedule the retained older frame. */
    stereo_output_root_finished(serial);
    /* Failed/stale/missing-submission roots may have used source eyes but no
       destination command was issued. Release ONLY that unused destination
       lease; injected source lifetime still requires independent real drain. */
    if(stereo_output.reserved.serial==serial &&
       __atomic_load_n(&stereo_output_event.state,__ATOMIC_ACQUIRE)==EEC_IDLE)
        (void)stereo_reservation_cancel();
    if(!failed) stereo_output_drain_root(serial);
    stereo_draw_order.serial=0;stereo_draw_order.ready=0;
    stereo_forward_error=0;
#ifdef GF2VR_DRAW_ORDER_OBSERVE
    draw_order_finished(serial,!failed && stereo_submit_scope.returned_serial==serial);
#endif
#ifdef GF2VR_NAVIGATION_BLOCK_OBSERVE
    if(GetCurrentThreadId()==stereo_pipeline.owner_thread)navigation_block_flush(failed);
#endif
}
static void stereo_forward_preserved(void *pipeline,void *context,void *cameras,void *method) {
    unsigned selected=stereo_pipeline.render.active && stereo_pipeline.armed &&
        stereo_pipeline.render.injected && cameras==stereo_pipeline.render.batch.array &&
        pipeline==stereo_pipeline.pipeline && method==stereo_pipeline.method;
    unsigned dual=stereo_two_before_original(pipeline,context,cameras,method);
    if(dual)selected=1;
    int dual_begin=EFO_OK;
    if(dual && !stereo_submit_scope.depth) {
        dual_begin=eye_flight_backend_begin(&stereo_two.backend,&stereo_two.owner,stereo_two.root_slot,
            stereo_two_issue,stereo_two_callback);
#ifdef GF2VR_TWO_FLIGHT_ARM_OBSERVE
        stereo_two.slots[stereo_two.root_slot].arm_stage=dual_begin==EFO_OK?18:17;
        stereo_two.slots[stereo_two.root_slot].arm_result=dual_begin;
#endif
    }
    if(dual && !stereo_submit_scope.depth && dual_begin!=EFO_OK) {
        /* Begin was never queued: do not render an unwatched private batch.
           Forward originals ONCE, not a replay after engine work. */
        stereo_live->flights.slots[stereo_two.root_slot].scope.render.injected=0;
        cameras=stereo_two.original_cameras;selected=0;
    }
#ifdef GF2VR_SUBMISSION_OBSERVE
    if(stereo_hooks.ready && stereo_pipeline.render.active && method==stereo_start.method) selected=1;
#endif
    /* Independent nested renders may submit auxiliary work inside the outer
       pipeline. They cannot replace its borrowed context or count as its
       Submit (depth==1 is still required). Only re-entering the selected
       injected batch makes that outer correlation ambiguous. Its own matched
       Submit return and unchanged graphics queue remain mandatory. */
    if(stereo_submit_scope.depth && selected) {
        stereo_submit_scope.ambiguous=1;
#ifdef GF2VR_SUBMIT_BOUNDARY
        submit_boundary.reason_mask|=8;
#endif
    }
#ifdef GF2VR_SUBMIT_BOUNDARY
    ++submit_boundary.render_calls;
    if(selected) ++submit_boundary.selected_renders;
#endif
    if(selected && !stereo_submit_scope.depth) stereo_submit_scope.context=context;
    if(selected && !stereo_submit_scope.depth && stereo_output.enabled &&
       stereo_pipeline.render.injected && stereo_source_frame.serial==stereo_submit_scope.serial) {
        DrawQueueIdentity now={0};
        if(stereo_draw_order.serial!=stereo_submit_scope.serial || !stereo_draw_order.ready ||
           !draw_runtime_identity(&now) || !draw_queue_same(stereo_draw_order.entry,now))
            stereo_draw_order.ready=0;
        if(stereo_interval_enabled && stereo_draw_order.ready && stereo_interval_entry())
            (void)eye_interval_issue_begin(&stereo_interval,
                (void (*)(void (*)(int),int))(unity+0x94d840),stereo_interval_callback);
    }
#ifdef GF2VR_DRAW_ORDER_OBSERVE
    if(selected && !stereo_submit_scope.depth) draw_order_before(stereo_submit_scope.serial);
#endif
#ifdef GF2VR_PRODUCER_SCOPE_OBSERVE
    if(dual && dual_begin==EFO_OK && !stereo_submit_scope.depth)producer_scope_arm(context);
    /* Match the product's selected-batch ambiguity above. Independent nested
       originals do not invalidate this private batch's scalar correlation. */
    else if(stereo_submit_scope.depth && selected && producer_scope.active)
        psn_fail(&producer_scope.native,producer_scope.native.token.serial);
#endif
    ++stereo_submit_scope.depth;
#ifdef GF2VR_COMPOSITION_OBSERVE
    if(stereo_submit_scope.depth==1 && composition_request_serial) {
        U64 trigger=(U64)(!selected)|
            ((U64)(composition_request_serial==stereo_pipeline.render.serial)<<1)|
            ((U64)(pipeline==stereo_source.pipeline)<<2)|
            ((U64)(method==stereo_source.method)<<3)|
            ((U64)(!stereo_pipeline.render.injected)<<4)|
            ((U64)(!composition_diag.attempted)<<5);
        composition_diag_stage(0,trigger);
    }
    if(!selected && stereo_submit_scope.depth==1 &&
       composition_request_serial==stereo_pipeline.render.serial &&
       pipeline==stereo_source.pipeline && method==stereo_source.method &&
       !stereo_pipeline.render.injected &&
       !composition_diag.attempted)
        (void)composition_diag_begin(stereo_pipeline.render.serial,cameras,context);
#endif
    SetLastError(stereo_forward_error);
#ifdef GF2VR_ROOT_INTERVAL_TIMING_OBSERVE
    RootIntervalTag timing_tag=(dual && selected && stereo_submit_scope.depth==1 &&
        dual_begin==EFO_OK)?rit_main_tag():(RootIntervalTag){0};
    rit_render_call(timing_tag,stereo_render_original,pipeline,context,cameras,method);
#else
    stereo_render_original(pipeline,context,cameras,method);
#endif
    DWORD outgoing=GetLastError();
    --stereo_submit_scope.depth;
#ifdef GF2VR_PRODUCER_BORROW_RETURN_FIX
    /* A later original Begin may have arrived inside independent depth2 work.
       Its scalar request can run only after that work returns to the still
       borrowed selected depth1 owner. Keep all original ordered guards. */
    producer_scope_after_nested(selected);
#endif
#ifdef GF2VR_COMPOSITION_OBSERVE
    if(composition_diag.serial==stereo_pipeline.render.serial &&
       composition_diag.lifetime.attempted && !stereo_submit_scope.depth)
        composition_diag_return(stereo_pipeline.render.serial,0,
            stereo_pipeline.render.active && context &&
            pipeline==stereo_source.pipeline && method==stereo_source.method);
#endif
    if(selected && !stereo_submit_scope.depth) {
        if(stereo_submit_scope.matches && !stereo_submit_scope.ambiguous)
            stereo_submit_scope.returned_serial=stereo_submit_scope.serial;
        if(stereo_draw_order.serial==stereo_submit_scope.serial) {
            DrawQueueIdentity after={0};
            if(!stereo_draw_current(stereo_submit_scope.serial,&after)) stereo_draw_order.ready=0;
        }
#ifdef GF2VR_DRAW_ORDER_OBSERVE
        draw_order_after(stereo_submit_scope.serial,stereo_submit_scope.returned_serial==stereo_submit_scope.serial);
#endif
        stereo_submit_scope.context=0;
    }
    SetLastError(outgoing);
}
static void stereo_render_forward(void *pipeline,void *context,void *cameras,void *method) {
    DWORD incoming=GetLastError();
    DWORD thread=GetCurrentThreadId();
    if(thread!=stereo_pipeline.owner_thread) {
#ifdef GF2VR_PRODUCER_SCOPE_OBSERVE
#ifdef GF2VR_PRODUCER_FIRST_FAULT_OBSERVE
        producer_scope_foreign_trace(__FILE__,__LINE__);
#else
        producer_scope_foreign();
#endif
#endif
        SetLastError(incoming);stereo_render_original(pipeline,context,cameras,method);return;
    }
    /* The check/array preparation may alter LastError; preserve it at the
       trampoline boundary, not merely on entry to this wrapper. */
#ifdef GF2VR_ACTIVITY_PRIVATE_BATCH
    if(stereo_pipeline.render.active && stereo_source.enabled && stereo_source.generation &&
       pipeline==stereo_source.pipeline && method==stereo_source.method &&
       stereo_source.generation->owner_thread==thread &&
       stereo_source.generation->publication==&stereo_publication_generation &&
       stereo_source.generation->data==stereo_source.data && stereo_source.generation->anchor==stereo_source.anchor) {
        U64 root_serial=stereo_pipeline.render.serial;
        if(activity_private_batch_only(&stereo_private_batch,stereo_source.generation->api,cameras) &&
           stereo_pipeline.render.active && stereo_pipeline.render.serial==root_serial &&
           stereo_source.enabled && pipeline==stereo_source.pipeline && method==stereo_source.method &&
           stereo_source.generation && stereo_source.generation->owner_thread==thread &&
           stereo_source.generation->publication==&stereo_publication_generation &&
           stereo_source.generation->data==stereo_source.data && stereo_source.generation->anchor==stereo_source.anchor) {
            stereo_private_batch.private_seen=1;
            /* Forward the exact independent engine batch once. It cannot
               capture/admit/revoke a world, borrow or inject a stereo pair. */
            SetLastError(incoming);stereo_render_original(pipeline,context,cameras,method);return;
        }
    }
#ifdef GF2VR_ACTIVITY_PRIVATE_ROOT_SCOPE
    /* Only selected-source work can make a proved private root mixed.
       Unselected calls still follow ordinary capture/forwarding below. */
    if(stereo_pipeline.render.active && pipeline==stereo_source.pipeline && method==stereo_source.method)
        stereo_private_batch.other_seen=1;
#else
    if(stereo_pipeline.render.active)stereo_private_batch.other_seen=1;
#endif
#endif
    DWORD previous=stereo_forward_error;stereo_forward_error=incoming;
    stereo_start_capture(pipeline,method);
    /* Observe selected inputs even while the previous eye pair awaits drain.
       Generation changes invalidate output before attempting any new eyes. */
    if(stereo_source.generation && stereo_source.enabled &&
       pipeline==stereo_source.pipeline && method==stereo_source.method &&
       stereo_pipeline.render.active) {
        SourceGeneration *g=stereo_source.generation;
        stereo_source_observed=1;
        if(g->publication!=&stereo_publication_generation || g->data!=stereo_source.data ||
           g->anchor!=stereo_source.anchor || g->owner_thread!=thread) {
            __atomic_store_n(&stereo_publication_generation,0,__ATOMIC_RELEASE);
            if(stereo_source.manual)main_test_revoke(stereo_source.manual);
            if(g->owner_thread==thread) source_generation_invalidate(g);
        } else {
            if(stereo_source.topology) {
                if(stereo_source.manual) {
#ifdef GF2VR_ACTIVITY_STACK
                    if(stereo_live && stereo_live->ready && g==&stereo_live->generation &&
                       stereo_source.topology==&stereo_live->topology) {
                        stereo_source.topology->attribute=activity_source_attribute;
                        stereo_source.topology->context=&stereo_live->activity;
                    }else
#endif
                    {
                    stereo_source.topology->attribute=main_test_attribute;
                    stereo_source.topology->context=stereo_source.manual;
                    }
                }
                g->observe=source_topology_observe;g->observe_context=stereo_source.topology;
            }
            unsigned requested=stereo_source.manual && stereo_source.manual->state==MTG_REQUESTED;
#ifdef GF2VR_COMPOSITION_OBSERVE
            if(requested && !composition_diag.attempted)
                composition_request_serial=stereo_pipeline.render.serial;
#endif
#ifdef GF2VR_ADMISSION_OBSERVE
            admission_stage=0;admission_active=requested;
#ifdef GF2VR_MAIN_PHASE_OBSERVE
            admission_active=stereo_source.manual && stereo_source.manual->state!=MTG_OFF;
            unsigned phase_observed=admission_active;
#endif
#endif
            int admitted=0;
            if(stereo_source.manual && stereo_source.manual->flat_tracking &&
               stereo_source.manual->state==MTG_OFF){
                /* Retain only revalidated numeric tracking while flat. Never
                   create a generation/eye producer from this observation. */
                SourceGenerationCandidate observed={0};
                SourceTopology *topology=stereo_source.topology;
                if(topology)topology->attribute=main_test_track;
#ifdef GF2VR_ACTIVITY_STACK
                if(topology)topology->context=stereo_source.manual; /* read-only tracking is never a group grant */
#endif
                if(!stereo_source.manual->tracking_focused || !topology ||
                   !source_topology_observe(topology,cameras,&observed))
                    main_test_revoke(stereo_source.manual);
                source_generation_invalidate(g);
            }else {
#ifdef GF2VR_MULTICAMERA_OBSERVE
                multicamera_admission_stage=0;
#endif
                admitted=source_generation_update(g,thread,cameras);
#ifdef GF2VR_ACTIVITY_UI_OBSERVE
                if(admitted)activity_ui_observe(stereo_live,cameras,stereo_pipeline.render.serial);
#endif
#ifdef GF2VR_MULTICAMERA_OBSERVE
                multicamera_source_record(g,stereo_source.manual,stereo_pipeline.render.serial,admitted);
#endif
            }
#ifdef GF2VR_ADMISSION_OBSERVE
            admission_active=0;
#ifdef GF2VR_MAIN_PHASE_OBSERVE
            static unsigned phase_rejects;
            if(phase_observed && !admitted && phase_rejects++<24) {
                event("main_phase_reject_root",stereo_pipeline.render.serial,thread);
                event("main_phase_reject_stage",((U64)admission_stage<<32)|
                    ((U64)stereo_source.manual->state<<8),thread);
            }
#endif
#ifdef GF2VR_ACTIVITY_ADMISSION_OBSERVE
            static unsigned activity_stage_records,activity_stage_valid;
            static U64 activity_stage_last;
            U64 activity_stage=((U64)admission_stage<<32)|(U64)(admitted!=0)|
                ((U64)stereo_source.manual->state<<8);
            if(requested && activity_stage_records<24 &&
               (!activity_stage_valid || activity_stage_last!=activity_stage)) {
                activity_stage_valid=1;activity_stage_last=activity_stage;++activity_stage_records;
                event("source_admission_root",stereo_pipeline.render.serial,thread);
                event("source_admission_boundary",activity_stage,thread);
            }
#else
            if(requested) event("source_admission_boundary",((U64)admission_stage<<32)|
                (U64)(admitted!=0)|((U64)stereo_source.manual->state<<8),thread);
#endif
#endif
#ifdef GF2VR_MAIN_STEREO
            if(requested) event("main_f8_source_admission",(U64)(admitted!=0)|
                ((U64)stereo_source.manual->state<<8),thread);
#else
            (void)requested;(void)admitted;
#endif
        }
        if(stereo_source.manual && !g->selection.eligible &&
           !(stereo_source.manual->flat_tracking && stereo_source.manual->state==MTG_OFF) &&
           !(stereo_source.manual->handoff_enabled && g->replacement_pending))
            main_test_revoke(stereo_source.manual);
        stereo_source.selection=g->owner_thread==thread?&g->selection:0;
    }
#ifdef GF2VR_PREPARE_OBSERVE
    unsigned observe_prepare=!prepare_active && prepare_samples<16 && stereo_source.manual &&
        stereo_source.manual->state==MTG_ACTIVE && stereo_source.enabled &&
        pipeline==stereo_source.pipeline && method==stereo_source.method && stereo_pipeline.render.active;
    if(observe_prepare) {prepare_active=1;prepare_stage=900;}
#endif
    if(stereo_live && stereo_live->pair_count==EFO_CAPACITY && stereo_source.enabled &&
       !(stereo_source.manual && stereo_source.manual->flat_tracking && stereo_source.manual->state==MTG_OFF) &&
       pipeline==stereo_source.pipeline && method==stereo_source.method &&
       stereo_pipeline.render.active && !stereo_prepare_attempted) (void)stereo_two_arm();
    if(!(stereo_live && stereo_live->pair_count==EFO_CAPACITY) && stereo_source.enabled &&
       !(stereo_source.manual && stereo_source.manual->flat_tracking && stereo_source.manual->state==MTG_OFF) &&
       pipeline==stereo_source.pipeline && method==stereo_source.method &&
       stereo_pipeline.render.active && !stereo_prepare_attempted && !stereo_pipeline.armed &&
       !stereo_pipeline.render.attempted && !stereo_source_frame.serial) {
        /* Set before any managed preparation; reentrant wrappers forward only. */
        stereo_prepare_attempted=1;
        GF2VR_PREPARE_STEP(1);
        int prepared=!stereo_source.preparation ||
            (stereo_source.preparation->source==stereo_source.generation &&
             stereo_source.generation && stereo_source.generation->data==stereo_source.data &&
             source_eye_prepare(stereo_source.preparation,thread,stereo_source.eye_width,stereo_source.eye_height));
        int armed=prepared && source_render_frame_arm(&stereo_source_frame,stereo_source.data,stereo_source.selection,
            stereo_source.reader,stereo_source.anchor,stereo_source.math,stereo_source.mailbox,
            &stereo_pipeline,thread,stereo_source.batch,pipeline,method);
        /* Arm now has its own failure markers; keep the first rejected
           operation through cleanup, including nested refresh checks. */
        if(armed) GF2VR_PREPARE_STEP(801);
        if(armed) stereo_frame_configure(&stereo_source_frame);
        if(stereo_source.manual && stereo_source.generation && !stereo_source.generation->selection.eligible &&
           !(stereo_source.manual->handoff_enabled && stereo_source.generation->replacement_pending))
            main_test_revoke(stereo_source.manual);
    }
#ifdef GF2VR_PREPARE_OBSERVE
    if(observe_prepare) {
        prepare_active=0;
        U64 code=((U64)prepare_stage<<32)|
            ((U64)stereo_source.manual->state<<8)|
            (U64)(stereo_source.generation && stereo_source.generation->selection.eligible)|
            ((U64)(stereo_source_frame.serial!=0)<<1)|((U64)stereo_pipeline.armed<<2);
        /* Repeated initial unready frames must not consume all observations
           before a later, different boundary can be seen. */
        if(!prepare_last_valid || prepare_last_code!=code) {
            prepare_last_valid=1;prepare_last_code=code;++prepare_samples;
            event("source_prepare_root",stereo_pipeline.render.serial,thread);
            event("source_prepare_boundary",code,thread);
        }
    }
#endif
    if(stereo_live && stereo_live->pair_count==EFO_CAPACITY && stereo_two.root_slot<EFO_CAPACITY) {
        void *prior=stereo_two.original_cameras;stereo_two.original_cameras=cameras;
        source_render_flight_forward(&stereo_live->flights.slots[stereo_two.root_slot],thread,
            stereo_forward_preserved,pipeline,context,cameras,method);
        stereo_two.original_cameras=prior;
    } else camera_pipeline_forward(&stereo_pipeline,thread,stereo_forward_preserved,
                                  pipeline,context,cameras,method);
    DWORD outgoing=GetLastError();stereo_forward_error=previous;SetLastError(outgoing);
}
#if defined(BOUNDARY_TEST) && defined(GF2VR_MAIN_PHASE_OBSERVE)
__declspec(dllexport) int main_phase_observation_test(void) {
    U64 saved_serial=stereo_pipeline.render.serial;
    SourceTopologyEntry e[2]={0};e[0].identity.source=(void *)1;e[0].identity.instance_id=7;
    e[0].width=1920;e[0].height=1080;e[0].mask=-1;
    e[1]=e[0];e[1].identity.source=(void *)2;e[1].identity.instance_id=8;e[1].mask=0x20;
    stereo_pipeline.render.serial=0xface;phase_records=phase_last_valid=0;
    for(unsigned i=0;i<350;++i) {
        SetLastError(941);main_phase_policy(e,2,MTG_ACTIVE,0);
        if(GetLastError()!=941 || phase_records!=1) return __LINE__;
    }
    for(unsigned i=1;i<40;++i) {
        e[0].width=1920+(int)i;SetLastError(941);main_phase_policy(e,2,MTG_ACTIVE,i%5);
        if(GetLastError()!=941 || phase_records!=(i<24?i+1:24)) return __LINE__;
    }
    if(e[0].identity.source!=(void *)1 || e[1].mask!=0x20) return __LINE__;
    stereo_pipeline.render.serial=saved_serial;return 0;
}
#endif
#if defined(BOUNDARY_TEST) && defined(GF2VR_REENTRY_OBSERVE)
__declspec(dllexport) int reentry_fault_observation_test(void) {
    unsigned previous=stereo_pipeline.owner_thread;
    stereo_pipeline.owner_thread=GetCurrentThreadId();
    stereo_interval.state=ERI_PENDING;
    SetLastError(731);stereo_output_owner_tick();
    if(GetLastError()!=731 || stereo_interval.state!=ERI_PENDING) return __LINE__;
    stereo_interval.ticket=(EyeGpuTicket){901,13,2};
    stereo_interval.fault_reason=ERIF_ABANDONED;
    stereo_interval.begin_issued=stereo_interval.abandoned=1;
    stereo_interval.state=ERI_FAULT;
    for(unsigned i=0;i<3;++i) {
        SetLastError(731);stereo_output_owner_tick();
        if(GetLastError()!=731 || stereo_interval.state!=ERI_FAULT ||
           stereo_interval.ticket.serial!=901 || !stereo_interval.abandoned) return __LINE__;
    }
    stereo_interval=(EyeRenderInterval){0};stereo_pipeline.owner_thread=previous;
    return 0;
}
#endif
#ifdef GF2VR_RETIRE_OBSERVE
#include "retirement_observe.h"
#endif
#ifdef GF2VR_TOUCH_PRIVATE
#include "touch_p3_runtime.h"
#endif
static void stereo_local_tick(U64 n) {
#line 3644
#ifdef GF2VR_ACTIVITY_CANVAS_SCOPE
    primary_ui_canvas_owner_tick();
#endif
#ifdef GF2VR_PRODUCER_SCOPE_OBSERVE
    producer_scope_tick();
#endif
#ifdef GF2VR_COMPOSITION_OBSERVE
    composition_diag_owner_tick();
#endif
#ifdef GF2VR_ACTIVITY_UI_DRAW_OBSERVE
    ui_draw_owner_tick();
#endif
    stereo_start_tick();
#if defined(GF2VR_MAIN_STEREO) && !defined(BOUNDARY_TEST)
    /* Configuration does not grant a scene: the approved main-only F8 policy
       still owns source eligibility and never reseeds the HMD reference. */
    unsigned configured=stereo_hooks.ready && stereo_live && stereo_live->ready &&
        stereo_start.requested && !stereo_start.failed && stereo_interval_enabled &&
        __atomic_load_n(&panel_device_ready,__ATOMIC_ACQUIRE) &&
        !__atomic_load_n(&panel_device_changed,__ATOMIC_ACQUIRE);
    stereo_source.enabled=configured;stereo_output.enabled=configured;
#endif
    stereo_manual_tick();
    stereo_two_owner_tick();
    stereo_output_owner_tick();
    if(stereo_source.generation && GetCurrentThreadId()==stereo_source.generation->owner_thread) {
        if(!stereo_source.enabled) source_generation_invalidate(stereo_source.generation);
        (void)source_generation_collect(stereo_source.generation,GetCurrentThreadId());
    }
#ifdef GF2VR_RETIRE_OBSERVE
    retirement_observe_flush();
#endif
#ifdef GF2VR_MAIN_STEREO
    static U64 status_next;
    if(GetTickCount64()>=status_next) {
        status_next=GetTickCount64()+1000;
        U64 flags=(stereo_hooks.ready?1u:0u)|(stereo_live?2u:0u)|(stereo_start.failed?4u:0u)|
            (stereo_source.enabled?8u:0u)|(stereo_output.enabled?16u:0u)|
            ((U64)(stereo_source.manual?stereo_source.manual->state:0)<<8)|
            ((U64)__atomic_load_n(&stereo_interval.state,__ATOMIC_ACQUIRE)<<16)|
            ((U64)__atomic_load_n(&stereo_xr_config.state,__ATOMIC_ACQUIRE)<<24);
        event("main_stereo_state",flags,GetCurrentThreadId());
    }
#endif
    if(stereo_hooks_permitted) {
        render_main_tick(n);
        if(render_installed && !stereo_hooks.attempted &&
           GetCurrentThreadId()==__atomic_load_n(&first_tid,__ATOMIC_ACQUIRE)) {
            DWORD saved=GetLastError();void *target=0,*submit=0;
            void *method=stereo_hooks_method(&target,&submit);
            stereo_pipeline.owner_thread=GetCurrentThreadId();
            if(method && stereo_hooks_install(&stereo_hooks,target,submit,stereo_render_forward,stereo_submit_forward,
                (void **)&stereo_render_original,(void **)&stereo_submit_original)) {
                stereo_start.method=method;
#ifndef GF2VR_SUBMISSION_OBSERVE
                stereo_start.requested=1;
#endif
            } else stereo_hooks.attempted=1;
            event(stereo_hooks.ready?"stereo_hooks_ready":"stereo_hooks_unavailable",stereo_hooks.rollback_pending,GetCurrentThreadId());
#ifdef GF2VR_FOCUS_CORRECT
            if(stereo_hooks.ready) event("focus_projection_hook",focus_projection_install(),GetCurrentThreadId());
#endif
            SetLastError(saved);
        }
    } else panel_main_tick(n);
    (void)render_main_tick;(void)stereo_render_forward;
    (void)stereo_submit_forward;
    (void)camera_pipeline_arm;(void)camera_render_forward;
    (void)xr_eye_mailbox_read;(void)xr_eye_mailbox_current;
    (void)managed_eyes_optics;(void)managed_eyes_create;(void)managed_eyes_targets;
    (void)managed_eyes_borrow;(void)managed_eyes_return;
    (void)camera_data_settings_bind;(void)camera_data_references_bind;
    (void)eye_binding_bind;(void)eye_binding_release;
    (void)eye_native_bind;(void)eye_native_read;
    (void)camera_data_component_bind;(void)camera_data_component_release;
    (void)camera_data_owner_prepare;(void)source_render_frame_gpu_drained;
    (void)source_pose_bind;(void)source_anchor_retire;(void)pose_math_open;(void)pose_math_close;
    (void)source_topology_bind;
    (void)source_live_open;(void)source_live_open_two;(void)source_live_prepare_slot;
    (void)source_live_arm_flight;(void)source_live_release_flight;
    (void)source_render_flight_forward;(void)source_render_flight_finish;
    (void)stereo_live_attach;(void)stereo_live_detach;
    (void)xr_eye_size_read;
}
#ifdef BOUNDARY_TEST
#ifdef GF2VR_BANK_FENCE_OBSERVE
__declspec(dllexport) int bank_fence_observe_drain_test(void) {
    DWORD saved=GetLastError();
    BankFenceObservation r={0};r.serial=12345;r.generation=7;r.epoch=9;
    r.qpc=1;r.thread=GetCurrentThreadId();r.phase=1;r.tried=15;
    r.hr[3]=0x80004005;bank_fence_observe_record(&r);
    r.phase=3;r.value=1;bank_fence_observe_record(&r);
    for(unsigned i=0;i<BFO_CAPACITY/8;++i)stereo_bank_fence_observe_tick();
    SetLastError(saved);return 0;
}
#ifdef GF2VR_BANK_DISPATCH_OBSERVE
__declspec(dllexport) int bank_dispatch_observe_drain_test(void) {
    DWORD saved=GetLastError();SetLastError(731);
    for(unsigned phase=11;phase<=17;++phase)
        bank_dispatch_observe_record(12346,7,9,phase,0x100000003ULL);
    BankFenceObservation wide={0};wide.serial=12347;wide.generation=0x100000007ULL;
    wide.epoch=0x200000009ULL;wide.qpc=0x100000011ULL;wide.value=0x100000003ULL;
    wide.phase=11;wide.thread=GetCurrentThreadId();bank_fence_observe_record(&wide);
    int ok=GetLastError()==731;
    for(unsigned i=0;i<BFO_CAPACITY/8;++i)stereo_bank_fence_observe_tick();
    SetLastError(saved);return ok?0:__LINE__;
}
#endif
#ifdef GF2VR_BANK_ADMISSION_OBSERVE
#ifdef GF2VR_BANK_CYCLE_OBSERVE
__declspec(dllexport) int bank_cycle_observe_drain_test(void) {
    DWORD saved=GetLastError();SetLastError(731);
    /* This fixture links the same production backend object as version.dll.
       Arm/warm the actual filter, then serialize only its selected phases. */
    bank_admission_observe_record(12348,0x100000007ULL,0x200000009ULL,
        31,0x100000003ULL,0x303,1);
    Sleep(2000); /* BOUNDARY_TEST only; never compiled into version.dll. */
    const unsigned phases[]={21,22,23,24,28,29,30,31
#ifdef GF2VR_BANK_CPU_OBSERVE
        ,40,41
#endif
    };
    for(unsigned i=0;i<sizeof(phases)/sizeof(phases[0]);++i)
        bank_admission_observe_record(12348,0x100000007ULL,0x200000009ULL,
            phases[i],0x100000003ULL,0x303,1);
#ifdef GF2VR_BANK_NATIVE_OBSERVE
    for(unsigned i=0;i<4;++i)
        bank_native_observe_record(12348,0x100000007ULL,0x200000009ULL,
            0x100000003ULL,42+i,0x100000021ULL+i,7,3);
#endif
#ifdef GF2VR_BANK_COPY_DELIVERY_OBSERVE
    for(unsigned i=0;i<4;++i)
        bank_native_observe_record(12348,0x100000007ULL,0x200000009ULL,
            0x100000003ULL,46+i,0x100000031ULL+i,9,5);
    for(unsigned i=11;i<=17;++i)
        bank_dispatch_observe_record(12348,0x100000007ULL,0x200000009ULL,i,0x100000003ULL);
#endif
    int ok=GetLastError()==731;
    for(unsigned i=0;i<BFO_CAPACITY/8;++i)stereo_bank_fence_observe_tick();
    SetLastError(saved);return ok?0:__LINE__;
}
#endif
__declspec(dllexport) int bank_admission_observe_drain_test(void) {
    DWORD saved=GetLastError();SetLastError(731);
    for(unsigned phase=21;phase<=BAO_LAST_PHASE;++phase)
        bank_admission_observe_record(12348,0x100000007ULL,0x200000009ULL,
            phase,0x100000003ULL,0x303,-1);
    int ok=GetLastError()==731;
    for(unsigned i=0;i<BFO_CAPACITY/8;++i)stereo_bank_fence_observe_tick();
    SetLastError(saved);return ok?0:__LINE__;
}
#endif
#endif
#define GF2VR_EYE_BRIDGE_TEST 1
#include "../panel/panel_tests.h"
#include "render_pipeline_tests.h"
#include "source_generation_tests.h"
#include "source_topology_tests.h"
#include "main_test_grant_tests.h"
#if defined(GF2VR_MAIN_PHASE_ROLES) || defined(GF2VR_ACTIVITY_STACK_OBSERVE)
#include "main_phase_roles_tests.h"
#endif
#include "source_live_owner_tests.h"
#include "two_flight_pipeline_tests.h"
#include "cpu_return_retry_tests.h"
#ifdef GF2VR_NATIVE_BOUNDARY_OBSERVE
#include "native_boundary_observe_tests.h"
#ifdef GF2VR_NATIVE_TARGET_CACHE
#include "eye_native_cache_tests.h"
#ifdef GF2VR_ROOT_INTERVAL_TIMING_OBSERVE
#include "root_interval_observe_tests.h"
#ifdef GF2VR_HELD_FRESH_CLOCK
#include "held_fresh_clock_tests.h"
#ifdef GF2VR_SOURCE_RETIRE_OBSERVE
#include "source_retire_observe_tests.h"
#ifdef GF2VR_ACTIVITY_STACK
#include "activity_target_retire_tests.h"
#endif
#endif
#endif
#endif
#endif
#endif
#include "xr_eye_output_tests.h"
#include "xr_eye_flight_images_tests.h"
#include "xr_eye_frame_driver_tests.h"
#include "xr_eye_session_stop_tests.h"
#include "xr_eye_gpu_integration_tests.h"
#ifdef GF2VR_MAIN_TRANSITION_HOLD
#include "transition_hold_tests.h"
#ifdef GF2VR_PRESENTATION_CACHE
#include "completed_presentation_tests.h"
#endif
#endif
#include "aux_panel_tests.h"
#include "world_drag_runtime_tests.h"
#include "controller_runtime_tests.h"
#include "xr_controller_composition_tests.h"
#include "controller_shared_actions_tests.h"
#ifdef GF2VR_THUMBSTICK_NAVIGATION
#include "navigation_integration_tests.h"
#endif
#ifdef GF2VR_BANK_PRESENTATION
#include "bank_presentation_runtime_tests.h"
#ifdef GF2VR_AUTO_STEREO
#include "auto_stereo_tests.h"
#endif
#include "bank_presentation_gpu_tests.h"
#endif
#include "stereo_hooks_tests.h"
#ifdef GF2VR_PRODUCER_SCOPE_OBSERVE
#include "producer_scope_observe_tests.h"
#ifdef GF2VR_PRODUCER_PREPARE_OWNERSHIP
#include "producer_prepare_tests.h"
#ifdef GF2VR_PRODUCER_ENTRY_GUARD
#include "producer_entry_guard_tests.h"
#ifdef GF2VR_PRODUCER_FIRST_FAULT_OBSERVE
#include "producer_first_fault_tests.h"
#include "producer_nested_selection_tests.h"
#ifdef GF2VR_PRODUCER_TAIL_ORDER_OBSERVE
#include "producer_tail_order_tests.h"
#ifdef GF2VR_PRODUCER_BORROW_RETURN_TEST
#include "producer_borrow_return_tests.h"
#endif
#ifdef GF2VR_PRODUCER_TRANSFER_TEST
#include "producer_transfer_tests.h"
#endif
#endif
#endif
#endif
#endif
#endif
#ifdef GF2VR_BANK_END_OBSERVE
#include "end_boundary_observe_tests.h"
#endif
#endif
