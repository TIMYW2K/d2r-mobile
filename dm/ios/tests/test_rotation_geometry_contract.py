"""Guard the UIKit call chain that previously disabled its own recovery."""
from pathlib import Path
r=Path(__file__).resolve().parents[1]
f=(r/'Sources/front_ios.m').read_text(encoding='utf-8');main=(r/'Sources/main.m').read_text(encoding='utf-8');project=(r/'project.yml').read_text(encoding='utf-8')
assert main.count('return front_ios_supported_orientations();')==2
watch=f[f.index('int posture=dm_orientation_target(&g_orientation_state);'):f.index('static UIInterfaceOrientation lastO')]
assert 'g_orientation_state.session' in watch and 'front_ios_ingame_autorotate()' not in watch
rec=f[f.index('static void dm_orientation_reconcile(const char *why) {'):f.index('// Scene lifecycle (called by SceneDelegate',f.index('static void dm_orientation_reconcile(const char *why) {'))]
assert 'int target=dm_orientation_target' in rec and 'switch (UIDevice' not in rec
assert rec.index('setNeedsUpdateOfSupportedInterfaceOrientations')<rec.index('dispatch_async(dispatch_get_main_queue()')<rec.index('requestGeometryUpdateWithPreferences')
assert 'epoch!=g_orientation_resume_epoch' in rec
assert '#import <CoreMotion/CoreMotion.h>' in f and 'CoreMotion.framework' in project and 'NSMotionUsageDescription' in project
assert 'stopDeviceMotionUpdates' in f and 'g_orientation_session_signal' in f
assert 'g_orientation_entry' not in f and '.enabled' not in rec
print('rotation geometry wiring: independent sensor/session/target, both controller masks, next-loop requests, generation guard and lifecycle passed')
