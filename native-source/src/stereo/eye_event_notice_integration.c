#ifndef GF2VR_EVENT_NOTICE_HANDOFF
#error Use original integration TU when event handoff is OFF
#endif
#ifdef BOUNDARY_TEST
static unsigned event_notice_test_disabled,event_notice_test_skip_retirement_guard;
#endif
static void stereo_event_notice_service(void *);
#include "render_pipeline_integration.c"
#include "eye_event_notice.h"
static unsigned long long event_notice_render_acks,event_notice_copy_acks,event_notice_rejected,event_notice_busy;
static void stereo_event_notice_service(void *opaque){
 PanelXr *p=opaque;XrEyeFrameDriver *d=&stereo_two_xr;
 if(!p||!xfd_owner(d)||d->api.prefetch.session!=p->session)return;
#ifdef BOUNDARY_TEST
 if(event_notice_test_disabled)return;
#endif
 DWORD saved=GetLastError();static unsigned cursor,reports;static U64 next_report;
 for(unsigned n=0;n<EGN_CAPACITY;++n){unsigned slot;EyeGpuNotice notice;
  if(!eye_gpu_notice_take(&cursor,&slot,&notice))break;
  unsigned ack=0;int result=notice.valid?eye_flight_backend_notice(&stereo_two.backend,&stereo_two.owner,
      notice.ticket,notice.drain_only,d->thread,&ack):EFO_REJECTED;
  if(result==EFO_BUSY){++event_notice_busy;eye_gpu_notice_return(slot,0);continue;}
  eye_gpu_notice_return(slot,1);
  if(result!=EFO_OK)++event_notice_rejected;
  else if(ack){if(notice.drain_only)++event_notice_render_acks;else ++event_notice_copy_acks;}
 }
 U64 now=GetTickCount64();if(reports<40&&now>=next_report){next_report=now+2000;++reports;
  char line[1000],*q=source_prefix(line,"event_notice_progress");
#define EGN_WRITE(name,value) do{q=literal(q,",\"" name "\":");q=bank_dispatch_decimal(q,(U64)(value));}while(0)
  EGN_WRITE("posted",eye_gpu_notice_stat(1));EGN_WRITE("completed",eye_gpu_notice_stat(2));EGN_WRITE("capacity_fallback",eye_gpu_notice_stat(3));
  EGN_WRITE("state",eye_gpu_notice_stat(0));EGN_WRITE("wait_error",eye_gpu_notice_stat(4));
  EGN_WRITE("render_numeric_acks",event_notice_render_acks);EGN_WRITE("copy_numeric_acks",event_notice_copy_acks);
  EGN_WRITE("busy",event_notice_busy);EGN_WRITE("rejected",event_notice_rejected);
#undef EGN_WRITE
  q=literal(q,"}\n");source_log(line,q);
 }
 SetLastError(saved);
}
