#ifndef GF2VR_LIVE_SETTINGS_WIRE_H
#define GF2VR_LIVE_SETTINGS_WIRE_H
#include "live_settings_surface.h"
enum { LSW_MAGIC=0x31535647,LSW_VERSION=1,LSW_CAPACITY=262144,LSW_DOCUMENTS=10 };
enum { LSW_STATE=1,LSW_PREPARE,LSW_APPLY,LSW_FINALIZE,LSW_ROLLBACK };
enum { LSW_OK,LSW_REJECTED,LSW_BUSY,LSW_UNAVAILABLE,LSW_PENDING };
enum { LSD_RENDER,LSD_PERFORMANCE,LSD_DEPTH,LSD_WORLD,LSD_ACTIVITY,LSD_INPUT,LSD_HANDS,LSD_NAVIGATION,LSD_DRAG,LSD_CONTACT };
static const char *const live_settings_files[LSW_DOCUMENTS]={"render_quality.json","performance.json","stereo_depth.json","world_scale.json",
    "activity_ui.json","controller_input.json","controller_hands.json","navigation.json","world_drag.json","touch_interaction.json"};
typedef struct {unsigned length;char text[16385];} LiveSettingsDocument;
typedef struct {LiveSettingsDocument files[LSW_DOCUMENTS];unsigned count;} LiveSettingsDocuments;
typedef struct {
    unsigned started,initialized,ready,command,length,status,dirty,pid,open_request;
    unsigned long long request,revision,instance;
    unsigned short root[1024];char root_utf8[4096],build[128];
    unsigned char payload[LSW_CAPACITY];
    LiveSettingsSurface *surface;
#ifdef GF2VR_LIVE_SETTINGS_ENTRY_OBSERVE
    unsigned long long entry_host_status,entry_host_process,entry_transport;
#endif
} LiveSettingsWire;
extern LiveSettingsWire live_settings_wire;
void live_settings_service_start(const unsigned short *root,const char *build,unsigned dirty);
static unsigned live_settings_get32(const unsigned char *p){return (unsigned)p[0]|((unsigned)p[1]<<8)|((unsigned)p[2]<<16)|((unsigned)p[3]<<24);}
static unsigned long long live_settings_get64(const unsigned char *p){return live_settings_get32(p)|((unsigned long long)live_settings_get32(p+4)<<32);}
static void live_settings_put32(unsigned char *p,unsigned v){for(unsigned i=0;i<4;++i)p[i]=(unsigned char)(v>>(i*8));}
static void live_settings_put64(unsigned char *p,unsigned long long v){for(unsigned i=0;i<8;++i)p[i]=(unsigned char)(v>>(i*8));}
static unsigned live_settings_length(const char *s){unsigned n=0;while(s[n])++n;return n;}
static int live_settings_equal(const unsigned char *p,unsigned n,const char *s){unsigned i=0;while(i<n&&s[i]&&p[i]==(unsigned char)s[i])++i;return i==n&&!s[i];}
static unsigned live_settings_limit(unsigned index){return index==LSD_PERFORMANCE?16384:4096;}
static int live_settings_documents_decode(const unsigned char *p,unsigned length,unsigned dirty,LiveSettingsDocuments *out){
    if(!p||length<4||!out)return 0;unsigned count=live_settings_get32(p),seen=0,at=4,expected=dirty?10:9;
    if(count!=expected)return 0;
    for(unsigned i=0;i<count;++i){
        if(length-at<4)return 0;unsigned n=live_settings_get32(p+at);at+=4;
        if(n>64||n>length-at)return 0;
        unsigned index=0;while(index<expected&&!live_settings_equal(p+at,n,live_settings_files[index]))++index;
        if(index==expected||(seen&(1u<<index)))return 0;seen|=1u<<index;at+=n;
        if(length-at<4)return 0;n=live_settings_get32(p+at);at+=4;
        if(!n||n>live_settings_limit(index)||n>length-at)return 0;
        for(unsigned k=0;k<n;++k)out->files[index].text[k]=(char)p[at+k];
        out->files[index].text[n]=0;out->files[index].length=n;at+=n;
    }
    if(at!=length)return 0;out->count=count;return 1;
}
static unsigned live_settings_documents_encode(unsigned char *p,const LiveSettingsDocuments *docs){
    unsigned at=4;live_settings_put32(p,docs->count);
    for(unsigned i=0;i<docs->count;++i){
        unsigned n=live_settings_length(live_settings_files[i]);live_settings_put32(p+at,n);at+=4;
        for(unsigned k=0;k<n;++k)p[at++]=(unsigned char)live_settings_files[i][k];
        n=docs->files[i].length;live_settings_put32(p+at,n);at+=4;
        for(unsigned k=0;k<n;++k)p[at++]=(unsigned char)docs->files[i].text[k];
    }
    return at;
}
#endif
