#ifndef GF2_CONTROLLER_WIN32_H
#define GF2_CONTROLLER_WIN32_H
/* Included after windows.h and controller_input.h. XR worker owns this sink.
   No API calls occur until its owner supplies a specific game HWND, PID and
   the client extent used by the presented panel mapping. Never pick a window
   merely because it happens to be foreground. */
typedef struct {
    HWND (WINAPI *Foreground)(void);
    DWORD (WINAPI *WindowProcess)(HWND,LPDWORD);
    BOOL (WINAPI *ClientRect)(HWND,LPRECT);
    BOOL (WINAPI *ClientScreen)(HWND,LPPOINT);
    BOOL (WINAPI *CursorSet)(int,int);
    BOOL (WINAPI *CursorGet)(LPPOINT);
    SHORT (WINAPI *KeyState)(int);
    UINT (WINAPI *Inject)(UINT,LPINPUT,int);
} ControllerWin32Api;
typedef struct {
    ControllerWin32Api api;
    HWND window;DWORD process;
    unsigned width,height,mouse_owned,escape_owned,moved;
    unsigned x,y;
} ControllerWin32;
static int controller_win32_mouse(ControllerWin32 *s,DWORD flags) {
    INPUT input={0};input.type=INPUT_MOUSE;input.mi.dwFlags=flags;
    return s->api.Inject && s->api.Inject(1,&input,sizeof(input))==1;
}
static int controller_win32_key(ControllerWin32 *s,DWORD flags) {
    INPUT input={0};input.type=INPUT_KEYBOARD;input.ki.wVk=VK_ESCAPE;input.ki.dwFlags=flags;
    return s->api.Inject && s->api.Inject(1,&input,sizeof(input))==1;
}
/* Only releases this sink's successful synthetic presses. Independent single
   events make partial Escape delivery explicit; failed releases retain owner
   flags and block all later fresh events until cleanup succeeds. */
static int controller_win32_cancel(ControllerWin32 *s) {
    if(!s)return 0;
    s->moved=0;
    if(s->mouse_owned && controller_win32_mouse(s,MOUSEEVENTF_LEFTUP))s->mouse_owned=0;
    if(s->escape_owned && controller_win32_key(s,KEYEVENTF_KEYUP))s->escape_owned=0;
    return !s->mouse_owned && !s->escape_owned;
}
static int controller_win32_target(ControllerWin32 *s,unsigned x,unsigned y,POINT *point) {
    DWORD process=0;RECT rect={0};
    if(!s||!point||!s->window||!s->process||!s->width||!s->height||
       s->width>32768||s->height>32768||x>=s->width||y>=s->height||
       !s->api.Foreground||!s->api.WindowProcess||!s->api.ClientRect||!s->api.ClientScreen||
       s->api.Foreground()!=s->window||!s->api.WindowProcess(s->window,&process)||process!=s->process||
       !s->api.ClientRect(s->window,&rect)||rect.left||rect.top||
       rect.right!=(LONG)s->width||rect.bottom!=(LONG)s->height)return 0;
    *point=(POINT){(LONG)x,(LONG)y};
    if(!s->api.ClientScreen(s->window,point)||s->api.Foreground()!=s->window)return 0;
    return 1;
}
static int controller_win32_emit(void *context,unsigned kind,unsigned x,unsigned y) {
    ControllerWin32 *s=(ControllerWin32 *)context;POINT point={0},current={0};
    if(!s)return 0;
    if(kind==CONTROLLER_UP) {
        /* Releases remain allowed after focus/visibility loss; no reposition. */
        s->moved=0;
        if(!s->mouse_owned)return 1;
        if(!controller_win32_mouse(s,MOUSEEVENTF_LEFTUP))return 0;
        s->mouse_owned=0;return 1;
    }
    if(s->escape_owned) {
        if(!controller_win32_key(s,KEYEVENTF_KEYUP))return 0;
        s->escape_owned=0;
    }
    if(!controller_win32_target(s,x,y,&point)) {controller_win32_cancel(s);return 0;}
    if(kind==CONTROLLER_MOVE) {
        s->moved=0;
        /* Do not turn a real user's held mouse button into a synthetic drag. */
        if(!s->api.KeyState||(!s->mouse_owned&&(s->api.KeyState(VK_LBUTTON)&0x8000)))return 0;
        if(!s->api.CursorSet||!s->api.CursorSet(point.x,point.y)||
           !controller_win32_target(s,x,y,&current)||current.x!=point.x||current.y!=point.y)return 0;
        s->moved=1;s->x=x;s->y=y;return 1;
    }
    if(kind==CONTROLLER_DOWN) {
        if(s->mouse_owned)return 1;
        if(!s->moved||s->x!=x||s->y!=y||!s->api.CursorGet||!s->api.CursorGet(&current)||
           current.x!=point.x||current.y!=point.y||!s->api.KeyState||
           (s->api.KeyState(VK_LBUTTON)&0x8000)||s->api.Foreground()!=s->window)return 0;
        if(!controller_win32_mouse(s,MOUSEEVENTF_LEFTDOWN))return 0;
        s->mouse_owned=1;return 1;
    }
    if(kind==CONTROLLER_BACK) {
        if(s->mouse_owned||!s->api.KeyState||(s->api.KeyState(VK_ESCAPE)&0x8000)||
           s->api.Foreground()!=s->window)return 0;
        if(!controller_win32_key(s,0))return 0;
        s->escape_owned=1;
        if(!controller_win32_key(s,KEYEVENTF_KEYUP))return 0;
        s->escape_owned=0;return 1;
    }
    return 0;
}
#endif
