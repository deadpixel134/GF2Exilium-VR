#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include "draw_order_probe.h"
int draw_order_prepare(EyeEventCopy *event,int id,EyeGpuTicket ticket,unsigned thread,void *device,void *context) {
    if(!event || !device || !context || !thread ||
       __atomic_load_n(&event->state,__ATOMIC_ACQUIRE)!=EEC_IDLE) return 0;
    ID3D11Device *d=device;
    if(ID3D11Device_GetCreationFlags(d)&D3D11_CREATE_DEVICE_SINGLETHREADED)return 0;
    D3D11_TEXTURE2D_DESC desc={1,1,1,1,DXGI_FORMAT_R8G8B8A8_UNORM,{1,0},D3D11_USAGE_DEFAULT,0,0,0};
    ID3D11Texture2D *left=0,*right=0;int ok=0;
    if(SUCCEEDED(ID3D11Device_CreateTexture2D(d,&desc,0,&left)) &&
       SUCCEEDED(ID3D11Device_CreateTexture2D(d,&desc,0,&right)))
        ok=eye_event_prepare_drain(event,id,ticket,thread,1,d,context,left,right);
    if(right)ID3D11Texture2D_Release(right);
    if(left)ID3D11Texture2D_Release(left);
    return ok;
}
