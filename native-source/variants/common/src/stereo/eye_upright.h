#ifndef GF2_EYE_UPRIGHT_H
#define GF2_EYE_UPRIGHT_H
#include "eye_upright_vs.h"
#include "eye_upright_ps_uint.h"
#include "eye_upright_ps_unorm.h"
#ifdef GF2VR_UPRIGHT_TEST
int egc_upright_test_fail;
#endif
/* Prepare an entire pair before issuing any GPU work. A private deferred
   context owns all draw state. The returned list retains all transient resources
   until the existing pair event query drains; no global device cache/lifetime.
   Scratch storage avoids assuming SRV/RTV binds on borrowed Unity/XR textures.
   Integer views preserve raw half-float and RGBA bytes, including SRGB storage.
   BGRA uses UNORM views on both sides (no SRGB conversion or filtering). */
static ID3D11CommandList *egc_upright_prepare(ID3D11Device *d,void **textures) {
    ID3D11DeviceContext *record=0;ID3D11CommandList *list=0;
    ID3D11Texture2D *scratch[2]={0};ID3D11ShaderResourceView *srv=0;
    ID3D11RenderTargetView *rtv=0;ID3D11VertexShader *vs=0;ID3D11PixelShader *ps=0;
    ID3D11RasterizerState *raster=0;ID3D11DepthStencilState *depth=0;
    D3D11_TEXTURE2D_DESC desc;ID3D11Texture2D_GetDesc((ID3D11Texture2D *)textures[0],&desc);
    int bgra=desc.Format==DXGI_FORMAT_B8G8R8A8_UNORM || desc.Format==DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    desc.Format=bgra?DXGI_FORMAT_B8G8R8A8_UNORM:
        desc.Format==DXGI_FORMAT_R16G16B16A16_FLOAT?DXGI_FORMAT_R16G16B16A16_UINT:DXGI_FORMAT_R8G8B8A8_UINT;
    desc.MiscFlags=0;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
#define EU_OK(call) do {if(FAILED(call)) goto done;} while(0)
    EU_OK(ID3D11Device_CreateDeferredContext(d,0,&record));
    EU_OK(ID3D11Device_CreateTexture2D(d,&desc,0,&scratch[0]));
    desc.BindFlags=D3D11_BIND_RENDER_TARGET;
    EU_OK(ID3D11Device_CreateTexture2D(d,&desc,0,&scratch[1]));
    EU_OK(ID3D11Device_CreateShaderResourceView(d,(ID3D11Resource *)scratch[0],0,&srv));
    EU_OK(ID3D11Device_CreateRenderTargetView(d,(ID3D11Resource *)scratch[1],0,&rtv));
    EU_OK(ID3D11Device_CreateVertexShader(d,egc_upright_vs,sizeof(egc_upright_vs),0,&vs));
    EU_OK(ID3D11Device_CreatePixelShader(d,bgra?egc_upright_ps_unorm:egc_upright_ps_uint,
        bgra?sizeof(egc_upright_ps_unorm):sizeof(egc_upright_ps_uint),0,&ps));
    D3D11_RASTERIZER_DESC rd={0};rd.FillMode=D3D11_FILL_SOLID;rd.CullMode=D3D11_CULL_NONE;rd.DepthClipEnable=TRUE;
    EU_OK(ID3D11Device_CreateRasterizerState(d,&rd,&raster));
    D3D11_DEPTH_STENCIL_DESC dd={0};dd.DepthFunc=D3D11_COMPARISON_ALWAYS;
    EU_OK(ID3D11Device_CreateDepthStencilState(d,&dd,&depth));
    D3D11_VIEWPORT vp={0,0,(float)desc.Width,(float)desc.Height,0,1};
    ID3D11DeviceContext_RSSetViewports(record,1,&vp);
    ID3D11DeviceContext_RSSetState(record,raster);
    ID3D11DeviceContext_OMSetDepthStencilState(record,depth,0);
    ID3D11DeviceContext_IASetPrimitiveTopology(record,D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext_VSSetShader(record,vs,0,0);ID3D11DeviceContext_PSSetShader(record,ps,0,0);
    for(unsigned eye=0;eye<2;++eye) {
        ID3D11DeviceContext_CopyResource(record,(ID3D11Resource *)scratch[0],(ID3D11Resource *)textures[eye]);
        ID3D11DeviceContext_PSSetShaderResources(record,0,1,&srv);
        ID3D11DeviceContext_OMSetRenderTargets(record,1,&rtv,0);
        ID3D11DeviceContext_Draw(record,3,0);
        ID3D11ShaderResourceView *null_srv=0;
        ID3D11DeviceContext_PSSetShaderResources(record,0,1,&null_srv);
        ID3D11DeviceContext_OMSetRenderTargets(record,0,0,0);
        ID3D11DeviceContext_CopyResource(record,(ID3D11Resource *)textures[eye+2],(ID3D11Resource *)scratch[1]);
    }
#ifdef GF2VR_UPRIGHT_TEST
    if(egc_upright_test_fail) goto done;
#endif
    EU_OK(ID3D11DeviceContext_FinishCommandList(record,FALSE,&list));
done:
    egc_release(record);egc_release(scratch[0]);egc_release(scratch[1]);
    egc_release(srv);egc_release(rtv);egc_release(vs);egc_release(ps);egc_release(raster);egc_release(depth);
#undef EU_OK
    return list;
}
#endif
