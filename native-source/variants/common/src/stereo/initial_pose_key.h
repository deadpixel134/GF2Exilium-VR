#ifndef GF2_INITIAL_POSE_KEY_H
#define GF2_INITIAL_POSE_KEY_H
typedef struct {unsigned released,down;} InitialPoseKey;
/* A held key at startup/focus return never confirms a neutral posture. */
static int initial_pose_key_update(InitialPoseKey *key,int focused,int down) {
    if(!focused) {key->released=key->down=0;return 0;}
    if(!down) {key->released=1;key->down=0;return 0;}
    int pressed=key->released && !key->down;key->down=1;return pressed;
}
#endif
