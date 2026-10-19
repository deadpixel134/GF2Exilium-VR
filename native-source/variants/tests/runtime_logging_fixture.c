#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define GF2VR_OPTIONAL_LOGGING 1
#define GF2VR_PORTABLE_BINDING 1
#define GF2VR_TOUCH_PRIVATE 1
#include "../common/src/bootstrap/callback_observer.c"
void gf2vr_version_set_self(HINSTANCE unused){(void)unused;}
static unsigned checks;
static void check(int ok,const char *what){++checks;if(!ok){fprintf(stderr,"FAIL %s\n",what);exit(1);}}
static void config(const char *path,const char *text){FILE *f=fopen(path,"wb");check(f!=0,"fixture config open");fwrite(text,1,strlen(text),f);fclose(f);}
static long size(const char *path){FILE *f=fopen(path,"rb");if(!f)return -1;fseek(f,0,SEEK_END);long n=ftell(f);fclose(f);return n;}
int main(int argc,char **argv){
 check(argc==2,"inert fixture root argument");char path[1000],log[1000];
 snprintf(path,sizeof(path),"%s/config.json",argv[1]);snprintf(log,sizeof(log),"%s/vrmod/logs/bootstrap/runtime.jsonl",argv[1]);
 check(size(log)<0,"fresh output required");
 mbstowcs((wchar_t *)gf2vr_paths.controller_input,path,1024);mbstowcs((wchar_t *)gf2vr_paths.log,log,1024);mbstowcs((wchar_t *)gf2vr_paths.root,argv[1],1024);gf2vr_paths.ready=1;
 char base[1000];snprintf(base,sizeof(base),"%s/vrmod",argv[1]);wchar_t wide[1000];mbstowcs(wide,base,1000);CreateDirectoryW((unsigned short *)wide,0);
 const char *valid[]={"{}","{\"schemaVersion\":1}","{\"diagnostics\":{}}","{\"diagnostics\":{\"loggingEnabled\":false}}","{\"schemaVersion\":1,\"diagnostics\":{\"loggingEnabled\":true},\"auxiliaryPanel\":{\"rotationPitch\":-67}}","\xef\xbb\xbf{\"diagnostics\":{\"loggingEnabled\":true}}"};
 for(unsigned i=0;i<sizeof(valid)/sizeof(*valid);++i){unsigned value=99;check(runtime_logging_config_parse(valid[i],(unsigned)strlen(valid[i]),&value),"valid parse");check(value==(i>=4),"missing/OFF/ON contract");}
 const char *bad[]={"", "null", "[]", "{\"schemaVersion\":2}","{\"schemaVersion\":10}","{\"diagnostics\":null}","{\"diagnostics\":{\"loggingEnabled\":null}}","{\"diagnostics\":{\"loggingEnabled\":1}}","{\"diagnostics\":{\"loggingEnabled\":\"true\"}}","{\"diagnostics\":{\"loggingEnabled\":true,\"loggingEnabled\":false}}","{\"diagnostics\":{},\"diagnostics\":{\"loggingEnabled\":true}}","{\"diagnostics\":{\"loggingEnabled\":true,}}","{\"diagnostics\":{\"loggingEnabled\":true}}x"};
 for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);++i){unsigned value=99;check(!runtime_logging_config_parse(bad[i],(unsigned)strlen(bad[i]),&value),"invalid parse rejected");}
 SetLastError(1234);gf2vr_logging_initialize();check(GetLastError()==1234&&!GF2VR_LOGGING_ALLOWED(),"missing config OFF, last error preserved");event("must_not_exist",1,0);check(size(log)<0,"OFF never creates log");
 config(path,valid[4]);gf2vr_logging_initialize();check(GF2VR_LOGGING_ALLOWED(),"actual ON read");event("fixture_on",1,0);long first=size(log);check(first>0,"actual ON writes");event("fixture_append",2,0);check(size(log)>first,"ON appends");long before=size(log);
 config(path,valid[3]);gf2vr_logging_initialize();event("fixture_off",3,0);check(size(log)==before,"OFF preserves existing log without append");
 config(path,bad[6]);gf2vr_logging_initialize();check(!GF2VR_LOGGING_ALLOWED(),"invalid OFF");event("invalid_no_write",4,0);check(size(log)==before,"invalid never writes");
 printf("{\"passed\":true,\"checks\":%u,\"actualFileLoggingOnOff\":true,\"gameLaunches\":0}\n",checks);return 0;
}
