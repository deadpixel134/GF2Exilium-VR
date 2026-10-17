#ifndef GF2_ACTIVITY_UI_CANVAS_MEMBERSHIP_H
#define GF2_ACTIVITY_UI_CANVAS_MEMBERSHIP_H
#if !defined(GF2VR_ACTIVITY_UI_FRAME_ELIGIBLE) || !defined(GF2VR_ACTIVITY_UI_GENERATION_LEASE)
#error Canvas membership observation requires the frozen eligibility/lease parent
#endif
/* Diagnostic only. Capture the existing public setter's synchronous native
   argument as an opaque equality token; never dereference that token, inspect
   a native field, retain a batch/job/scene resource, or invoke a native API. */
enum {ACM_CAMERAS=8};
static struct {
    void (*original_set)(void *,unsigned);
    unsigned attempted,ready,hook_stage,owner,armed,captures,pending,active,finished;
    unsigned stage,overflow;
    U64 native_token,generation,started,deadline;
    void *managed;
    struct {int id;unsigned any,entered,returned,read_fail,zero,nonzero,reported;} cameras[ACM_CAMERAS];
} acm;
typedef struct {unsigned row,enrolled,root,read_ok;unsigned short before;U64 generation;} AcmCall;
static int acm_lease(void) {
    unsigned owner=__atomic_load_n(&acm.owner,__ATOMIC_ACQUIRE);
    return owner && GetCurrentThreadId()==owner && primary_ui_canvas.scope.active &&
        !primary_ui_canvas.scope.failed && primary_ui_canvas.scope.owner==acm.owner &&
        primary_ui_canvas.canvas_root && primary_ui_canvas.camera_root && primary_ui_canvas.transform_root &&
        primary_ui_canvas.canvas==acm.managed && primary_ui_generation_context &&
        primary_ui_generation_id==acm.generation && acm.generation;
}
static void acm_report(unsigned stage) {
    char line[3600],*p=source_prefix(line,"activity_ui_canvas_membership");
    p=literal(p,",\"stage\":");p=decimal(p,stage);
    p=literal(p,",\"generation\":");p=hexadecimal(p,acm.generation);
    p=literal(p,",\"capture_count\":");p=decimal(p,acm.captures);
    p=literal(p,",\"hook_ready\":");p=decimal(p,acm.ready);
    p=literal(p,",\"hook_stage\":");p=decimal(p,acm.hook_stage);
    p=literal(p,",\"window_ms\":");U64 now=GetTickCount64();
    p=hexadecimal(p,acm.started && now>=acm.started?now-acm.started:0);
    p=literal(p,",\"overflow\":");p=decimal(p,acm.overflow);
    p=literal(p,",\"cameras\":[");unsigned rows=0;
    for(unsigned i=0;i<ACM_CAMERAS;++i)if(acm.cameras[i].id) {
        if(rows++)p=literal(p,",");p=literal(p,"{\"id\":");p=hexadecimal(p,(unsigned)acm.cameras[i].id);
        p=literal(p,",\"any_canvas\":");p=decimal(p,acm.cameras[i].any);
        p=literal(p,",\"root_entered\":");p=decimal(p,acm.cameras[i].entered);
        p=literal(p,",\"root_returned\":");p=decimal(p,acm.cameras[i].returned);
        p=literal(p,",\"counter_read_fail\":");p=decimal(p,acm.cameras[i].read_fail);
        p=literal(p,",\"counter_zero\":");p=decimal(p,acm.cameras[i].zero);
        p=literal(p,",\"counter_nonzero\":");p=decimal(p,acm.cameras[i].nonzero);p=literal(p,"}");
    }
    p=literal(p,"]}\n");source_log(line,p);
}
static void acm_close(unsigned stage) {
    if(acm.finished)return;acm.active=acm.pending=acm.armed=0;acm.finished=1;acm.stage=stage;
    /* Native token is an identity comparison only; discard on every close. */
    acm.native_token=0;acm_report(stage);
}
static void acm_set_forward(void *canvas,unsigned mode) {
    DWORD incoming=GetLastError();
    if(GetCurrentThreadId()==__atomic_load_n(&acm.owner,__ATOMIC_ACQUIRE) && acm.armed && acm_lease() && mode==2) {
        ++acm.captures;
        if(acm.captures==1)acm.native_token=(U64)canvas;
    }
    SetLastError(incoming);acm.original_set(canvas,mode);
    /* No observer calls after original: preserve its complete LastError. */
}
static int acm_hook_set(void *target) {
    static const unsigned char prefix[]={0x40,0x53,0x48,0x83,0xec,0x50,0x48,0x8b,0x81,0xa8,1,0,0};
    acm.hook_stage=1;if(!stereo_hook_bytes(target,prefix,sizeof(prefix)))return 0;
    int status=MH_Initialize();acm.hook_stage=2;if(status!=0 && status!=1)return 0;
    acm.hook_stage=3;if(MH_CreateHook(target,acm_set_forward,(void **)&acm.original_set))return 0;
    acm.hook_stage=4;if(MH_EnableHook(target)){(void)MH_DisableHook(target);return 0;}
    acm.hook_stage=0;return acm.ready=1;
}
static void acm_set_before(void *managed,unsigned mode) {
    DWORD saved=GetLastError();
    unsigned owner=__atomic_load_n(&acm.owner,__ATOMIC_ACQUIRE);
    if(owner && GetCurrentThreadId()!=owner)goto end;
    if(mode!=2) {if(acm.pending || acm.active)acm_close(1);goto end;}
    if(acm.attempted || acm.finished || !managed || primary_ui_canvas.canvas!=managed ||
       !primary_ui_canvas.scope.active || primary_ui_canvas.scope.failed ||
       primary_ui_canvas.scope.owner!=GetCurrentThreadId() || !primary_ui_generation_id ||
       !primary_ui_generation_context || !primary_ui_canvas.canvas_root ||
       !primary_ui_canvas.camera_root || !primary_ui_canvas.transform_root)goto end;
    acm.attempted=1;__atomic_store_n(&acm.owner,GetCurrentThreadId(),__ATOMIC_RELEASE);
    acm.managed=managed;acm.generation=primary_ui_generation_id;
    if(!acm.ready && (!unity || !acm_hook_set(unity+0x28d500))) {acm_close(2);goto end;}
    acm.armed=1;
end:SetLastError(saved);
}
static void acm_set_after(void *managed,unsigned mode,int ok) {
    DWORD saved=GetLastError();
    if(GetCurrentThreadId()!=__atomic_load_n(&acm.owner,__ATOMIC_ACQUIRE))goto end;
    if(acm.armed && mode==2 && GetCurrentThreadId()==acm.owner) {
        acm.armed=0;
        if(!ok || managed!=acm.managed || acm.captures!=1 || !acm.native_token || !acm_lease())acm_close(3);
        else acm.pending=1;
    }
end:SetLastError(saved);
}
static void acm_tick(void) {
    DWORD saved=GetLastError();
    if(GetCurrentThreadId()!=__atomic_load_n(&acm.owner,__ATOMIC_ACQUIRE))goto end;
    if(!acm.finished && (acm.pending || acm.active) && GetCurrentThreadId()==acm.owner) {
        if(!acm_lease())acm_close(1);
        else {
            U64 now=GetTickCount64();
            if(acm.pending){
                if(now>~0ULL-10000)acm_close(4);
                else {acm.pending=0;acm.active=1;acm.started=now;acm.deadline=now+10000;}
            }
            else if(now>=acm.deadline)acm_close(0);
        }
    }
end:SetLastError(saved);
}
static int acm_counter(void *order,unsigned short *value) {
    U64 read=0;
    return order && (U64)order<=~0ULL-2 &&
        ReadProcessMemory((HANDLE)(long long)-1,order,value,2,&read) && read==2;
}
static AcmCall acm_register_begin(unsigned enrolled,U64 generation,void *canvas,void *order) {
    AcmCall call={ACM_CAMERAS,UICR_CAPACITY,0,0,0,0};
    if(enrolled>=UICR_CAPACITY || GetCurrentThreadId()!=uicr_thread())return call;
    if(!acm.active || generation!=acm.generation || !acm_lease() ||
       GetTickCount64()>=acm.deadline || !uicr.rows[enrolled].root || uicr.rows[enrolled].retired ||
       uicr.rows[enrolled].generation!=generation)return call;
    int id=uicr.rows[enrolled].instance;unsigned row=ACM_CAMERAS;
    for(unsigned i=0;i<ACM_CAMERAS;++i)if(acm.cameras[i].id==id){row=i;break;}
    if(row==ACM_CAMERAS)for(unsigned i=0;i<ACM_CAMERAS;++i)if(!acm.cameras[i].id){row=i;acm.cameras[i].id=id;break;}
    if(row==ACM_CAMERAS){++acm.overflow;return call;}
    ++acm.cameras[row].any;call.row=row;call.enrolled=enrolled;call.generation=generation;
    if((U64)canvas==acm.native_token) {
        call.root=1;++acm.cameras[row].entered;call.read_ok=acm_counter(order,&call.before);
    }
    return call;
}
static void acm_register_end(const AcmCall *call,void *order) {
    if(!call->root || call->row>=ACM_CAMERAS || call->enrolled>=UICR_CAPACITY || !acm.active ||
       call->generation!=acm.generation || !acm_lease() || !uicr.rows[call->enrolled].root ||
       uicr.rows[call->enrolled].retired || uicr.rows[call->enrolled].generation!=call->generation ||
       uicr.rows[call->enrolled].instance!=acm.cameras[call->row].id)return;
    unsigned row=call->row;unsigned short after=0;++acm.cameras[row].returned;
    if(!call->read_ok || !acm_counter(order,&after))++acm.cameras[row].read_fail;
    else if(after<call->before)++acm.cameras[row].read_fail; /* wrap is not a traversal inference */
    else if(after==call->before)++acm.cameras[row].zero;
    else ++acm.cameras[row].nonzero;
}
#endif
