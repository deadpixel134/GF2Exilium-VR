#ifndef GF2_CONTROLLER_GAME_RUNTIME_H
#define GF2_CONTROLLER_GAME_RUNTIME_H
#include "controller_game_mode.h"
#include "controller_game_keys_win32.h"
/* The existing auxiliary sampling owner owns mutable policy and key state.
   Main reads only the atomic consumption flag and publishes configured hand.
   Foreign cancellation signals a serial; XR releases before its next output.
   Close runs after the existing frame-owner drain, retaining failed keyups. */
static struct {
    ControllerGameMode mode;ControllerGameKeys keys;ControllerGameFrame frame;
    DWORD thread;U64 serial,cancel_seen,sequence,generation,reference;
    unsigned prepared;
} stereo_game;
static U64 controller_game_cancel_serial;
static ControllerGameSink controller_game_runtime_sink(void){
    return (ControllerGameSink){&stereo_game.keys,controller_game_wasd_emit,controller_game_key_emit};
}
static int controller_game_runtime_busy(void){
    return __atomic_load_n(&controller_game_consumed,__ATOMIC_ACQUIRE)!=0;
}
static unsigned controller_game_drag_consumed(void){
    return controller_game_runtime_busy()?(1u<<(stereo_aux.pointer.hand?3:1)):0;
}
static void controller_game_runtime_publish(void){
    unsigned consumed=controller_game_move_consumed(&stereo_game.mode)||stereo_game.keys.owned;
    __atomic_store_n(&controller_game_consumed,consumed,__ATOMIC_RELEASE);
    static unsigned prior=~0u,reports;
    unsigned flags=consumed|(stereo_game.mode.held<<1)|(stereo_game.keys.owned<<2);
    if(flags!=prior && reports<64){prior=flags;++reports;event("controller_game_mode",flags,GetCurrentThreadId());}
}
static void controller_game_runtime_cancel(void){
    __atomic_add_fetch(&controller_game_cancel_serial,1,__ATOMIC_ACQ_REL);
    __atomic_store_n(&controller_game_consumed,1,__ATOMIC_RELEASE);
    if(stereo_game.thread==GetCurrentThreadId()){
        ControllerGameSink sink=controller_game_runtime_sink();
        (void)controller_game_cancel(&stereo_game.mode,&sink);
        (void)controller_game_keys_cancel(&stereo_game.keys);
        stereo_game.prepared=0;controller_game_runtime_publish();
    }
}
static void controller_game_runtime_open(void){
    /* Prior close must release successfully before another session opens. */
    stereo_game.thread=GetCurrentThreadId();stereo_game.prepared=0;
    ControllerGameSink sink=controller_game_runtime_sink();
    (void)controller_game_cancel(&stereo_game.mode,&sink);
    (void)controller_game_keys_cancel(&stereo_game.keys);
    stereo_game.keys.api=(ControllerGameKeyApi){stereo_controller.sink.api.Foreground,
        stereo_controller.sink.api.WindowProcess,stereo_controller.sink.api.KeyState,stereo_controller.sink.api.Inject};
    if(!stereo_game.keys.owned&&!stereo_game.mode.blocked)stereo_game.mode=(ControllerGameMode){.rearm=1};
    ++stereo_game.serial;stereo_game.generation=stereo_game.reference=0;
    stereo_game.cancel_seen=__atomic_load_n(&controller_game_cancel_serial,__ATOMIC_ACQUIRE);
    controller_game_runtime_publish();
}
static void controller_game_runtime_prepare(int focused,U64 generation,U64 reference){
    if(stereo_game.thread!=GetCurrentThreadId())return;
    U64 cancel=__atomic_load_n(&controller_game_cancel_serial,__ATOMIC_ACQUIRE);
    if(cancel!=stereo_game.cancel_seen){
        ControllerGameSink sink=controller_game_runtime_sink();
        (void)controller_game_cancel(&stereo_game.mode,&sink);stereo_game.cancel_seen=cancel;
    }
    if(stereo_game.generation!=generation || stereo_game.reference!=reference){
        ++stereo_game.serial;stereo_game.generation=generation;stereo_game.reference=reference;
    }
    ControllerGameFrame *f=&stereo_game.frame;*f=(ControllerGameFrame){0};
    f->sequence=++stereo_game.sequence;f->tick_ms=stereo_aux.drag.frame.time_ms;f->epoch=stereo_game.serial;
    /* Existing pointer transport uses 0=RIGHT,1=LEFT; navigation/drag use
       0=LEFT,1=RIGHT. Derive panel role as the opposite pointer role. */
    f->panel_hand=stereo_aux.pointer.hand?1u:0u;
    f->move_hand=__atomic_load_n(&controller_game_move_hand,__ATOMIC_ACQUIRE);
    f->focused=focused && stereo_aux.ready;
    for(unsigned h=0;h<2;++h){
        f->tracked[h]=stereo_aux.drag.frame.tracked[h];
        f->trigger_active[h]=stereo_aux.drag.frame.active[h*2+1];f->trigger[h]=stereo_aux.drag.frame.values[h*2+1];
        f->axis_active[h]=stereo_aux.navigation.frame.active[h];
        f->x[h]=stereo_aux.navigation.frame.x[h];f->y[h]=stereo_aux.navigation.frame.y[h];
        f->face_active[h]=stereo_aux.game.active[h];f->primary[h]=stereo_aux.game.first[h];f->secondary[h]=stereo_aux.game.second[h];
    }
    stereo_game.prepared=1;
    unsigned pending=f->panel_hand<2 && f->focused && f->tracked[f->panel_hand] &&
        f->trigger_active[f->panel_hand] && f->trigger[f->panel_hand]>=.72f;
    __atomic_store_n(&controller_game_consumed,pending||controller_game_move_consumed(&stereo_game.mode)||
        stereo_game.keys.owned,__ATOMIC_RELEASE);
    if(controller_game_runtime_busy())controller_runtime_cancel();
}
static void controller_game_runtime_end(XrResult result){
    if(stereo_game.thread!=GetCurrentThreadId())return;
    DWORD saved=GetLastError();ControllerGameSink sink=controller_game_runtime_sink();
    U64 cancel=__atomic_load_n(&controller_game_cancel_serial,__ATOMIC_ACQUIRE);
    HWND window=__atomic_load_n(&controller_game_window,__ATOMIC_ACQUIRE);
    if(stereo_game.keys.window && stereo_game.keys.window!=window)
        (void)controller_game_cancel(&stereo_game.mode,&sink);
    int target=controller_game_keys_target(&stereo_game.keys,window,GetCurrentProcessId());
    if(result!=XR_SUCCESS || !stereo_game.prepared || cancel!=stereo_game.cancel_seen || !target ||
       !controller_game_key_target(&stereo_game.keys) || stereo_controller.input.pressed ||
       stereo_controller.sink.mouse_owned || stereo_controller.sink.escape_owned){
        (void)controller_game_cancel(&stereo_game.mode,&sink);
        (void)controller_game_keys_cancel(&stereo_game.keys);
    }else{
        (void)controller_game_step(&stereo_game.mode,&stereo_game.frame,GetTickCount64(),&sink);
    }
    stereo_game.cancel_seen=cancel;stereo_game.prepared=0;controller_game_runtime_publish();SetLastError(saved);
}
static int controller_game_runtime_close(void){
    ControllerGameSink sink=controller_game_runtime_sink();
    (void)controller_game_cancel(&stereo_game.mode,&sink);
    (void)controller_game_keys_cancel(&stereo_game.keys);
    if(stereo_game.keys.owned || stereo_game.mode.axis_owned || stereo_game.mode.owned_keys)return 0;
    stereo_game.thread=stereo_game.prepared=0;
    __atomic_store_n(&controller_game_consumed,0,__ATOMIC_RELEASE);return 1;
}
#endif
