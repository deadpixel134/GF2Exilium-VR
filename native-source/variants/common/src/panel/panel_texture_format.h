#ifndef GF2_PANEL_TEXTURE_FORMAT_H
#define GF2_PANEL_TEXTURE_FORMAT_H
/* OpenXR may expose a typeless resource for either view in a format family. */
static unsigned panel_format(unsigned source) { return source==28||source==29 ? 29 : source==87||source==91 ? 91 : 0; }
static int panel_destination_format(unsigned actual,unsigned requested) {
    return requested==29 ? (actual==27 || actual==28 || actual==29) :
           requested==91 && (actual==87 || actual==90 || actual==91);
}
#endif
