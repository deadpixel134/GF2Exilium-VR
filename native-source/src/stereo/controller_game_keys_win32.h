#ifndef GF2_CONTROLLER_GAME_KEYS_WIN32_H
#define GF2_CONTROLLER_GAME_KEYS_WIN32_H
/* XR input owner; keyboard mapping has no pointer-hit/panel-visibility gate.
   Target HWND/PID is supplied by the existing game-window owner. */
typedef struct {
    HWND (WINAPI *Foreground)(void);
    DWORD (WINAPI *WindowProcess)(HWND,LPDWORD);
    SHORT (WINAPI *KeyState)(int);
    UINT (WINAPI *Inject)(UINT,LPINPUT,int);
} ControllerGameKeyApi;
typedef struct {ControllerGameKeyApi api;HWND window;DWORD process;unsigned owned;} ControllerGameKeys;
static unsigned controller_game_vk(unsigned key){
    switch(key){case CGM_F:return 0x46;case CGM_ESCAPE:return VK_ESCAPE;
        case CGM_W:return 0x57;case CGM_A:return 0x41;case CGM_S:return 0x53;case CGM_D:return 0x44;default:return 0;}
}
static int controller_game_key_target(ControllerGameKeys *s){
    DWORD process=0;
    return s&&s->window&&s->process&&s->api.Foreground&&s->api.WindowProcess&&
        s->api.Foreground()==s->window&&s->api.WindowProcess(s->window,&process)&&process==s->process;
}
static int controller_game_key_emit(void *context,unsigned key,int down){
    ControllerGameKeys *s=context;unsigned vk=controller_game_vk(key);
    if(!s||!vk||!s->api.Inject)return 0;
    if(!down && !(s->owned&key))return 1;
    if(down){
        if(s->owned&key)return 1;
        if(!controller_game_key_target(s)||!s->api.KeyState||(s->api.KeyState((int)vk)&0x8000)||
           !controller_game_key_target(s))return 0;
    }
    INPUT input={0};input.type=INPUT_KEYBOARD;input.ki.wVk=(unsigned short)vk;
    input.ki.dwFlags=down?0:KEYEVENTF_KEYUP;
    if(s->api.Inject(1,&input,sizeof(input))!=1)return 0;
    if(down)s->owned|=key;else s->owned&=~key;
    return 1;
}
static int controller_game_keys_cancel(ControllerGameKeys *s){
    if(!s)return 0;
    for(unsigned key=CGM_F;key<=CGM_D;key<<=1)if(s->owned&key)(void)controller_game_key_emit(s,key,0);
    return !s->owned;
}
/* Per-axis 0.25 press / 0.15 release hysteresis. Opposing directions never
   coexist; release old directions before pressing a new one. Diagonal is two
   independent holds. Failed releases remain owned by the Win32 sink. */
static int controller_game_wasd_emit(void *context,float x,float y){
    ControllerGameKeys *s=context;
    if(!s||!controller_game_axis_valid(x)||!controller_game_axis_valid(y))return 0;
    unsigned want=0;
    if(x<=-.25f || ((s->owned&CGM_A)&&x<-.15f))want|=CGM_A;
    if(x>= .25f || ((s->owned&CGM_D)&&x> .15f))want|=CGM_D;
    if(y>= .25f || ((s->owned&CGM_W)&&y> .15f))want|=CGM_W;
    if(y<=-.25f || ((s->owned&CGM_S)&&y<-.15f))want|=CGM_S;
    for(unsigned k=CGM_W;k<=CGM_D;k<<=1)
        if((s->owned&k)&&!(want&k)&&!controller_game_key_emit(s,k,0))return 0;
    for(unsigned k=CGM_W;k<=CGM_D;k<<=1)
        if((want&k)&&!(s->owned&k)&&!controller_game_key_emit(s,k,1))return 0;
    return 1;
}
static int controller_game_keys_target(ControllerGameKeys *s,HWND window,DWORD process){
    if(!s)return 0;
    if((window!=s->window||process!=s->process)&&!controller_game_keys_cancel(s))return 0;
    s->window=window;s->process=process;return 1;
}
#endif
