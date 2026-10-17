#ifndef GF2_CONTROLLER_PLATFORM_H
#define GF2_CONTROLLER_PLATFORM_H
/* Exact Win64 user32 ABI for the existing no-SDK/no-CRT production TU. */
typedef void *HWND;
typedef DWORD *LPDWORD;
typedef long LONG;
typedef short SHORT;
typedef unsigned UINT;
typedef struct {LONG left,top,right,bottom;} RECT,*LPRECT;
typedef struct {LONG x,y;} POINT,*LPPOINT;
typedef struct {LONG dx,dy;DWORD mouseData,dwFlags,time;U64 extra;} ControllerMouse;
typedef struct {unsigned short wVk,wScan;DWORD dwFlags,time;U64 extra;} ControllerKeyboard;
typedef struct {DWORD type;union {ControllerMouse mi;ControllerKeyboard ki;};} INPUT,*LPINPUT;
_Static_assert(sizeof(INPUT)==40 && sizeof(RECT)==16 && sizeof(POINT)==8,"Win64 input ABI");
#define INPUT_MOUSE 0
#define INPUT_KEYBOARD 1
#define VK_LBUTTON 1
#define VK_ESCAPE 27
#define MOUSEEVENTF_LEFTDOWN 2
#define MOUSEEVENTF_LEFTUP 4
#define KEYEVENTF_KEYUP 2
#include "controller_input.h"
#include "controller_win32.h"
#endif
