#ifndef GF2_FOCUS_CAPTURE_H
#define GF2_FOCUS_CAPTURE_H
/* Diagnostic-only, serialized render owner. One attempt per process. The
   existing pair event covers these copies; no new wait/flush or publication
   permission. Faulted owners retain staging exactly like their normal refs. */
static struct {
    EyeGpuCopy *owner;
    ID3D11Texture2D *stage[2];
    D3D11_TEXTURE2D_DESC desc;
    unsigned attempted;
} focus_capture;
static void focus_capture_release(void) {
    for(unsigned i=0;i<2;++i) {
        if(focus_capture.stage[i]) ID3D11Texture2D_Release(focus_capture.stage[i]);
        focus_capture.stage[i]=0;
    }
    focus_capture.owner=0;
}
static void focus_capture_begin(EyeGpuCopy *p) {
    if(focus_capture.attempted || p->drain_only) return;
    DWORD saved=GetLastError();focus_capture.attempted=1;
    D3D11_TEXTURE2D_DESC desc;
    ID3D11Texture2D_GetDesc((ID3D11Texture2D *)p->textures[2],&desc);
    /* Raw byte capture supports RGBA/BGRA8 and RGBA16F without color changes. */
    unsigned bpp=desc.Format==DXGI_FORMAT_R16G16B16A16_FLOAT?8:4;
    if(!desc.Width || !desc.Height || desc.Width>8192 || desc.Height>8192 ||
       (unsigned long long)desc.Width*desc.Height*bpp>128ull*1024*1024) goto done;
    focus_capture.desc=desc;
    desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;
    desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;desc.MiscFlags=0;
    for(unsigned i=0;i<2;++i)
        if(FAILED(ID3D11Device_CreateTexture2D((ID3D11Device *)p->device,&desc,0,&focus_capture.stage[i]))) {
            focus_capture_release();goto done;
        }
    focus_capture.owner=p;
    for(unsigned i=0;i<2;++i)
        ID3D11DeviceContext_CopyResource((ID3D11DeviceContext *)p->context,
            (ID3D11Resource *)focus_capture.stage[i],(ID3D11Resource *)p->textures[i+2]);
done:
    SetLastError(saved);
}
static wchar_t *focus_capture_number(wchar_t *out,unsigned long long value) {
    wchar_t reverse[20];unsigned n=0;
    do {reverse[n++]=(wchar_t)(L'0'+value%10);value/=10;} while(value);
    while(n) *out++=reverse[--n];return out;
}
static int focus_capture_write(HANDLE file,const void *data,DWORD length) {
    DWORD written=0;
    return WriteFile(file,data,length,&written,0) && written==length;
}
static void focus_capture_end(EyeGpuCopy *p) {
    if(focus_capture.owner!=p || p->state!=EGC_DRAINED) return;
    DWORD saved=GetLastError();
    for(unsigned eye=0;eye<2;++eye) {
        D3D11_MAPPED_SUBRESOURCE mapped={0};
        HRESULT hr=ID3D11DeviceContext_Map((ID3D11DeviceContext *)p->context,
            (ID3D11Resource *)focus_capture.stage[eye],0,D3D11_MAP_READ,D3D11_MAP_FLAG_DO_NOT_WAIT,&mapped);
        if(FAILED(hr)) continue; /* Never stall/retry or affect the normal pair. */
        wchar_t path[512];const wchar_t *base=GF2VR_FOCUS_CAPTURE_DIR;
        unsigned n=0;while(base[n] && n<400){path[n]=base[n];++n;}
        if(!base[n]) {
            wchar_t *end=path+n;*end++=L'/';end=focus_capture_number(end,GetCurrentProcessId());
            *end++=L'-';end=focus_capture_number(end,p->ticket.serial);*end++=L'-';*end++=(wchar_t)(L'0'+eye);
            *end++=L'.';*end++=L'e';*end++=L'y';*end++=L'e';*end=0;
            HANDLE file=CreateFileW(path,GENERIC_WRITE,0,0,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,0);
            if(file!=INVALID_HANDLE_VALUE) {
                /* Fixed little-endian 64-byte header; tight rows follow.
                   magic/version/width/height/format/rowbytes/pid/eye then
                   serial/generation/epoch/reserved as four uint64 values. */
                unsigned bpp=focus_capture.desc.Format==DXGI_FORMAT_R16G16B16A16_FLOAT?8:4;
                unsigned header[8]={0x31455945,1,focus_capture.desc.Width,focus_capture.desc.Height,
                    focus_capture.desc.Format,focus_capture.desc.Width*bpp,GetCurrentProcessId(),eye};
                unsigned long long identity[4]={p->ticket.serial,p->ticket.generation,p->ticket.epoch,0};
                int ok=mapped.pData && mapped.RowPitch>=header[5] &&
                    focus_capture_write(file,header,sizeof(header)) && focus_capture_write(file,identity,sizeof(identity));
                for(unsigned y=0;ok && y<header[3];++y)
                    ok=focus_capture_write(file,(unsigned char *)mapped.pData+(size_t)y*mapped.RowPitch,header[5]);
                CloseHandle(file);
                if(!ok) DeleteFileW(path); /* Only this newly created partial artifact. */
            }
        }
        ID3D11DeviceContext_Unmap((ID3D11DeviceContext *)p->context,(ID3D11Resource *)focus_capture.stage[eye],0);
    }
    focus_capture_release();SetLastError(saved);
}
#endif
