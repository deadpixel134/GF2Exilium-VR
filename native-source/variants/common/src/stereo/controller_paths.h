#ifndef GF2VR_CONTROLLER_PATHS_H
#define GF2VR_CONTROLLER_PATHS_H
// Same validated module-relative root as the common VR provider.
static const unsigned short *gf2vr_controller_path(unsigned kind) {
#ifdef GF2VR_PORTABLE_BINDING
    static unsigned short hands[1024], contact[1024], inventory[1024];
    static unsigned ready;
    if(!ready) {
        if(!gf2vr_paths.ready) return 0;
        unsigned n=gf2vr_path_length(gf2vr_paths.root);
        if(!gf2vr_path_join(hands,gf2vr_paths.root,n,L"vrmod\\config\\controller_hands.json") ||
           !gf2vr_path_join(inventory,gf2vr_paths.root,n,GF2VR_HANDS_INVENTORY_TAIL)) return 0;
#ifdef GF2VR_DIRTY_CONTACT
        if(!gf2vr_path_join(contact,gf2vr_paths.root,n,L"vrmod\\config\\touch_interaction.json")) return 0;
#else
        (void)contact;
#endif
        ready=1;
    }
    return kind==0?hands:kind==2?inventory:contact;
#else
    // Unshipped8E derivation witness only. Final products require PORTABLE.
    return kind==0?L"G:\\GF2Exilium\\vrmod\\config\\controller_hands.json":
        kind==2?GF2_TOUCH_INVENTORY_LOG:L"G:\\GF2Exilium\\vrmod\\config\\touch_interaction.json";
#endif
}
#endif
