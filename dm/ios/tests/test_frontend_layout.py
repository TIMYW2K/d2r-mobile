"""Front-end touch sidebar and three-line character list contract."""
import os
from pathlib import Path
import re
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import touch_layout as layout
ribbon = '\n'.join((
 '{ "type": "Panel", "name": "MainMenuButtonRibbon", "children": [',
 '{ "type": "ImageWidget", "name": "Ribbon", "fields": { "rect": { "x": 0, "y": 0 }, "filename": "FrontEnd/HD/Final/FrontEnd_LeftPanel" }, "children": [',
 '{ "type": "ButtonWidget", "name": "Settings", "fields": { "onClickMessage": "PanelManager:OpenPanel:SettingsPanel", } },',
 '{ "type": "ButtonWidget", "name": "Cinematics", "fields": { "rect": { "x": 165, "y": 960 }, "onClickMessage": "FrontEndNavigation:ToCinematics", } },',
 '{ "type": "ButtonWidget", "name": "Exit", "fields": { "onClickMessage": "FrontEndNavigation:ToExit", } },',
 '] }, { "type": "Widget", "name": "MotDSection", "fields": { "rect": { "x": 0, "y": 0 } } } ] }'))
for enabled in ('1','0'):
 os.environ['DM_MOVIE_FFMPEG']=enabled
 out,_=layout._pc_ribbon(ribbon)
 assert '"x": %d, "y": 0' % layout.SAFE_X in out
 assert 'PanelManager:OpenPanel:SettingsPanel' in out
 assert 'FrontEndNavigation:ToExit' not in out and 'TouchQuitApp' in out
 cine=layout._widget_block(out,'Cinematics')
 assert ('99999' in cine)==(enabled=='0')
 assert '99999' in layout._widget_block(out,'MotDSection')
print('frontend sidebar: safe inset, settings, native quit, optional cinematics passed')
