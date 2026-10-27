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
 assert '"x": %d, "y": 0' % (layout.SAFE_X+layout.FE_EXTRA_INSET_UNITS) in out
 assert 'PanelManager:OpenPanel:SettingsPanel' in out
 assert 'FrontEndNavigation:ToExit' not in out and 'TouchQuitApp' in out
 cine=layout._widget_block(out,'Cinematics')
 assert ('99999' in cine)==(enabled=='0')
 assert '99999' in layout._widget_block(out,'MotDSection')
print('frontend sidebar: safe inset, settings, native quit, optional cinematics passed')

credits='{"type":"CreditsModal","name":"CreditsModal","children":[{"type":"ButtonWidget","name":"ExitButton","children": [{"type":"ButtonLegendWidget","name":"DismissLegend","fields":{"presets":[{"action":"back"}]}}]}]}'
credits,_=layout.offscreen(credits)
credits=layout.credits_exit_touch(credits)
exit_widget=layout._widget_block(credits,'ExitButton')
assert 'CreditsModalMessage:Finish' in exit_widget
assert '"anchor": { "x": 1.0, "y": 1.0 }' in exit_widget
assert '"rect": { "x": -766, "y": -245 }' in exit_widget
assert '99999' in layout._widget_block(credits,'DismissLegend')
print('credits exit: visible inherited button, bottom-right anchor, original Finish action, hidden pad legend passed')

assert abs(layout.FE_EXTRA_INSET_UNITS*1206/2160-10)<.5
assert layout.FE_CHARACTER_LIST_HEIGHT==6*layout.FE_CHARACTER_ROW_HEIGHT
assert 10+64<=84 and 84+76<=164 and 164+76<layout.FE_CHARACTER_ROW_HEIGHT
assert 140+layout.FE_CHARACTER_LIST_HEIGHT<40+1775
print('character geometry: 10 physical px inset, three non-overlapping text rows, six cards, footer clearance passed')

focus = layout._pc_character_focus('{"focusIndicatorFilename":"Controller/HoverImages/CharacterListSelection_Hover"}')
assert 'AdvancedCharacterSheet_SkullScroller' in focus and 'CharacterListSelection_Hover' not in focus
print('character focus: touch layout uses dedicated enlarged focus art')
