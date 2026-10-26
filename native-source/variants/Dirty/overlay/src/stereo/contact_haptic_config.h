#ifndef GF2VR_CONTACT_HAPTIC_CONFIG_H
#define GF2VR_CONTACT_HAPTIC_CONFIG_H
#include "touch2_config.h"
#include "stereo_depth_config.h"
/* Existing controller_hands schema/duplicates/UTF8 policy. Optional numbers
   are preferences for Dirty only; old files retain 15 percent / 30 ms. */
typedef struct {float amplitude;XrDuration duration;} ContactHapticSettings;
static ContactHapticSettings contact_haptic_defaults(void) {
    return (ContactHapticSettings){.15f,30000000};
}
static int contact_haptic_config_parse(const char *text,unsigned length,ContactHapticSettings *out) {
    unsigned enabled;
    if(!out || !touch2_config_parse(text,length,&enabled))return 0;
    Touch2Json j={(const unsigned char *)text,length,0};
    if(length>=3 && j.text[0]==239 && j.text[1]==187 && j.text[2]==191)j.position=3;
    touch2_json_ws(&j);++j.position;touch2_json_ws(&j);
    double strength=15,duration=30;
    while(j.position<j.length && j.text[j.position]!='}') {
        unsigned key=j.position+1;if(!touch2_json_string(&j))return 0;
        touch2_json_ws(&j);++j.position;touch2_json_ws(&j);
        unsigned start=j.position;
        if(!touch2_json_value(&j,1,0))return 0;
        double *field=touch2_json_key_is(&j,key,"contactHapticStrengthPercent")?&strength:
            touch2_json_key_is(&j,key,"contactHapticDurationMs")?&duration:0;
        if(field) {
            WorldDragJson number={text+start,text+j.position,0};
            if(!stereo_depth_number(&number,field) || number.p!=number.end)return 0;
        }
        touch2_json_ws(&j);if(j.text[j.position]=='}')break;++j.position;touch2_json_ws(&j);
    }
    double nanoseconds=duration*1000000.0;
    float amplitude=(float)(strength/100.0);
    /* OpenXR amplitude [0,1] and nonnegative int64 nanoseconds are backend
       constraints, not UI recommendation limits. Zero explicitly disables. */
    if(strength>100 || (strength>0 && amplitude==0) ||
       !__builtin_isfinite(nanoseconds) || nanoseconds>=9223372036854775808.0 ||
       (duration>0 && nanoseconds<1))return 0;
    ContactHapticSettings result={enabled?amplitude:0,enabled?(XrDuration)nanoseconds:0};
    *out=result;return 1;
}
#endif
