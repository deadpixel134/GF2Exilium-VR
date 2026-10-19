#ifndef GF2_FOCUS_GPU_OBSERVE_H
#define GF2_FOCUS_GPU_OBSERVE_H
#include "../bootstrap/runtime_logging_state.h"
#include <d3d11_1.h>
static const GUID focus_context1_iid={0xbb2c6faa,0xb5fb,0x4082,{0x8e,0x6b,0x38,0x8b,0x8c,0xfa,0x90,0xe1}};
/* Diagnostic render-owner state. Bound buffers are not proof that the shader
   consumes a particular field. One interval, at most four draws per eye; the
   normal interval query covers every copy before watch release. */
typedef struct {
    ID3D11Buffer *buffers[14];
    unsigned sizes[14],captured[14],first[14],constants[14],create_status[14],eye,draw,viewport_count,ranges;
    D3D11_VIEWPORT viewport;
    unsigned long long shader;
} FocusGpuDraw;
static struct {
    EyeTargetWatch *owner;
    EyeGpuTicket ticket;
    unsigned count[2],total,done;
    FocusGpuDraw draws[8];
} focus_gpu;
static void focus_gpu_draw(EyeTargetWatch *w,ID3D11DeviceContext *c,unsigned mask,unsigned trace_row) {
    if(!GF2VR_LOGGING_ALLOWED())return;
    if(focus_gpu.done || (mask!=1 && mask!=2) ||
       (focus_gpu.owner && (focus_gpu.owner!=w || focus_gpu.ticket.serial!=w->ticket.serial ||
        focus_gpu.ticket.generation!=w->ticket.generation || focus_gpu.ticket.epoch!=w->ticket.epoch))) return;
    unsigned eye=mask==1?0:1;
    if(focus_gpu.count[eye]>=4) return;
    DWORD saved=GetLastError();
    focus_gpu.owner=w;focus_gpu.ticket=w->ticket;
    FocusGpuDraw *r=&focus_gpu.draws[focus_gpu.total++];
    r->eye=eye;r->draw=focus_gpu.count[eye]++;
    ID3D11VertexShader *shader=0;ID3D11DeviceContext_VSGetShader(c,&shader,0,0);
    r->shader=(unsigned long long)shader;if(shader) ID3D11VertexShader_Release(shader);
#ifdef GF2VR_FOCUS_MRT
    /* Schema3 stores a lineage row, NOT a shader address. eye is an observation
       group, NOT an inferred camera. Its downstream association lives in FTL. */
    r->shader=trace_row;
#else
    (void)trace_row;
#endif
    D3D11_VIEWPORT viewport[16]={0};UINT count=16;
    ID3D11DeviceContext_RSGetViewports(c,&count,viewport);
    r->viewport_count=count;if(count) r->viewport=viewport[0];
    ID3D11Buffer *bound[14]={0};ID3D11Device *device=0;
    ID3D11DeviceContext_GetDevice(c,&device);
    ID3D11DeviceContext1 *c1=0;
    if(SUCCEEDED(ID3D11DeviceContext_QueryInterface(c,&focus_context1_iid,(void **)&c1)) && c1) {
        ID3D11DeviceContext1_VSGetConstantBuffers1(c1,0,14,bound,r->first,r->constants);
        r->ranges=1;ID3D11DeviceContext1_Release(c1);
    } else ID3D11DeviceContext_VSGetConstantBuffers(c,0,14,bound);
    for(unsigned slot=0;slot<14;++slot) if(bound[slot]) {
        D3D11_BUFFER_DESC desc;ID3D11Buffer_GetDesc(bound[slot],&desc);
        r->sizes[slot]=desc.ByteWidth;r->create_status[slot]=0x80070057u;
        unsigned long long start=(unsigned long long)r->first[slot]*16;
        unsigned long long bytes=r->ranges?(unsigned long long)r->constants[slot]*16:desc.ByteWidth;
        /* Out-of-resource shader range reads are zero; record only real bytes.
           Never mistake a ring-buffer's base for its bound subrange. */
        if(start<desc.ByteWidth && bytes>desc.ByteWidth-start)bytes=desc.ByteWidth-start;
        if(device && bytes && bytes<=65536 && start<desc.ByteWidth && !(bytes%16) &&
           (desc.BindFlags&D3D11_BIND_CONSTANT_BUFFER)) {
            desc.ByteWidth=(unsigned)bytes;r->captured[slot]=(unsigned)bytes;
            desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
            desc.MiscFlags=0;desc.StructureByteStride=0;
            HRESULT hr=ID3D11Device_CreateBuffer(device,&desc,0,&r->buffers[slot]);
            r->create_status[slot]=(unsigned)hr;
            if(SUCCEEDED(hr) && r->buffers[slot]) {
                D3D11_BOX box={(UINT)start,0,0,(UINT)(start+bytes),1,1};
                ID3D11DeviceContext_CopySubresourceRegion(c,(ID3D11Resource *)r->buffers[slot],0,0,0,0,
                    (ID3D11Resource *)bound[slot],0,&box);
            }
        }
        ID3D11Buffer_Release(bound[slot]);
    }
    if(device) ID3D11Device_Release(device);SetLastError(saved);
}
static wchar_t *fg_number(wchar_t *out,unsigned long long value) {
    wchar_t digits[20];unsigned n=0;do{digits[n++]=(wchar_t)(L'0'+value%10);value/=10;}while(value);
    while(n)*out++=digits[--n];return out;
}
static int fg_write(HANDLE file,const void *data,DWORD size) {
    DWORD written=0;return WriteFile(file,data,size,&written,0) && written==size;
}
static void focus_gpu_end(EyeTargetWatch *w) {
    if(focus_gpu.owner!=w || focus_gpu.done ||
       focus_gpu.ticket.serial!=w->ticket.serial || focus_gpu.ticket.generation!=w->ticket.generation ||
       focus_gpu.ticket.epoch!=w->ticket.epoch) return;
    DWORD saved=GetLastError();focus_gpu.done=1;
    for(unsigned i=0;i<focus_gpu.total;++i) {
        FocusGpuDraw *r=&focus_gpu.draws[i];wchar_t path[512];const wchar_t *base=GF2VR_FOCUS_GPU_DIR;
        unsigned n=0;while(base[n] && n<400){path[n]=base[n];++n;}
        HANDLE file=INVALID_HANDLE_VALUE;
        if(!base[n]) {
            wchar_t *end=path+n;*end++=L'/';end=fg_number(end,GetCurrentProcessId());
            *end++=L'-';end=fg_number(end,w->ticket.serial);*end++=L'-';end=fg_number(end,r->eye);
            *end++=L'-';end=fg_number(end,r->draw);*end++=L'.';
#ifdef GF2VR_FOCUS_MRT
            *end++=L'm';*end++=L'r';*end++=L't';
#else
            *end++=L'v';*end++=L's';*end++=L'c';
#endif
            *end=0;
            file=CreateFileW(path,GENERIC_WRITE,0,0,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,0);
        }
        /* 32-byte uint32 header,32-byte identities,24-byte viewport, then14
           {slot,source bytes,create HRESULT,map HRESULT,payload bytes,
            first constant,constant count}+data. Constant units are16 bytes. */
        unsigned header[8]={0x31535646,2,GetCurrentProcessId(),r->eye,r->draw,14,r->viewport_count,r->ranges};
#ifdef GF2VR_FOCUS_MRT
        header[1]=3;
#endif
        unsigned long long identity[4]={w->ticket.serial,w->ticket.generation,w->ticket.epoch,r->shader};
        int ok=file!=INVALID_HANDLE_VALUE && fg_write(file,header,32) && fg_write(file,identity,32) &&
            fg_write(file,&r->viewport,24);
        for(unsigned slot=0;slot<14;++slot) {
            D3D11_MAPPED_SUBRESOURCE mapped={0};HRESULT hr=(HRESULT)0x80004005u;
            if(r->buffers[slot]) hr=ID3D11DeviceContext_Map((ID3D11DeviceContext *)w->context,
                (ID3D11Resource *)r->buffers[slot],0,D3D11_MAP_READ,D3D11_MAP_FLAG_DO_NOT_WAIT,&mapped);
            unsigned item[7]={slot,r->sizes[slot],r->create_status[slot],(unsigned)hr,
                SUCCEEDED(hr)&&mapped.pData?r->captured[slot]:0,r->first[slot],r->constants[slot]};
            if(ok) ok=fg_write(file,item,28) && (!item[4] || fg_write(file,mapped.pData,item[4]));
            if(SUCCEEDED(hr)) ID3D11DeviceContext_Unmap((ID3D11DeviceContext *)w->context,
                (ID3D11Resource *)r->buffers[slot],0);
            if(r->buffers[slot]) ID3D11Buffer_Release(r->buffers[slot]);r->buffers[slot]=0;
        }
        if(file!=INVALID_HANDLE_VALUE){CloseHandle(file);if(!ok)DeleteFileW(path);}
    }
    focus_gpu.owner=0;SetLastError(saved);
}
#endif
