from pathlib import Path
import re, subprocess, os
import argparse
p=argparse.ArgumentParser();p.add_argument('--zig',required=True);p.add_argument('--output',required=True);options=p.parse_args()
root=Path(__file__).resolve().parent
src=root/'src/stereo'
out=Path(options.output).resolve()
out.mkdir(parents=True,exist_ok=True)
def function(text,name):
    matches=re.findall(r'^static (?:int|void) '+name+r'\([^\n]*\)\s*\{.*?^\}',text,re.M|re.S)
    if len(matches)!=1: raise RuntimeError(name)
    return matches[0]
for filename,owner,names in [('frame_actual_managed.h','managed_eye_camera.h',['managed_eye_call','managed_eye_verified_dead']),('frame_actual_topology.h','source_topology.h',['source_topology_observe'])]:
    text=(src/owner).read_text(encoding='utf-8-sig')
    (out/filename).write_text('\n'.join(function(text,n) for n in names),encoding='utf-8')
e=(src/'test_activity_ui_frame_eligible.c').read_text(encoding='utf-8-sig')
e=e[:e.index('int main(void)')]
e=e.replace('#define GF2VR_ACTIVITY_PRIMARY_UI\n','#define GF2VR_ACTIVITY_PRIMARY_UI 1\n')
start=e.index('typedef struct {ActivitySourceGroup *source;Binding binding[7],ui_binding;')
end=e.index('static int activity_group_source_valid',start)
e=e[:start]+r'''
typedef struct {Binding binding;Data data;ManagedEyes eyes;unsigned linked,shown,activated,enrolled;}ActivityUiLayer;
typedef struct {ActivitySourceGroup *source;Binding binding[7];Data data[7],*base;ManagedEyes eyes[7],*base_eyes;Member members[8];
 union {ActivityUiLayer ui;struct {Binding ui_binding;Data ui_data;ManagedEyes ui_eyes;unsigned ui_linked,ui_shown,ui_activated,ui_enrolled;};};
 ActivityUiLayer dialogue_ui;unsigned count,ready,busy,retiring,borrowed,activated;U64 generation,frame;}ActivityEyeGroup;
static unsigned activity_ui_layer_count(void){return 2;}
static ActivityUiLayer *activity_ui_layer(ActivityEyeGroup *g,unsigned n){return n?&g->dialogue_ui:&g->ui;}
static void *activity_ui_owner_key(ActivityEyeGroup *g,ActivityUiLayer *l){return l==&g->ui?(void*)g:(void*)l;}
'''+e[end:]
e=e.replace('#include "frame_actual_group.h"','\n'.join(function((src/'activity_eye_group.h').read_text(encoding='utf-8-sig'),n) for n in ['activity_group_borrow','activity_group_cpu_return','activity_group_retire']))

(out/'crewdeck_fixture.h').write_text(e,encoding='utf-8')
env=os.environ.copy()
env['ZIG_GLOBAL_CACHE_DIR']=str(out/'zig-global-cache')
env['ZIG_LOCAL_CACHE_DIR']=str(out/'zig-cache')
exe=out/'dead-retire.exe'
subprocess.run([options.zig,'cc','-target','x86_64-windows-gnu','-std=c11','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-misleading-indentation','-I'+str(out),'-I'+str(src),str(src/'test_activity_ui_dead_retire.c'),'-o',str(exe)],env=env,check=True)
result=subprocess.run([str(exe)],capture_output=True,text=True)
print(result.stdout,end='')
(out/'result.txt').write_text(result.stdout,encoding='utf-8')
raise SystemExit(result.returncode)