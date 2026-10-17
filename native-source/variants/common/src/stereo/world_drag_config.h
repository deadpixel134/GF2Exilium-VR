#ifndef GF2_WORLD_DRAG_CONFIG_H
#define GF2_WORLD_DRAG_CONFIG_H
#include "world_drag.h"
/* Dedicated version1 JSON file. Only the top-level worldDrag object is
   interpreted. Unknown nested JSON values are validated/skipped; duplicate
   recognized keys, non-boolean known values and malformed files fail closed. */
typedef struct {const char *p,*end;unsigned depth;} WorldDragJson;
static void wd_json_ws(WorldDragJson *j) {while(j->p<j->end&&(*j->p==' '||*j->p=='\r'||*j->p=='\n'||*j->p=='\t'))++j->p;}
static int wd_json_char(WorldDragJson *j,char c) {wd_json_ws(j);if(j->p==j->end||*j->p!=c)return 0;++j->p;return 1;}
static int wd_json_word(WorldDragJson *j,const char *word) {
    wd_json_ws(j);const char *p=j->p;while(*word) {if(p==j->end||*p++!=*word++)return 0;}j->p=p;return 1;
}
static int wd_json_string(WorldDragJson *j,char *name,unsigned cap) {
    if(!wd_json_char(j,'"'))return 0;unsigned n=0,escaped=0;
    while(j->p<j->end) {
        unsigned char c=(unsigned char)*j->p++;
        if(c=='"') {if(name&&cap)name[n<cap?n:cap-1]=0;return 1;}
        if(c<32)return 0;
        if(c=='\\') {
            if(name)return 0; /* property names use unescaped ASCII contract */
            escaped=1;if(j->p==j->end)return 0;c=(unsigned char)*j->p++;
            if(c=='u') {for(unsigned i=0;i<4;++i) {if(j->p==j->end)return 0;char h=*j->p++;if(!((h>='0'&&h<='9')||(h>='a'&&h<='f')||(h>='A'&&h<='F')))return 0;}}
            else if(!(c=='"'||c=='\\'||c=='/'||c=='b'||c=='f'||c=='n'||c=='r'||c=='t'))return 0;
        }
        if(name&&n+1<cap)name[n]=escaped?'?':(char)c;++n;
    }
    return 0;
}
static int wd_json_equal(const char *a,const char *b) {while(*a&&*a==*b) {++a;++b;}return *a==*b;}
static int wd_json_skip(WorldDragJson *j) {
    wd_json_ws(j);if(j->p==j->end||j->depth>=16)return 0;
    char c=*j->p;
    if(c=='"')return wd_json_string(j,0,0);
    if(c=='{'||c=='[') {
        ++j->p;++j->depth;char close=c=='{'?'}':']';
        if(wd_json_char(j,close)) {--j->depth;return 1;}
        do {if(c=='{'&&(!wd_json_string(j,0,0)||!wd_json_char(j,':')))return 0;
            if(!wd_json_skip(j))return 0;if(wd_json_char(j,close)) {--j->depth;return 1;}
        }while(wd_json_char(j,','));return 0;
    }
    if(c=='t')return wd_json_word(j,"true");if(c=='f')return wd_json_word(j,"false");if(c=='n')return wd_json_word(j,"null");
    if(c=='-') {++j->p;if(j->p==j->end)return 0;}
    if(j->p==j->end)return 0;
    if(*j->p=='0')++j->p;
    else {if(*j->p<'1'||*j->p>'9')return 0;do {++j->p;}while(j->p<j->end&&*j->p>='0'&&*j->p<='9');}
    if(j->p<j->end&&*j->p=='.') {++j->p;const char *first=j->p;while(j->p<j->end&&*j->p>='0'&&*j->p<='9')++j->p;if(first==j->p)return 0;}
    if(j->p<j->end&&(*j->p=='e'||*j->p=='E')) {++j->p;if(j->p<j->end&&(*j->p=='-'||*j->p=='+'))++j->p;
        const char *first=j->p;while(j->p<j->end&&*j->p>='0'&&*j->p<='9')++j->p;if(first==j->p)return 0;}
    return 1;
}
static int world_drag_config_parse(const char *text,unsigned length,WorldDragSettings *out) {
    if(!out)return 0;*out=world_drag_defaults();if(!text||!length)return 0;
    WorldDragJson j={text,text+length,0};WorldDragSettings s=*out;
    unsigned found=0,seen=0;
    if(!wd_json_char(&j,'{'))return 0;
    if(!wd_json_char(&j,'}'))do {
        char key[64]={0};if(!wd_json_string(&j,key,64)||!wd_json_char(&j,':'))return 0;
        if(wd_json_equal(key,"worldDrag")) {
            if(found++||!wd_json_char(&j,'{'))return 0;
            if(!wd_json_char(&j,'}'))do {
                char name[64]={0};if(!wd_json_string(&j,name,64)||!wd_json_char(&j,':'))return 0;
                const char *keys[7]={"enabled","leftGripActivation","leftTriggerActivation","rightGripActivation","rightTriggerActivation","trackLeftHand","trackRightHand"};
                unsigned *fields[7]={&s.enabled,&s.leftGripActivation,&s.leftTriggerActivation,&s.rightGripActivation,&s.rightTriggerActivation,&s.trackLeftHand,&s.trackRightHand};
                unsigned i=0;while(i<7&&!wd_json_equal(name,keys[i]))++i;
                if(i==7) {if(!wd_json_skip(&j))return 0;}
                else {
                    if(seen&(1u<<i))return 0;seen|=1u<<i;
                    if(wd_json_word(&j,"true"))*fields[i]=1;else if(wd_json_word(&j,"false"))*fields[i]=0;else return 0;
                }
                if(wd_json_char(&j,'}'))break;if(!wd_json_char(&j,','))return 0;
            }while(1);
        }else if(wd_json_equal(key,"schemaVersion")) {
            if(seen&128u)return 0;seen|=128u;if(!wd_json_word(&j,"1"))return 0;
        }else if(!wd_json_skip(&j))return 0;
        if(wd_json_char(&j,'}'))break;if(!wd_json_char(&j,','))return 0;
    }while(1);
    wd_json_ws(&j);if(j.p!=j.end||!world_drag_settings_valid(&s))return 0;
    *out=s;return 1;
}
#endif
