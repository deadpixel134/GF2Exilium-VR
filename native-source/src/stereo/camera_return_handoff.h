#ifndef GF2_CAMERA_RETURN_HANDOFF_H
#define GF2_CAMERA_RETURN_HANDOFF_H
/* XR press latch + small shared POD command. No API, pose or resource work
   while locked. Main consumes only a current source/reference and eye time. */
typedef struct {unsigned long long serial,epoch,generation,tick,sample;} CameraReturnCommand;
typedef struct {unsigned lock,pending,armed;unsigned long long serial,epoch,generation,sample;CameraReturnCommand command;} CameraReturnHandoff;
static void camera_return_lock(CameraReturnHandoff *h){for(;;){unsigned zero=0;if(__atomic_compare_exchange_n(&h->lock,&zero,1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED))return;}}
static void camera_return_unlock(CameraReturnHandoff *h){__atomic_store_n(&h->lock,0,__ATOMIC_RELEASE);}
static void camera_return_cancel(CameraReturnHandoff *h){camera_return_lock(h);h->pending=h->armed=0;h->sample=0;camera_return_unlock(h);}
static int camera_return_publish(CameraReturnHandoff *h,int eligible,int active,int down,
    unsigned long long epoch,unsigned long long generation,unsigned long long tick,unsigned long long sample){
    if(!h)return 0;int pressed=0;camera_return_lock(h);
    if(!eligible||!active||!epoch||!generation||!tick||!sample){h->armed=0;h->pending=0;h->sample=0;}
    else{
        if(h->epoch!=epoch||h->generation!=generation){h->armed=0;h->pending=0;h->sample=0;h->epoch=epoch;h->generation=generation;}
        if(sample>h->sample){
            h->sample=sample;
            if(!down)h->armed=1;
            else if(h->armed){h->armed=0;h->command=(CameraReturnCommand){++h->serial,epoch,generation,tick,sample};h->pending=1;pressed=1;}
        }
    }
    camera_return_unlock(h);return pressed;
}
static CameraReturnCommand camera_return_take(CameraReturnHandoff *h,unsigned long long epoch,
    unsigned long long generation,unsigned long long now,long long pose_time){
    CameraReturnCommand result={0};camera_return_lock(h);
    if(h->pending){CameraReturnCommand c=h->command;
        if(c.epoch!=epoch||c.generation!=generation||now<c.tick||now-c.tick>250||pose_time<=0||
           (pose_time>=(long long)c.sample&&pose_time-(long long)c.sample>250000000ll))h->pending=0;
        else if(pose_time>=(long long)c.sample){result=c;h->pending=0;}
    }
    camera_return_unlock(h);return result;
}
#endif
