#ifndef GF2_CAMERA_FOLLOW_HANDOFF_H
#define GF2_CAMERA_FOLLOW_HANDOFF_H
/* Same fresh press boundary as camera return. Pending parity preserves two
   clicks between producer updates; XR never owns the selected follow mode. */
typedef struct {CameraReturnHandoff edge;unsigned parity;} CameraFollowHandoff;
static void camera_follow_cancel(CameraFollowHandoff *h){
    camera_return_lock(&h->edge);h->edge.pending=h->edge.armed=0;h->edge.sample=0;h->parity=0;camera_return_unlock(&h->edge);
}
static void camera_follow_publish(CameraFollowHandoff *h,int eligible,int active,int down,
    unsigned long long epoch,unsigned long long generation,unsigned long long tick,unsigned long long sample){
    CameraReturnHandoff *e=&h->edge;camera_return_lock(e);
    if(!eligible||!active||!epoch||!generation||!tick||!sample){e->pending=e->armed=0;e->sample=0;h->parity=0;}
    else{
        if(e->epoch!=epoch||e->generation!=generation){e->pending=e->armed=0;e->sample=0;h->parity=0;e->epoch=epoch;e->generation=generation;}
        if(e->pending&&(tick<e->command.tick||tick-e->command.tick>250||
           (sample>=e->command.sample&&sample-e->command.sample>250000000ull))){e->pending=0;h->parity=0;}
        if(sample>e->sample){e->sample=sample;
            if(!down)e->armed=1;
            else if(e->armed){e->armed=0;h->parity^=1;e->pending=1;e->command=(CameraReturnCommand){++e->serial,epoch,generation,tick,sample};}
        }
    }
    camera_return_unlock(e);
}
static unsigned camera_follow_take(CameraFollowHandoff *h,unsigned long long epoch,
    unsigned long long generation,unsigned long long now,long long pose_time){
    unsigned result=0;CameraReturnHandoff *e=&h->edge;camera_return_lock(e);
    if(e->pending){CameraReturnCommand c=e->command;
        if(c.epoch!=epoch||c.generation!=generation||now<c.tick||now-c.tick>250||pose_time<=0||
           (pose_time>=(long long)c.sample&&pose_time-(long long)c.sample>250000000ll)){e->pending=0;h->parity=0;}
        else if(pose_time>=(long long)c.sample){result=h->parity;e->pending=0;h->parity=0;}
    }
    camera_return_unlock(e);return result;
}
/* Non-consuming eligibility check: source owner may refresh a frozen base for
   LEFT return; the existing pose owner still consumes and applies the reset. */
static int camera_follow_return_pending(CameraReturnHandoff *e,unsigned long long epoch,
    unsigned long long generation,unsigned long long now,long long pose_time){
    camera_return_lock(e);CameraReturnCommand c=e->command;
    int ready=e->pending&&c.epoch==epoch&&c.generation==generation&&now>=c.tick&&now-c.tick<=250&&
        pose_time>0&&pose_time>=(long long)c.sample&&pose_time-(long long)c.sample<=250000000ll;
    camera_return_unlock(e);return ready;
}
#endif
