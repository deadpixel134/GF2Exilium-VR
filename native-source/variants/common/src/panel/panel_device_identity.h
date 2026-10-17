#ifndef GF2_PANEL_DEVICE_IDENTITY_H
#define GF2_PANEL_DEVICE_IDENTITY_H
/* Requires PanelGuid, panel_qi and panel_release from panel_capture.h.
   These helpers use only COM QueryInterface; they never inspect wrapper memory
   or replace device/context vtables. Successful QI references are balanced. */
static const PanelGuid panel_identity_iid={0,0,0,{0xc0,0,0,0,0,0,0,0x46}};
static const PanelGuid panel_migoto_device_iid={0x83ffd841,0xa5c9,0x46f4,{0x81,0x09,0xbc,0x25,0x95,0x58,0xfe,0xf4}};
static const PanelGuid panel_migoto_context_iid={0xa3046b1e,0x336b,0x4d90,{0x9f,0xd6,0x23,0x4b,0xc0,0x9b,0x86,0x87}};
static int panel_same_device(void *a,void *b) {
    if(!a || !b) return 0;
    if(a==b) return 1;
    void *x=0,*y=0;
    long hx=panel_qi(a,&panel_identity_iid,&x);
    long hy=panel_qi(b,&panel_identity_iid,&y);
    int same=hx>=0 && hy>=0 && x && y && x==y;
    panel_release(x);panel_release(y);return same;
}
static int panel_has_migoto_interface(void *object,const PanelGuid *iid) {
    if(!object) return 0;
    void *wrapper=0;
    long result=panel_qi(object,iid,&wrapper);
    panel_release(wrapper);
    return result>=0;
}
#endif
