/* Freestanding Win32 transport only. No Unity, XR calls or D3D context access.
   The accepted game-main owner exclusively executes the versioned mailbox. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "live_settings_wire.h"
#include "live_settings_defaults.generated.h"
#ifdef LSW_TEST_TRACE
#include <stdio.h>
#define LS_TRACE(stage,value) do{fprintf(stderr,"%s %lu error=%lu\n",stage,(unsigned long)(value),(unsigned long)GetLastError());fflush(stderr);}while(0)
#else
#define LS_TRACE(stage,value) ((void)0)
#endif
LiveSettingsWire live_settings_wire;
static HANDLE live_settings_host;
static HMODULE live_settings_security;
static BOOL (WINAPI *ls_open_token)(HANDLE,DWORD,PHANDLE);
static BOOL (WINAPI *ls_token_info)(HANDLE,TOKEN_INFORMATION_CLASS,LPVOID,DWORD,PDWORD);
static BOOL (WINAPI *ls_equal_sid)(PSID,PSID);

static unsigned short *ls_wcopy(unsigned short *out,const unsigned short *in){while((*out++=*in++)){}return out-1;}
static unsigned short *ls_decimal(unsigned short *out,unsigned v){unsigned short n[16];unsigned count=0;do{n[count++]=(unsigned short)('0'+v%10);v/=10;}while(v);while(count)*out++=n[--count];*out=0;return out;}
#ifdef GF2VR_LIVE_SETTINGS_ENTRY_OBSERVE
static void ls_entry_status(unsigned stage,DWORD error,DWORD pid,DWORD code){
    __atomic_store_n(&live_settings_wire.entry_host_process,((unsigned long long)pid<<32)|code,__ATOMIC_RELEASE);
    __atomic_store_n(&live_settings_wire.entry_host_status,((unsigned long long)error<<32)|stage,__ATOMIC_RELEASE);
}
#endif
static void ls_host_tick(void){
#ifdef GF2VR_LIVE_SETTINGS_ENTRY_OBSERVE
    if(live_settings_host&&WaitForSingleObject(live_settings_host,0)==WAIT_OBJECT_0){
        DWORD code=0;GetExitCodeProcess(live_settings_host,&code);
        ls_entry_status(3,0,GetProcessId(live_settings_host),code);
    }
#endif
    unsigned action=__atomic_exchange_n(&live_settings_wire.open_request,0,__ATOMIC_ACQ_REL);if(!action)return;
    LiveSettingsSurface *s=live_settings_wire.surface;if(!s)return;
    if(live_settings_host){if(WaitForSingleObject(live_settings_host,0)==WAIT_TIMEOUT){__atomic_store_n(&s->input_reset,action,__ATOMIC_RELEASE);__atomic_add_fetch(&s->toggle,1,__ATOMIC_ACQ_REL);return;}CloseHandle(live_settings_host);live_settings_host=0;}
    static unsigned short exe[1400],command[3000];
    unsigned short *p=ls_wcopy(exe,live_settings_wire.root);ls_wcopy(p,L"\\vrmod\\bin\\GF2VR.Configurator\\GF2VR.Configurator.exe");
    p=command;*p++='"';p=ls_wcopy(p,exe);p=ls_wcopy(p,L"\" --vr-settings-host --root \"");p=ls_wcopy(p,live_settings_wire.root);
    p=ls_wcopy(p,L"\" --pid ");ls_decimal(p,live_settings_wire.pid);
    STARTUPINFOW startup={0};startup.cb=sizeof(startup);startup.dwFlags=STARTF_USESHOWWINDOW;startup.wShowWindow=SW_HIDE;
    PROCESS_INFORMATION child={0};
    if(CreateProcessW(exe,command,0,0,FALSE,CREATE_NO_WINDOW,0,live_settings_wire.root,&startup,&child)){
        CloseHandle(child.hThread);live_settings_host=child.hProcess;
#ifdef GF2VR_LIVE_SETTINGS_ENTRY_OBSERVE
        ls_entry_status(2,0,child.dwProcessId,0);
    }else{
        ls_entry_status(4,GetLastError(),0,0);
#endif
    }
}
static int ls_same_user(HANDLE pipe){
    ULONG pid=0;DWORD a=0,b=0;
    if(!GetNamedPipeClientProcessId(pipe,&pid)||!ProcessIdToSessionId(pid,&a)||!ProcessIdToSessionId(GetCurrentProcessId(),&b)||a!=b){LS_TRACE("client_session_rejected",pid);return 0;}
    HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid),remote=0,local=0;int ok=0;
    if(process&&ls_open_token(process,TOKEN_QUERY,&remote)&&ls_open_token(GetCurrentProcess(),TOKEN_QUERY,&local)){
        unsigned char remote_user[256],local_user[256];DWORD rn=0,ln=0;
        if(ls_token_info(remote,TokenUser,remote_user,sizeof(remote_user),&rn)&&ls_token_info(local,TokenUser,local_user,sizeof(local_user),&ln))
            ok=ls_equal_sid(((TOKEN_USER *)remote_user)->User.Sid,((TOKEN_USER *)local_user)->User.Sid)!=0;
    }
    LS_TRACE("client_user",ok);
#ifdef GF2VR_LIVE_SETTINGS_ENTRY_OBSERVE
    __atomic_store_n(&live_settings_wire.entry_transport,((unsigned long long)(ok?0:GetLastError())<<32)|(ok?3u:4u),__ATOMIC_RELEASE);
#endif
    if(remote)CloseHandle(remote);if(local)CloseHandle(local);if(process)CloseHandle(process);return ok;
}
static int ls_wait(HANDLE pipe,OVERLAPPED *op,DWORD *done,ULONGLONG deadline){
    for(;;){DWORD result=WaitForSingleObject(op->hEvent,67);ls_host_tick();
        if(result==WAIT_OBJECT_0)return GetOverlappedResult(pipe,op,done,FALSE)!=0;
        if(result!=WAIT_TIMEOUT||GetTickCount64()>=deadline){CancelIoEx(pipe,op);WaitForSingleObject(op->hEvent,INFINITE);return 0;}
    }
}
static int ls_transfer(HANDLE pipe,void *data,DWORD size,int writing){
    OVERLAPPED op={0};op.hEvent=CreateEventW(0,TRUE,FALSE,0);if(!op.hEvent)return 0;
    unsigned char *p=data;int ok=1;ULONGLONG deadline=GetTickCount64()+30000;
    while(size){DWORD done=0;ResetEvent(op.hEvent);
        BOOL started=writing?WriteFile(pipe,p,size,&done,&op):ReadFile(pipe,p,size,&done,&op);
        if(!started&&(GetLastError()!=ERROR_IO_PENDING||!ls_wait(pipe,&op,&done,deadline))){LS_TRACE("transfer_failed",writing);ok=0;break;}
        if(!done||done>size){ok=0;break;}p+=done;size-=done;
    }
    CloseHandle(op.hEvent);return ok;
}
static int ls_seed(void){
    static LiveSettingsDocuments docs;static unsigned short path[1400];LiveSettingsWire *w=&live_settings_wire;
    docs.count=w->dirty?10:9;
    for(unsigned i=0;i<docs.count;++i){
        unsigned short *p=ls_wcopy(path,w->root);p=ls_wcopy(p,L"\\vrmod\\config\\");
        const char *name=live_settings_files[i];while(*name)*p++=(unsigned char)*name++;*p=0;
        HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
        if(file==INVALID_HANDLE_VALUE){
            DWORD error=GetLastError();if(error!=ERROR_FILE_NOT_FOUND&&error!=ERROR_PATH_NOT_FOUND)return 0;
            unsigned n=live_settings_length(live_settings_defaults[i]);docs.files[i].length=n;
            for(unsigned k=0;k<=n;++k)docs.files[i].text[k]=live_settings_defaults[i][k];
        }else{
            DWORD length=0;BOOL ok=ReadFile(file,docs.files[i].text,live_settings_limit(i)+1,&length,0);CloseHandle(file);
            if(!ok||!length||length>live_settings_limit(i))return 0;
            docs.files[i].length=length;docs.files[i].text[length]=0;
        }
    }
    w->command=0;w->request=w->revision=0;w->length=live_settings_documents_encode(w->payload,&docs);
    __atomic_store_n(&w->ready,1,__ATOMIC_RELEASE);
    while(__atomic_load_n(&w->ready,__ATOMIC_ACQUIRE)!=2)Sleep(1);
    int ok=w->status==LSW_OK;__atomic_store_n(&w->ready,0,__ATOMIC_RELEASE);return ok;
}
static DWORD WINAPI ls_worker(void *unused){
    (void)unused;LiveSettingsWire *w=&live_settings_wire;
    live_settings_security=LoadLibraryW(L"advapi32.dll");if(!live_settings_security)return 0;
    ls_open_token=(void *)GetProcAddress(live_settings_security,"OpenProcessToken");
    ls_token_info=(void *)GetProcAddress(live_settings_security,"GetTokenInformation");
    ls_equal_sid=(void *)GetProcAddress(live_settings_security,"EqualSid");
    if(!ls_open_token||!ls_token_info||!ls_equal_sid)return 0;
    unsigned short name[96];unsigned short *n=ls_wcopy(name,L"Local\\GF2VR.SettingsSurface.");ls_decimal(n,w->pid);
    HANDLE mapping=CreateFileMappingW(INVALID_HANDLE_VALUE,0,PAGE_READWRITE,0,sizeof(LiveSettingsSurface),name);
    if(!mapping||GetLastError()==ERROR_ALREADY_EXISTS){if(mapping)CloseHandle(mapping);return 0;}
    LiveSettingsSurface *surface=MapViewOfFile(mapping,FILE_MAP_ALL_ACCESS,0,0,sizeof(LiveSettingsSurface));if(!surface){CloseHandle(mapping);return 0;}
    surface->version=1;surface->pid=w->pid;surface->width=LSS_WIDTH;surface->height=LSS_HEIGHT;surface->stride=LSS_WIDTH*4;
    surface->session=1;surface->instance=w->instance;__atomic_store_n(&surface->magic,0x31534647,__ATOMIC_RELEASE);
    __atomic_store_n(&w->surface,surface,__ATOMIC_RELEASE);
    if(!ls_seed())return 0;
    n=ls_wcopy(name,L"\\\\.\\pipe\\GF2VR.LiveSettings.");ls_decimal(n,w->pid);
    HANDLE pipe=CreateNamedPipeW(name,PIPE_ACCESS_DUPLEX|FILE_FLAG_OVERLAPPED|FILE_FLAG_FIRST_PIPE_INSTANCE,
        PIPE_TYPE_BYTE|PIPE_READMODE_BYTE|PIPE_WAIT|PIPE_REJECT_REMOTE_CLIENTS,1,65536,65536,0,0);
    if(pipe==INVALID_HANDLE_VALUE){
#ifdef GF2VR_LIVE_SETTINGS_ENTRY_OBSERVE
        __atomic_store_n(&w->entry_transport,((unsigned long long)GetLastError()<<32)|1u,__ATOMIC_RELEASE);
#endif
        return 0;
    }
#ifdef GF2VR_LIVE_SETTINGS_ENTRY_OBSERVE
    __atomic_store_n(&w->entry_transport,2,__ATOMIC_RELEASE);
#endif
    for(;;){
        OVERLAPPED connect={0};connect.hEvent=CreateEventW(0,TRUE,FALSE,0);if(!connect.hEvent)break;
        BOOL connected=ConnectNamedPipe(pipe,&connect);DWORD error=connected?ERROR_SUCCESS:GetLastError(),done=0;
        if(!connected&&error==ERROR_IO_PENDING)connected=ls_wait(pipe,&connect,&done,GetTickCount64()+3600000);
        else if(error==ERROR_PIPE_CONNECTED)connected=TRUE;
        CloseHandle(connect.hEvent);
        if(connected&&ls_same_user(pipe))for(;;){
            unsigned char header[32];if(!ls_transfer(pipe,header,sizeof(header),0))break;
            unsigned command=live_settings_get32(header+8),length=live_settings_get32(header+12);
            LS_TRACE("command",command);
            if(live_settings_get32(header)!=LSW_MAGIC||live_settings_get32(header+4)!=LSW_VERSION||command<1||command>5||length>LSW_CAPACITY||!live_settings_get64(header+16))break;
            while(__atomic_load_n(&w->ready,__ATOMIC_ACQUIRE)!=0){ls_host_tick();Sleep(1);}
            if(length&&!ls_transfer(pipe,w->payload,length,0))break;
            w->command=command;w->length=length;w->request=live_settings_get64(header+16);w->revision=live_settings_get64(header+24);
            __atomic_store_n(&w->ready,1,__ATOMIC_RELEASE);
            while(__atomic_load_n(&w->ready,__ATOMIC_ACQUIRE)!=2){ls_host_tick();Sleep(1);}
            live_settings_put32(header+8,w->status);live_settings_put32(header+12,w->length);live_settings_put64(header+24,w->revision);
            LS_TRACE("reply_length",w->length);
            int sent=ls_transfer(pipe,header,sizeof(header),1)&&(!w->length||ls_transfer(pipe,w->payload,w->length,1));
            __atomic_store_n(&w->ready,0,__ATOMIC_RELEASE);if(!sent)break;
        }
        DisconnectNamedPipe(pipe);ls_host_tick();
    }
    CloseHandle(pipe);return 0;
}
void live_settings_service_start(const unsigned short *root,const char *build,unsigned dirty){
    LiveSettingsWire *w=&live_settings_wire;unsigned expected=0;
    if(!root||!build||!__atomic_compare_exchange_n(&w->started,&expected,1,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE))return;
    unsigned n=0;while(root[n]&&n<1023){w->root[n]=root[n];++n;}if(root[n])return;w->root[n]=0;
    n=0;while(build[n]&&n<127){w->build[n]=build[n];++n;}if(build[n])return;w->build[n]=0;
    if(!WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,root,-1,w->root_utf8,sizeof(w->root_utf8),0,0))return;
    LARGE_INTEGER qpc={0};QueryPerformanceCounter(&qpc);w->instance=(unsigned long long)qpc.QuadPart;if(!w->instance)w->instance=1;
    w->pid=GetCurrentProcessId();w->dirty=dirty;
    HANDLE worker=CreateThread(0,0,ls_worker,0,0,0);if(worker)CloseHandle(worker);
}
