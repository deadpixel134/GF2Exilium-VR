#ifndef GF2VR_PORTABLE_PATHS_H
#define GF2VR_PORTABLE_PATHS_H
/* Immutable, bounded Windows UTF-16 paths. The conventional GF2 Game directory
   keeps its parent root; other layouts bind resources beside the executable.
   No working-directory, environment or developer-root lookup. */
#define GF2VR_PATH_CAPACITY 1024u
typedef struct {
    unsigned ready;
    unsigned short game[GF2VR_PATH_CAPACITY];
    unsigned short root[GF2VR_PATH_CAPACITY];
    unsigned short loader[GF2VR_PATH_CAPACITY];
    unsigned short log[GF2VR_PATH_CAPACITY];
    unsigned short render[GF2VR_PATH_CAPACITY];
    unsigned short performance[GF2VR_PATH_CAPACITY];
    unsigned short depth[GF2VR_PATH_CAPACITY];
    unsigned short navigation[GF2VR_PATH_CAPACITY];
    unsigned short drag[GF2VR_PATH_CAPACITY];
    unsigned short activity_ui[GF2VR_PATH_CAPACITY];
    unsigned short controller_input[GF2VR_PATH_CAPACITY];
    unsigned short world_scale[GF2VR_PATH_CAPACITY];
} Gf2vrPaths;

__declspec(dllimport) int __stdcall CompareStringOrdinal(
    const unsigned short *, int, const unsigned short *, int, int);
static unsigned gf2vr_path_length(const unsigned short *s) {
    if (!s) return GF2VR_PATH_CAPACITY;
    unsigned n=0; while(n<GF2VR_PATH_CAPACITY && s[n]) ++n; return n;
}
static int gf2vr_path_equal(const unsigned short *a,unsigned an,
                            const unsigned short *b,unsigned bn) {
    return an==bn && CompareStringOrdinal(a,(int)an,b,(int)bn,1)==2;
}
static int gf2vr_path_absolute(const unsigned short *s,unsigned n) {
    /* Native queries return backslashes. Accept drive and UNC/extended roots;
       reject device namespaces and relative forms rather than guessing CWD. */
    if(n>=3 && ((s[0]>='A'&&s[0]<='Z')||(s[0]>='a'&&s[0]<='z')) &&
       s[1]==':' && s[2]=='\\') return 1;
    if(n<5 || s[0]!='\\' || s[1]!='\\' || s[2]=='.') return 0;
    if(s[2]=='?') {
        if(s[3]!='\\') return 0;
        if(n>=7 && ((s[4]>='A'&&s[4]<='Z')||(s[4]>='a'&&s[4]<='z')) &&
           s[5]==':' && s[6]=='\\') return 1;
        if(n<10 || !gf2vr_path_equal(s+4,3,L"UNC",3) || s[7]!='\\') return 0;
    }
    unsigned start=s[2]=='?'?8:2, split=start;
    while(split<n && s[split]!='\\') ++split;
    if(split==start || split>=n-1) return 0;
    unsigned end=split+1; while(end<n && s[end]!='\\') ++end;
    return end>split+1 && end<n;
}
static int gf2vr_path_components(const unsigned short *s,unsigned n) {
    /* GetModuleFileName may preserve the spelling used to load a DLL. Reject
       unnormalized dot/empty components instead of interpreting them as root. */
    for(unsigned i=0;i<n;++i) if(s[i]=='/') return 0;
    unsigned start=0;
    for(unsigned i=0;i<=n;++i) if(i==n || s[i]=='\\') {
        unsigned size=i-start;
        if((size==1 && s[start]=='.') ||
           (size==2 && s[start]=='.' && s[start+1]=='.')) return 0;
        if(!size && i>1 && !(i==3 && s[2]=='?')) return 0;
        start=i+1;
    }
    return 1;
}
static int gf2vr_path_join(unsigned short *out,const unsigned short *root,
                           unsigned rn,const unsigned short *tail) {
    unsigned tn=gf2vr_path_length(tail);
    if(!rn || tn>=GF2VR_PATH_CAPACITY || rn+1+tn>=GF2VR_PATH_CAPACITY) return 0;
    for(unsigned i=0;i<rn;++i) out[i]=root[i];
    /* A drive root already ends in a separator. */
    unsigned p=rn; if(root[rn-1]!='\\') out[p++]='\\';
    for(unsigned i=0;i<=tn;++i) out[p+i]=tail[i];
    return 1;
}
static int gf2vr_paths_build(Gf2vrPaths *p,const unsigned short *module,
                             const unsigned short *exe,const unsigned short *log_name) {
    p->ready=0;
    unsigned mn=gf2vr_path_length(module), en=gf2vr_path_length(exe);
    if(mn>=GF2VR_PATH_CAPACITY || en>=GF2VR_PATH_CAPACITY ||
       !gf2vr_path_absolute(module,mn) || !gf2vr_path_absolute(exe,en) ||
       !gf2vr_path_components(module,mn) || !gf2vr_path_components(exe,en)) return 0;
    unsigned md=mn,ed=en;
    while(md && module[md-1]!='\\') --md;
    while(ed && exe[ed-1]!='\\') --ed;
    if(!md || !ed || !gf2vr_path_equal(module+md,mn-md,L"version.dll",11) ||
       !gf2vr_path_equal(exe+ed,en-ed,L"GF2_Exilium.exe",15) ||
       !gf2vr_path_equal(module,md,exe,ed)) return 0;
    unsigned directory=ed-1,begin=directory;
    while(begin && exe[begin-1]!='\\') --begin;
    unsigned share_begin=0;
    if(exe[0]=='\\' && exe[1]=='\\' &&
       (exe[2]!='?' || gf2vr_path_equal(exe+4,3,L"UNC",3))) {
        share_begin=exe[2]=='?'?8:2;
        while(share_begin<directory && exe[share_begin]!='\\') ++share_begin;
        ++share_begin;
    }
    /* Only the conventional directory is a nested game layout. Choosing the
       parent of every renamed directory would put vrmod in a shared library. */
    unsigned rn=directory;
    /* A UNC share named GF2 Game has no filesystem parent: stay in that share. */
    if(begin && begin!=share_begin &&
       gf2vr_path_equal(exe+begin,directory-begin,L"GF2 Game",8)) rn=begin-1;
    /* Keep the separator in X:\ and extended drive roots. */
    if(rn && exe[rn-1]==':') ++rn;
    if(!rn) return 0;
    for(unsigned i=0;i<rn;++i) p->root[i]=exe[i]; p->root[rn]=0;
    for(unsigned i=0;i<=en;++i) p->game[i]=exe[i];
    if(!gf2vr_path_join(p->loader,p->root,rn,L"vrmod\\runtime\\openxr_loader.dll") ||
       !gf2vr_path_join(p->log,p->root,rn,log_name) ||
       !gf2vr_path_join(p->render,p->root,rn,L"vrmod\\config\\render_quality.json") ||
       !gf2vr_path_join(p->performance,p->root,rn,L"vrmod\\config\\performance.json") ||
       !gf2vr_path_join(p->depth,p->root,rn,L"vrmod\\config\\stereo_depth.json") ||
       !gf2vr_path_join(p->navigation,p->root,rn,L"vrmod\\config\\navigation.json") ||
       !gf2vr_path_join(p->drag,p->root,rn,L"vrmod\\config\\world_drag.json") ||
       !gf2vr_path_join(p->activity_ui,p->root,rn,L"vrmod\\config\\activity_ui.json") ||
       !gf2vr_path_join(p->controller_input,p->root,rn,L"vrmod\\config\\controller_input.json") ||
       !gf2vr_path_join(p->world_scale,p->root,rn,L"vrmod\\config\\world_scale.json")) return 0;
    p->ready=1; return 1;
}
#endif
