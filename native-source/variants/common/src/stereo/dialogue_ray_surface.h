#ifndef GF2_DIALOGUE_RAY_SURFACE_H
#define GF2_DIALOGUE_RAY_SURFACE_H
#include "controller_input.h"
/* Numeric sidecar only: never a camera/Canvas borrow or a completion proof.
   Main publishes applied geometry; XR selects the exact displayed bank token.
   A missed/colliding/busy record disables interaction, never rendering. */
typedef struct {
    unsigned long long generation,epoch,frame,owner,tick;
    unsigned valid,image_width,image_height;
    ControllerPanel panel; /* captured LOCAL, not current VIEW */
} DialogueRaySurface;
#define DIALOGUE_RAY_RECORDS 64u
typedef struct {unsigned lock;DialogueRaySurface records[DIALOGUE_RAY_RECORDS],latest;} DialogueRayMailbox;
static DialogueRayMailbox dialogue_ray_mailbox;
#ifdef GF2VR_ACTIVITY_UI_LIFETIME
/* Separate atomic live eligibility closes the old-record race even when the
   nonblocking numeric mailbox is contended during a UI invalidation. */
static unsigned long long dialogue_ray_live_ui;
static void dialogue_ray_invalidate(void) {
    __atomic_store_n(&dialogue_ray_live_ui,0,__ATOMIC_RELEASE);
}
#endif
#ifdef GF2VR_ACTIVITY_DIALOGUE_RAY_OBSERVE
#include "dialogue_ray_observe.h"
#endif
static int dialogue_ray_publish(DialogueRayMailbox *m,const DialogueRaySurface *s) {
    if(!m || !s || !s->frame || !s->generation || !s->epoch ||
       __atomic_exchange_n(&m->lock,1,__ATOMIC_ACQUIRE))return 0;
    m->records[s->frame%DIALOGUE_RAY_RECORDS]=*s;m->latest=*s;
    __atomic_store_n(&m->lock,0,__ATOMIC_RELEASE);return 1;
}
static int dialogue_ray_read(DialogueRayMailbox *m,unsigned long long generation,
    unsigned long long epoch,unsigned long long frame,unsigned long long now,DialogueRaySurface *out) {
    if(!m || !out || !frame || !generation || !epoch || !now ||
       __atomic_exchange_n(&m->lock,1,__ATOMIC_ACQUIRE))return 0;
    DialogueRaySurface s=m->records[frame%DIALOGUE_RAY_RECORDS],live=m->latest;
    __atomic_store_n(&m->lock,0,__ATOMIC_RELEASE);
#ifdef GF2VR_ACTIVITY_DIALOGUE_RAY_OBSERVE
    dialogue_ray_observe_read(&s,&live,generation,epoch,frame,now);
#endif
    if(!s.valid || !live.valid || s.frame!=frame || s.generation!=generation || s.epoch!=epoch ||
       live.generation!=generation || live.epoch!=epoch || live.owner!=s.owner || !s.owner ||
       !s.tick || now<s.tick || now-s.tick>=250 || live.frame<frame)return 0;
#ifdef GF2VR_ACTIVITY_UI_LIFETIME
    if(__atomic_load_n(&dialogue_ray_live_ui,__ATOMIC_ACQUIRE)!=s.owner)return 0;
#endif
    *out=s;return 1;
}
static ControllerQuaternion dialogue_ray_multiply(ControllerQuaternion a,ControllerQuaternion b) {
    return (ControllerQuaternion){a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,
        a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w,
        a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z};
}
/* Aim was sampled in the existing panel VIEW space. Transform the captured
   LOCAL surface with the actual CURRENT head pose, not the capture-frame head
   or an invented neutral. Cursor and hit test share this exact result. */
static int dialogue_ray_view(const DialogueRaySurface *s,const ControllerPose *head,
    unsigned width,unsigned height,ControllerPanel *out) {
    if(!s || !s->valid || !head || !out || !width || !height || width>32768 || height>32768 ||
       !s->image_width || !s->image_height || s->image_width>32768 || s->image_height>32768 ||
       (unsigned long long)width*s->image_height!=(unsigned long long)height*s->image_width ||
       !controller_vector_valid(head->position) || !controller_quaternion_valid(head->orientation) ||
       !controller_vector_valid(s->panel.pose.position) || !controller_quaternion_valid(s->panel.pose.orientation))return 0;
    ControllerQuaternion inverse={-head->orientation.x,-head->orientation.y,-head->orientation.z,head->orientation.w};
    ControllerVector d={s->panel.pose.position.x-head->position.x,s->panel.pose.position.y-head->position.y,
        s->panel.pose.position.z-head->position.z};
    ControllerPanel p=s->panel;p.pose.position=controller_rotate(inverse,d);
    p.pose.orientation=dialogue_ray_multiply(inverse,p.pose.orientation);
    if(!controller_quaternion_valid(p.pose.orientation))return 0;
    p.client_width=width;p.client_height=height;*out=p;return 1;
}
#endif
