#ifndef GF2_EYE_TARGET_HOOKS_H
#define GF2_EYE_TARGET_HOOKS_H
#include "eye_target_watch.h"
/* Optional, explicit install on the checked immediate-context owner only.
   Supports the four direct Draw variants and ClearRenderTargetView. Indirect,
   auto, copy/resolve/compute/deferred paths do not gain coverage by inference.
   Module and published watch storage must remain process-lived. */
int eye_target_hooks_install(void *context);
int eye_target_hooks_attach(EyeTargetWatch *);
int eye_target_hooks_detach(EyeTargetWatch *);
int eye_target_hooks_disable(void);
/* Numeric diagnostic only; owner/context checked, no COM call or mutation. */
unsigned eye_target_hooks_observe(void *context,unsigned long long targets[5]);
/* Refresh only on retained owner/context. At most one additional address per
   observed slot; unexpected further variants permanently reject coverage. */
int eye_target_hooks_refresh(void *context);
/* Read-only current dispatch coverage for diagnostics sharing this hook. */
int eye_target_hooks_coverage(void *context);
/* Optional render-thread diagnostic subscriber. The caller's storage remains
   pinned until detach has returned and all already-issued GPU work is drained.
   Direct Draw/Clear only; this does not certify copy/resolve/indirect coverage. */
enum { ETH_NOTE_DRAW=1,ETH_NOTE_CLEAR=2 };
typedef void (*EyeTargetHookNote)(void *owner,void *context,unsigned kind,void *view,unsigned nonempty);
int eye_target_hooks_attach_note(void *context,EyeTargetHookNote note,void *owner);
int eye_target_hooks_detach_note(EyeTargetHookNote note,void *owner);
#endif
