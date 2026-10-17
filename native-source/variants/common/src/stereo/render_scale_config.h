#ifndef GF2_RENDER_SCALE_CONFIG_H
#define GF2_RENDER_SCALE_CONFIG_H
/* Dedicated schema1 JSON: exactly schemaVersion + eyeRenderScale. Future GUI
   persists this number verbatim; recommended ranges never clamp the runtime. */
typedef struct {const char *p,*end;} RenderScaleJson;
static void rs_ws(RenderScaleJson *j){while(j->p<j->end&&(*j->p==' '||*j->p=='\t'||*j->p=='\r'||*j->p=='\n'))++j->p;}
static int rs_char(RenderScaleJson *j,char c){rs_ws(j);if(j->p==j->end||*j->p!=c)return 0;++j->p;return 1;}
static int rs_key(RenderScaleJson *j,const char *key){if(!rs_char(j,'"'))return 0;while(*key){if(j->p==j->end||*j->p++!=*key++)return 0;}return j->p<j->end&&*j->p++=='"';}
/* JSON grammar, bounded significant mantissa, exponent normalization. No CRT,
   locale dependence, product min/max, or mutation of the persisted raw value. */
static int rs_number(RenderScaleJson *j,double *out){
    rs_ws(j);const char *start=j->p,*p=start;if(p==j->end)return 0;
    if(*p=='-')return 0; /* scale must be positive */
    if(*p=='0')++p;else{if(*p<'1'||*p>'9')return 0;do{++p;}while(p<j->end&&*p>='0'&&*p<='9');}
    const char *integer_end=p,*fraction=0,*fraction_end=0;
    if(p<j->end&&*p=='.'){fraction=++p;while(p<j->end&&*p>='0'&&*p<='9')++p;if(p==fraction)return 0;fraction_end=p;}
    int exponent=0,negative=0;
    if(p<j->end&&(*p=='e'||*p=='E')){++p;if(p<j->end&&(*p=='+'||*p=='-'))negative=*p++=='-';const char *first=p;
        while(p<j->end&&*p>='0'&&*p<='9'){if(exponent>10000)return 0;exponent=exponent*10+(*p++-'0');}if(p==first)return 0;}
    if(negative)exponent=-exponent;
    double value=0;int kept=0,shift=0,significant=0;
    for(const char *q=start;q<integer_end;++q){int digit=*q-'0';if(digit||significant){significant=1;if(kept<18){value=value*10+digit;++kept;}else ++shift;}}
    if(fraction)for(const char *q=fraction;q<fraction_end;++q){int digit=*q-'0';if(digit||significant){significant=1;if(kept<18){value=value*10+digit;++kept;--shift;}}else --shift;}
    shift+=exponent;if(!significant||shift>308||shift< -340)return 0;
    while(shift>=100){value*=1e100;shift-=100;}while(shift<=-100){value/=1e100;shift+=100;}
    while(shift>=10){value*=1e10;shift-=10;}while(shift<=-10){value/=1e10;shift+=10;}
    while(shift>0){value*=10;--shift;}while(shift<0){value/=10;++shift;}
    if(!__builtin_isfinite(value)||value<=0)return 0;j->p=p;*out=value;return 1;
}
static int render_scale_config_parse(const char *text,unsigned length,double *out){
    if(!text||!length||length>4096||!out)return 0;
    /* Optional UTF-8 BOM is compatible with Windows GUI/PowerShell writers. */
    if(length>=3&&(unsigned char)text[0]==239&&(unsigned char)text[1]==187&&(unsigned char)text[2]==191){text+=3;length-=3;}
    RenderScaleJson j={text,text+length};unsigned seen=0;double scale=0;
    if(!rs_char(&j,'{')||rs_char(&j,'}'))return 0;
    do{const char *key=j.p;rs_ws(&j);key=j.p;
        if(rs_key(&j,"schemaVersion")){if(seen&1u||!rs_char(&j,':'))return 0;rs_ws(&j);if(j.p==j.end||*j.p++!='1')return 0;seen|=1u;}
        else{j.p=key;if(!rs_key(&j,"eyeRenderScale")||seen&2u||!rs_char(&j,':')||!rs_number(&j,&scale))return 0;seen|=2u;}
        if(rs_char(&j,'}'))break;if(!rs_char(&j,','))return 0;
    }while(1);
    rs_ws(&j);if(j.p!=j.end||seen!=3u)return 0;*out=scale;return 1;
}
static int render_scale_dimension(unsigned base,unsigned maximum,double scale,unsigned *out){
    if(!base||!maximum||!out||!__builtin_isfinite(scale)||scale<=0)return 0;
    double rounded=(double)base*scale+0.5;
    if(!__builtin_isfinite(rounded)||rounded<1.0||rounded>=16385.0||rounded>=(double)maximum+1.0)return 0;
    *out=(unsigned)rounded;return 1; /* nearest pixel; never clamp to device limit */
}
#endif
