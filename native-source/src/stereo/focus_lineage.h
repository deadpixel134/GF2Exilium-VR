#ifndef GF2_FOCUS_LINEAGE_H
#define GF2_FOCUS_LINEAGE_H
/* Diagnostic only: resource-level bindings, NOT shader consumption or complete
   producer coverage (copies/compute/deferred/indirect calls are not hooked).
   One existing eye interval. Canonical COM identities are retained until its
   normal drain, preventing pointer reuse from fabricating a lineage edge. */
#define FL_ROWS 4096
#define FL_OBJECTS 2048
typedef struct {
    unsigned kind,mask,writable,predicated,topology,vs,ps,reserved;
    unsigned outputs[8],inputs[128];
} FocusLineageRow;
typedef struct {unsigned kind,width,height,format,mips,array_size,reserved[2];} FocusLineageObject;
static struct {
    EyeTargetWatch *owner;
    EyeGpuTicket ticket;
    unsigned done,count,objects,overflow,seen;
    IUnknown *refs[FL_OBJECTS];
    FocusLineageObject descriptions[FL_OBJECTS];
    FocusLineageRow rows[FL_ROWS];
} fl;
#ifdef GF2VR_FOCUS_MRT
static unsigned fl_mrt_group;
/* The first two chronological windows ending at a writable private draw.
   Clears never label a window. Capture is only a candidate-stage witness:
   downstream eye ownership/consumption must be established from the trace. */
static void fl_mrt_observe(EyeTargetWatch *w,ID3D11DeviceContext *c,FocusLineageRow *r) {
    if(fl_mrt_group>=2 || fl.overflow) return;
    if(r->mask) {
        unsigned written=0;
        for(unsigned i=0;i<8;++i) if(r->outputs[i] && (r->writable&(1u<<i))) {
            IUnknown *id=fl.refs[r->outputs[i]-1];
            if(id==w->identity[0]) written|=1;
            if(id==w->identity[1]) written|=2;
        }
        if(!r->predicated && written) ++fl_mrt_group;
        return;
    }
    if(r->predicated || (r->writable&7)!=7 || !r->outputs[0] ||
       !r->outputs[1] || !r->outputs[2]) return;
    for(unsigned i=3;i<8;++i) if(r->outputs[i]) return;
    static const GUID texture_iid={0x6f15aaf2,0xd208,0x4e89,{0x9a,0xb4,0x48,0x95,0x35,0xd3,0x4f,0x9c}};
    ID3D11Texture2D *target=0;D3D11_TEXTURE2D_DESC td;
    if(FAILED(IUnknown_QueryInterface((IUnknown *)w->identity[0],&texture_iid,(void **)&target)) || !target) return;
    ID3D11Texture2D_GetDesc(target,&td);etw_release(target);
    for(unsigned i=0;i<3;++i) {
        FocusLineageObject *d=&fl.descriptions[r->outputs[i]-1];
        if(d->kind!=1 || d->width!=td.Width || d->height!=td.Height || d->mips!=1 || d->array_size!=1) return;
    }
    focus_gpu_draw(w,c,1u<<fl_mrt_group,(unsigned)(r-fl.rows));
}
#endif
static int fl_owner(EyeTargetWatch *w) {
    if(fl.done) return 0;
    if(!fl.owner){fl.owner=w;fl.ticket=w->ticket;}
    return fl.owner==w && etw_ticket(fl.ticket,w->ticket);
}
static unsigned fl_object(void *object,unsigned kind) {
    IUnknown *identity=etw_identity(object);
    if(!identity) return 0;
    for(unsigned i=0;i<fl.objects;++i) if(fl.refs[i]==identity){etw_release(identity);return i+1;}
    if(fl.objects==FL_OBJECTS){fl.overflow|=2;etw_release(identity);return 0;}
    unsigned index=fl.objects++;fl.refs[index]=identity;
    FocusLineageObject *d=&fl.descriptions[index];d->kind=kind;
    if(kind==1) {
        ID3D11Resource *resource=object;D3D11_RESOURCE_DIMENSION type;
        ID3D11Resource_GetType(resource,&type);
        if(type==D3D11_RESOURCE_DIMENSION_TEXTURE2D) {
            static const GUID iid={0x6f15aaf2,0xd208,0x4e89,{0x9a,0xb4,0x48,0x95,0x35,0xd3,0x4f,0x9c}};
            ID3D11Texture2D *texture=0;
            if(SUCCEEDED(ID3D11Resource_QueryInterface(resource,&iid,(void **)&texture)) && texture) {
                D3D11_TEXTURE2D_DESC td;ID3D11Texture2D_GetDesc(texture,&td);
                d->width=td.Width;d->height=td.Height;d->format=td.Format;
                d->mips=td.MipLevels;d->array_size=td.ArraySize;etw_release(texture);
            }
        }
    }
    return index+1;
}
static unsigned fl_view(ID3D11View *view) {
    if(!view) return 0;
    ID3D11Resource *resource=0;ID3D11View_GetResource(view,&resource);
    unsigned id=fl_object(resource,1);etw_release(resource);return id;
}
static FocusLineageRow *fl_row(EyeTargetWatch *w,unsigned kind) {
    if(!fl_owner(w)) return 0;
    if(fl.seen!=~0u) ++fl.seen;
    if(fl.count==FL_ROWS){fl.overflow|=1;return 0;}
    FocusLineageRow *r=&fl.rows[fl.count++];r->kind=kind;return r;
}
static void focus_lineage_clear(EyeTargetWatch *w,ID3D11RenderTargetView *view,unsigned mask) {
    FocusLineageRow *r=fl_row(w,2);if(!r)return;
    r->mask=mask;r->outputs[0]=fl_view((ID3D11View *)view);
}
static void focus_lineage_draw(EyeTargetWatch *w,ID3D11DeviceContext *c) {
    FocusLineageRow *r=fl_row(w,1);if(!r)return;
    ID3D11RenderTargetView *rt[8]={0};ID3D11ShaderResourceView *srv[128]={0};
    ID3D11DeviceContext_OMGetRenderTargets(c,8,rt,0);
    ID3D11DeviceContext_PSGetShaderResources(c,0,128,srv);
    ID3D11BlendState *blend=0;FLOAT factors[4];UINT sample_mask=0;D3D11_BLEND_DESC bd={0};
    ID3D11DeviceContext_OMGetBlendState(c,&blend,factors,&sample_mask);
    if(blend) ID3D11BlendState_GetDesc(blend,&bd);
    for(unsigned i=0;i<8;++i) {
        r->outputs[i]=fl_view((ID3D11View *)rt[i]);r->mask|=etw_view(w,rt[i]);
        if(rt[i] && sample_mask && (!blend || bd.RenderTarget[bd.IndependentBlendEnable?i:0].RenderTargetWriteMask)) r->writable|=1u<<i;
        etw_release(rt[i]);
    }
    etw_release(blend);
    for(unsigned i=0;i<128;++i){r->inputs[i]=fl_view((ID3D11View *)srv[i]);etw_release(srv[i]);}
    ID3D11Predicate *predicate=0;BOOL value=0;
    ID3D11DeviceContext_GetPredication(c,&predicate,&value);r->predicated=predicate?1:0;etw_release(predicate);
    ID3D11VertexShader *vs=0;ID3D11PixelShader *ps=0;
    ID3D11DeviceContext_VSGetShader(c,&vs,0,0);ID3D11DeviceContext_PSGetShader(c,&ps,0,0);
    r->vs=fl_object(vs,2);r->ps=fl_object(ps,3);etw_release(vs);etw_release(ps);
    D3D11_PRIMITIVE_TOPOLOGY topology;ID3D11DeviceContext_IAGetPrimitiveTopology(c,&topology);r->topology=topology;
#ifdef GF2VR_FOCUS_MRT
    fl_mrt_observe(w,c,r);
#endif
}
static void focus_lineage_end(EyeTargetWatch *w) {
    if(fl.done || fl.owner!=w || !etw_ticket(fl.ticket,w->ticket))return;
    fl.done=1;
    wchar_t path[512];const wchar_t *base=GF2VR_FOCUS_LINEAGE_DIR;unsigned n=0;
    while(base[n] && n<400){path[n]=base[n];++n;}
    if(!base[n]) {
        wchar_t *end=path+n;*end++=L'/';end=fg_number(end,GetCurrentProcessId());
        *end++=L'-';end=fg_number(end,w->ticket.serial);
        *end++=L'.';*end++=L'f';*end++=L't';*end++=L'l';*end=0;
        HANDLE file=CreateFileW(path,GENERIC_WRITE,0,0,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,0);
        if(file!=INVALID_HANDLE_VALUE) {
            unsigned header[8]={0x314c5446,1,GetCurrentProcessId(),fl.count,fl.objects,fl.overflow,fl.seen,sizeof(FocusLineageRow)};
            unsigned long long ids[4]={w->ticket.serial,w->ticket.generation,w->ticket.epoch,0};
            int ok=fg_write(file,header,32) && fg_write(file,ids,32) &&
                fg_write(file,fl.descriptions,fl.objects*sizeof(FocusLineageObject)) &&
                fg_write(file,fl.rows,fl.count*sizeof(FocusLineageRow));
            CloseHandle(file);if(!ok)DeleteFileW(path);
        }
    }
    for(unsigned i=0;i<fl.objects;++i){etw_release(fl.refs[i]);fl.refs[i]=0;}
    fl.owner=0;
}
#endif
