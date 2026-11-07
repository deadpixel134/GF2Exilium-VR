#ifndef GF2VR_LIVE_SETTINGS_GPU_H
#define GF2VR_LIVE_SETTINGS_GPU_H
#include "live_settings_wire.h"
/* XR publishes only an acquired, waited destination and immutable READING slot.
   This existing selected Present owner performs every immediate-context call.
   PC_FAULT retains the query/texture/source until proven drained; never timeout-free. */
static struct {
    unsigned state,format,slot;void *destination,*query;LiveSettingsSurface *surface;
} live_settings_gpu;
#ifdef GF2VR_LIVE_SETTINGS_ENTRY_OBSERVE
static U64 live_settings_entry_gpu_descriptor,live_settings_entry_gpu_format,live_settings_entry_gpu_result;
#endif
static void live_settings_gpu_render(void){
    unsigned state=__atomic_load_n(&live_settings_gpu.state,__ATOMIC_ACQUIRE);
    if(state==PC_INFLIGHT){
        BOOL done=0;
        long hr=((long (*)(void *,void *,void *,unsigned,unsigned))panel_vt(panel_context)[29])
            (panel_context,live_settings_gpu.query,&done,sizeof(done),1);
        if(hr==0&&done){panel_release(live_settings_gpu.query);live_settings_gpu.query=0;
            __atomic_store_n(&live_settings_gpu.state,PC_DONE,__ATOMIC_RELEASE);}
        else if(hr<0){
#ifdef GF2VR_LIVE_SETTINGS_ENTRY_OBSERVE
            __atomic_store_n(&live_settings_entry_gpu_result,((U64)(unsigned)hr<<32)|3u,__ATOMIC_RELEASE);
#endif
            __atomic_store_n(&live_settings_gpu.state,PC_FAULT,__ATOMIC_RELEASE);
        }
    }
    unsigned expected=PC_REQUEST;
    if(!__atomic_compare_exchange_n(&live_settings_gpu.state,&expected,PC_COPYING,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE))return;
    PanelTextureDesc td={0};void *device=0;void *destination=live_settings_gpu.destination;
    if(!destination||!live_settings_gpu.surface||live_settings_gpu.slot>1)goto skip;
    ((void (*)(void *,PanelTextureDesc *))panel_vt(destination)[10])(destination,&td);
    ((void (*)(void *,void **))panel_vt(destination)[3])(destination,&device);
    int same_device=panel_same_device(device,panel_device);
    int valid=same_device&&td.width==LSS_WIDTH&&td.height==LSS_HEIGHT&&td.mips==1&&td.array==1&&td.samples==1&&
        panel_destination_format(td.format,panel_format(live_settings_gpu.format));panel_release(device);
#ifdef GF2VR_LIVE_SETTINGS_ENTRY_OBSERVE
    unsigned checks=(same_device!=0)|((td.width==LSS_WIDTH)<<1)|((td.height==LSS_HEIGHT)<<2)|
        ((td.mips==1)<<3)|((td.array==1)<<4)|((td.samples==1)<<5)|
        ((panel_destination_format(td.format,panel_format(live_settings_gpu.format))!=0)<<6);
    __atomic_store_n(&live_settings_entry_gpu_descriptor,((U64)td.width<<32)|td.height,__ATOMIC_RELEASE);
    __atomic_store_n(&live_settings_entry_gpu_format,((U64)td.format<<32)|((U64)live_settings_gpu.format<<16)|checks,__ATOMIC_RELEASE);
    __atomic_store_n(&live_settings_entry_gpu_result,1,__ATOMIC_RELEASE);
#endif
    unsigned query_desc[2]={0,0};
    if(!valid)goto skip;
    long query_hr=((long (*)(void *,const void *,void **))panel_vt(panel_device)[24])(panel_device,query_desc,&live_settings_gpu.query);
#ifdef GF2VR_LIVE_SETTINGS_ENTRY_OBSERVE
    __atomic_store_n(&live_settings_entry_gpu_result,((U64)(unsigned)query_hr<<32)|2u,__ATOMIC_RELEASE);
#endif
    if(query_hr<0||!live_settings_gpu.query)goto skip;
    const unsigned *pixels=(const unsigned *)live_settings_gpu.surface->pixels[live_settings_gpu.slot];
    static unsigned rgba[LSS_WIDTH*LSS_HEIGHT];
    if(live_settings_gpu.format==28||live_settings_gpu.format==29){
        for(unsigned i=0;i<LSS_WIDTH*LSS_HEIGHT;++i){unsigned c=pixels[i];rgba[i]=(c&0xff00ff00u)|((c&255u)<<16)|((c>>16)&255u);}pixels=rgba;
    }
    ((void (*)(void *,void *,unsigned,const void *,const void *,unsigned,unsigned))panel_vt(panel_context)[48])
        (panel_context,destination,0,0,pixels,LSS_WIDTH*4,0);
    ((void (*)(void *,void *))panel_vt(panel_context)[28])(panel_context,live_settings_gpu.query);
    __atomic_store_n(&live_settings_gpu.state,PC_INFLIGHT,__ATOMIC_RELEASE);return;
skip:
    __atomic_store_n(&live_settings_gpu.state,PC_SKIPPED,__ATOMIC_RELEASE);
}
#endif
