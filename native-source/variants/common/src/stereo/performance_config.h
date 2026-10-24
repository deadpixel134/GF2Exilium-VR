#ifndef GF2VR_PERFORMANCE_CONFIG_H
#define GF2VR_PERFORMANCE_CONFIG_H
/* Cold schema1 configuration. No dependency on render_quality.json. A bad
   known object resets only that object; an invalid envelope resets both. */
typedef struct { unsigned fixed,width,height,always; } PcOutputConfig;
typedef struct {
    unsigned enabled,low_windows,high_windows,min_interval_ms;
    double floor,step,low_fresh_per_second,high_fresh_per_second;
} PerformanceDynamicConfig;
typedef struct { PcOutputConfig pc; PerformanceDynamicConfig dynamic; unsigned invalid; } PerformanceConfig;
enum { PERFORMANCE_BAD_PC=1, PERFORMANCE_BAD_DYNAMIC=2, PERFORMANCE_BAD_FILE=4 };
static PerformanceConfig performance_config_default(void) {
    return (PerformanceConfig){{0,2560,1440,0},{0,2,5,10000,0.6,0.15,70,80},0};
}
typedef struct { const char *p,*end; } PerformanceJson;
static void pj_ws(PerformanceJson *j) { while(j->p<j->end && (*j->p==' '||*j->p=='\t'||*j->p=='\r'||*j->p=='\n'))++j->p; }
static int pj_char(PerformanceJson *j,char c) { pj_ws(j);if(j->p==j->end||*j->p!=c)return 0;++j->p;return 1; }
static int pj_string(PerformanceJson *j,char *out,unsigned cap) {
    if(!pj_char(j,'"'))return 0;unsigned n=0;
    while(j->p<j->end) {
        unsigned c=(unsigned char)*j->p++;
        if(c=='"') { if(out&&cap)out[n<cap?n:cap-1]=0;return 1; }
        if(c<32)return 0;
        if(c=='\\') {
            if(j->p==j->end)return 0;c=(unsigned char)*j->p++;
            if(c=='u') {
                c=0;for(unsigned k=0;k<4;++k) {
                    if(j->p==j->end)return 0;unsigned h=(unsigned char)*j->p++;
                    if(h>='0'&&h<='9')h-='0';else if(h>='a'&&h<='f')h=h-'a'+10;
                    else if(h>='A'&&h<='F')h=h-'A'+10;else return 0;c=c*16+h;
                }
                /* All contract names/enum values are ASCII; preserve unknowns
                   as nonmatching, while still validating their JSON escape. */
                if(!c||c>127)c=255;
            } else if(c=='b'||c=='f'||c=='n'||c=='r'||c=='t')c=255;
            else if(c!='"'&&c!='\\'&&c!='/')return 0;
        }
        if(out&&n+1<cap)out[n]=(char)c;++n;
    }
    return 0;
}
static int pj_equal(const char *a,const char *b) { while(*a&&*a==*b){++a;++b;}return *a==*b; }
static int pj_number(PerformanceJson *j,double *out) {
    pj_ws(j);const char *p=j->p;int negative=0,exponent=0,esign=1;double v=0;
    if(p<j->end&&*p=='-'){negative=1;++p;}
    if(p==j->end)return 0;
    if(*p=='0')++p;
    else {if(*p<'1'||*p>'9')return 0;while(p<j->end&&*p>='0'&&*p<='9')v=v*10+(*p++-'0');}
    if(p<j->end&&*p=='.') {double divisor=1;++p;const char *start=p;
        while(p<j->end&&*p>='0'&&*p<='9'){v=v*10+(*p++-'0');divisor*=10;}if(p==start)return 0;v/=divisor;}
    if(p<j->end&&(*p=='e'||*p=='E')) {++p;if(p<j->end&&(*p=='+'||*p=='-')){if(*p=='-')esign=-1;++p;}
        const char *start=p;while(p<j->end&&*p>='0'&&*p<='9'){if(exponent<10000)exponent=exponent*10+(*p-'0');++p;}if(p==start)return 0;}
    if(out) {if(exponent>340)return 0;while(exponent--){if(esign>0)v*=10;else v/=10;}if(!__builtin_isfinite(v))return 0;*out=negative?-v:v;}
    j->p=p;return 1;
}
static int pj_word(PerformanceJson *j,const char *s) {const char *p=j->p;while(*s){if(p==j->end||*p++!=*s++)return 0;}j->p=p;return 1;}
static int pj_value(PerformanceJson *j,unsigned depth) {
    if(depth>16)return 0;pj_ws(j);if(j->p==j->end)return 0;
    if(*j->p=='"')return pj_string(j,0,0);
    if(*j->p=='{'||*j->p=='[') {char open=*j->p++,close=open=='{'?'}':']';if(pj_char(j,close))return 1;
        do {if(open=='{'&&(!pj_string(j,0,0)||!pj_char(j,':')))return 0;
            if(!pj_value(j,depth+1))return 0;if(pj_char(j,close))return 1;
        }while(pj_char(j,','));return 0;}
    return pj_word(j,"true")||pj_word(j,"false")||pj_word(j,"null")||pj_number(j,0);
}
/* Find a known object's envelope even if its internal grammar is bad. This
   allows a valid sibling to survive a malformed member/trailing comma. */
static int pj_span(PerformanceJson *j,PerformanceJson *value) {
    pj_ws(j);value->p=j->p;
    if(j->p<j->end&&*j->p=='{') {unsigned depth=0;
        do {char c=*j->p;if(c=='"'){if(!pj_string(j,0,0))return 0;continue;}
            ++j->p;if(c=='{'){if(++depth>16)return 0;}else if(c=='}')--depth;
        }while(j->p<j->end&&depth);if(depth)return 0;
    } else if(!pj_value(j,0))return 0;
    value->end=j->p;return 1;
}
static int pj_uint(PerformanceJson *j,unsigned *out,unsigned maximum) {
    double v;if(!pj_number(j,&v)||v<1||v>maximum||v!=(unsigned)v)return 0;*out=(unsigned)v;return 1;
}
static int performance_pc_parse(PerformanceJson j,PcOutputConfig *out) {
    PcOutputConfig p=performance_config_default().pc;unsigned seen=0;
    if(!pj_char(&j,'{'))return 0;if(!pj_char(&j,'}'))do {
        char key[64],value[64];unsigned bit=0;
        if(!pj_string(&j,key,sizeof(key))||!pj_char(&j,':'))return 0;
        if(pj_equal(key,"mode")){bit=1;if(!pj_string(&j,value,sizeof(value)))return 0;
            if(pj_equal(value,"FIXED"))p.fixed=1;else if(!pj_equal(value,"NATIVE"))return 0;}
        else if(pj_equal(key,"width")){bit=2;if(!pj_uint(&j,&p.width,16384))return 0;}
        else if(pj_equal(key,"height")){bit=4;if(!pj_uint(&j,&p.height,16384))return 0;}
        else if(pj_equal(key,"scope")){bit=8;if(!pj_string(&j,value,sizeof(value)))return 0;
            if(pj_equal(value,"ALWAYS"))p.always=1;else if(!pj_equal(value,"IMMERSIVE_ONLY"))return 0;}
        else if(!pj_value(&j,0))return 0;
        if(bit&seen)return 0;seen|=bit;
        if(pj_char(&j,'}'))break;if(!pj_char(&j,','))return 0;
    }while(1);
    pj_ws(&j);if(j.p!=j.end)return 0;*out=p;return 1;
}
static int performance_dynamic_parse(PerformanceJson j,PerformanceDynamicConfig *out) {
    PerformanceDynamicConfig d=performance_config_default().dynamic;unsigned seen=0;
    if(!pj_char(&j,'{'))return 0;if(!pj_char(&j,'}'))do {
        char key[64];unsigned bit=0;if(!pj_string(&j,key,sizeof(key))||!pj_char(&j,':'))return 0;pj_ws(&j);
        if(pj_equal(key,"enabled")){bit=1;if(pj_word(&j,"true"))d.enabled=1;else if(pj_word(&j,"false"))d.enabled=0;else return 0;}
        else if(pj_equal(key,"floor")){bit=2;if(!pj_number(&j,&d.floor))return 0;}
        else if(pj_equal(key,"step")){bit=4;if(!pj_number(&j,&d.step))return 0;}
        else if(pj_equal(key,"lowFreshPerSecond")){bit=8;if(!pj_number(&j,&d.low_fresh_per_second))return 0;}
        else if(pj_equal(key,"highFreshPerSecond")){bit=16;if(!pj_number(&j,&d.high_fresh_per_second))return 0;}
        else if(pj_equal(key,"lowWindows")){bit=32;if(!pj_uint(&j,&d.low_windows,2147483647))return 0;}
        else if(pj_equal(key,"highWindows")){bit=64;if(!pj_uint(&j,&d.high_windows,2147483647))return 0;}
        else if(pj_equal(key,"minIntervalMs")){bit=128;if(!pj_uint(&j,&d.min_interval_ms,2147483647))return 0;}
        else if(!pj_value(&j,0))return 0;
        if(bit&seen)return 0;seen|=bit;if(pj_char(&j,'}'))break;if(!pj_char(&j,','))return 0;
    }while(1);
    pj_ws(&j);if(j.p!=j.end||d.floor<0.3||d.floor>1||d.step<0.05||d.step>0.5||
        d.low_fresh_per_second<10||d.high_fresh_per_second>200||d.low_fresh_per_second>=d.high_fresh_per_second)return 0;
    *out=d;return 1;
}
static PerformanceConfig performance_config_parse(const char *text,unsigned size) {
    PerformanceConfig c=performance_config_default();unsigned seen=0,schema=1;
    if(!text||!size||size>16384)goto bad;
    if(size>=3&&(unsigned char)text[0]==239&&(unsigned char)text[1]==187&&(unsigned char)text[2]==191){text+=3;size-=3;}
    PerformanceJson j={text,text+size};if(!pj_char(&j,'{'))goto bad;
    if(!pj_char(&j,'}'))do {
        char key[64];PerformanceJson value;unsigned bit=0;
        if(!pj_string(&j,key,sizeof(key))||!pj_char(&j,':'))goto bad;
        if(pj_equal(key,"pcOutput")){bit=1;if(!pj_span(&j,&value))goto bad;
            if((seen&bit)||!performance_pc_parse(value,&c.pc))c.invalid|=PERFORMANCE_BAD_PC;}
        else if(pj_equal(key,"dynamicEyeScale")){bit=2;if(!pj_span(&j,&value))goto bad;
            if((seen&bit)||!performance_dynamic_parse(value,&c.dynamic))c.invalid|=PERFORMANCE_BAD_DYNAMIC;}
        else if(pj_equal(key,"schemaVersion")){bit=4;if((seen&bit)||!pj_uint(&j,&schema,1))goto bad;}
        else if(!pj_value(&j,0))goto bad;
        seen|=bit;if(pj_char(&j,'}'))break;if(!pj_char(&j,','))goto bad;
    }while(1);
    pj_ws(&j);if(j.p!=j.end)goto bad;
    if(c.invalid&PERFORMANCE_BAD_PC)c.pc=performance_config_default().pc;
    if(c.invalid&PERFORMANCE_BAD_DYNAMIC)c.dynamic=performance_config_default().dynamic;
    return c;
bad:c=performance_config_default();c.invalid=PERFORMANCE_BAD_FILE;return c;
}
#endif
