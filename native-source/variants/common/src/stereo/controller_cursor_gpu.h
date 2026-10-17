#ifndef GF2_CONTROLLER_CURSOR_GPU_H
#define GF2_CONTROLLER_CURSOR_GPU_H
/* One static cursor-image upload, issued only by the selected Present owner.
   XR publishes one acquired destination and waits for event-query completion.
   No game framebuffer writes, no XR-thread immediate-context operations. */
static unsigned controller_cursor_state;
static void *controller_cursor_destination,*controller_cursor_query;
static void *controller_game_window;
static void controller_cursor_render(void *chain) {
    struct {unsigned mode[7],sample[2],usage,buffers;void *window;int windowed;unsigned effect,flags;} desc={0};
    _Static_assert(sizeof(desc)==72,"DXGI swapchain descriptor ABI");
    if(((long (*)(void *,void *))panel_vt(chain)[12])(chain,&desc)>=0)
        __atomic_store_n(&controller_game_window,desc.window,__ATOMIC_RELEASE);
    unsigned state=__atomic_load_n(&controller_cursor_state,__ATOMIC_ACQUIRE);
    if(state==PC_INFLIGHT) {
        BOOL done=0;
        long hr=((long (*)(void *,void *,void *,unsigned,unsigned))panel_vt(panel_context)[29])
            (panel_context,controller_cursor_query,&done,sizeof(done),1);
        if(hr==0&&done) {panel_release(controller_cursor_query);controller_cursor_query=0;
            __atomic_store_n(&controller_cursor_state,PC_DONE,__ATOMIC_RELEASE);}
        else if(hr<0)__atomic_store_n(&controller_cursor_state,PC_FAULT,__ATOMIC_RELEASE);
    }
    unsigned expected=PC_REQUEST;
    if(!__atomic_compare_exchange_n(&controller_cursor_state,&expected,PC_COPYING,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE))return;
    PanelTextureDesc td={0};void *device=0;
    ((void (*)(void *,PanelTextureDesc *))panel_vt(controller_cursor_destination)[10])(controller_cursor_destination,&td);
    ((void (*)(void *,void **))panel_vt(controller_cursor_destination)[3])(controller_cursor_destination,&device);
    int valid=panel_same_device(device,panel_device)&&td.width==64&&td.height==64&&td.mips==1&&td.array==1&&td.samples==1&&panel_destination_format(td.format,29);
    panel_release(device);
    unsigned query_desc[2]={0,0};
    if(!valid||((long (*)(void *,const void *,void **))panel_vt(panel_device)[24])(panel_device,query_desc,&controller_cursor_query)<0||!controller_cursor_query) {
        __atomic_store_n(&controller_cursor_state,PC_SKIPPED,__ATOMIC_RELEASE);return;
    }
    static unsigned pixels[64*64];
    for(int y=0;y<64;++y)for(int x=0;x<64;++x) {
        int dx=2*x-63,dy=2*y-63,d=dx*dx+dy*dy;
        /* Premultiplied white ring plus opaque dark outline, transparent center. */
        pixels[y*64+x]=d>=1700&&d<=3000?(d>=2000&&d<=2700?0xffffffffu:0xff181818u):0;
    }
    ((void (*)(void *,void *,unsigned,const void *,const void *,unsigned,unsigned))panel_vt(panel_context)[48])
        (panel_context,controller_cursor_destination,0,0,pixels,256,0);
    ((void (*)(void *,void *))panel_vt(panel_context)[28])(panel_context,controller_cursor_query);
    __atomic_store_n(&controller_cursor_state,PC_INFLIGHT,__ATOMIC_RELEASE);
}
#endif
