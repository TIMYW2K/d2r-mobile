"""现有自定义页中两位置换组开关的跨语言契约。 | Cross-language contract for the two-position switch in the existing Custom page. | Cross-language contract for the two-position switch in the existing Custom page."""
import json
from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(root / 'ios/tools'))
import touch_layout as layout

out, _ = layout._custom_page('"name": "VideoOptionsPanel", "name": "OptionsTable"')
page = json.loads(re.sub(r',\s*([}\]])', r'\1', out))
nodes = {}
def walk(node):
    if isinstance(node, dict):
        if 'name' in node:
            assert node['name'] not in nodes, node['name']
            nodes[node['name']] = node
        for child in node.values():
            walk(child)
    elif isinstance(node, list):
        for child in node:
            walk(child)
walk(page)
for name in ('TouchCustom_touch_swap_above', 'TouchCustom_touch_swap_above_near'):
    assert nodes[name]['type'] == 'ToggleButtonWidget'
    assert nodes[name]['fields']['onClickMessage'].endswith(name)
assert nodes['TouchCustom_touch_swap_above_Text']['fields']['rect']['height'] == 360
for suffix in ('Near', 'Far'):
    assert any(n.get('fields', {}).get('text') == '@dmCustomTouchSwap' + suffix for n in nodes.values())
# 性能模式三选一（task-22）：三个勾选框都在、都是开关、点消息指向自己；三项标题与说明字都在；整行文字框盖住三行高
# Performance mode, one of three (task-22): all three checkboxes exist, are toggles and send their own message; the three
# titles and descriptions are in the page; the row's text box covers three rows.
for name in ('TouchCustom_q3d_smooth', 'TouchCustom_q3d', 'TouchCustom_q3d_force'):
    assert nodes[name]['type'] == 'ToggleButtonWidget', name
    assert nodes[name]['fields']['onClickMessage'].endswith(name), name
assert nodes['TouchCustom_q3d_Text']['fields']['rect']['height'] == 3 * 180, nodes['TouchCustom_q3d_Text']['fields']['rect']
for key in ('@dmCustomQ3dSmooth', '@dmCustomQ3dQuality', '@dmCustomQ3dForce'):
    assert any(n.get('fields', {}).get('text') == key for n in nodes.values()), key
for key in ('@dmCustomQ3dSmoothHint', '@dmCustomQ3dQualityHint', '@dmCustomQ3dForceHint'):
    assert any(n.get('fields', {}).get('text') == key for n in nodes.values()), key
host = (root / 'host/dm_custom.c').read_text()
keys = re.findall(r'"([a-z_0-9]+)"', re.search(r'k_keys\[\].*?=\s*\{(.*?)\}', host, re.S)[1])
page_keys = [key for key, _ in layout._CUSTOM_ALL]
# 宿主键表：lang_en 仍排第一（语言开关，不在自定义页的普通行里），然后与自定义页同序，末尾可以是「不在自定义页」的键
# （T18：show_fps 在游戏设置页的「用户界面」分区勾选框）。
# The host key table: lang_en is still first (language switch, not one of the custom page's ordinary rows), then the custom
# page's keys in the same order, and it may carry extra keys that are not on the custom page at the end (T18: show_fps is the
# checkbox in the game settings page's User Interface section).
assert keys[0] == 'lang_en', keys
assert keys[1:1 + len(page_keys)] == page_keys, (keys, page_keys)
assert keys[1 + len(page_keys):] == ['show_fps'], keys[1 + len(page_keys):]
defaults = [int(x) for x in re.search(r'k_def\[NK\].*?=\s*\{(.*?)\}', host, re.S)[1].split(',')]
assert len(defaults) == len(keys) and defaults[keys.index('touch_swap_above')] == 0
assert 'TouchCustom_touch_swap_above_near' in host
print('PASS touch layout options: generated radios (two-position switch + three-way performance mode), labels, defaults, host order and scrolling row extent')

# ---- T36（65.1 实测）：设置页三行改动 / 拖动高亮 / 角色页标题字体（合成布局文本，不依赖 RomFS） ----
# t36: settings-page three-row change / drag highlight / character-sheet title font (synthetic layout text, no RomFS needed)
def _t36_row(name, body):
    return '{\n  "type": "TableRowWidget", "name": "Row %s",\n  "children": [ %s ]\n}' % (name, body)

_life = _t36_row('ShowLifeOrbText',
                 '{ "type": "TextBoxWidget", "name": "ShowLifeOrbTextText", "fields": { "text": "@OptionsMenuShowLifeOrbText", "tooltipString": "@OptionsMenuShowLifeOrbTextTooltip" } },'
                 '{ "type": "ToggleButtonWidget", "name": "ShowLifeOrbText", "fields": { "onClickMessage": "OptionsPanelMessage:SetShowLifeOrbText" } }')
_mana = _t36_row('ShowManaOrbText',
                 '{ "type": "TextBoxWidget", "name": "ShowManaOrbTextText", "fields": { "text": "@OptionsMenuShowManaOrbText", "tooltipString": "@OptionsMenuShowManaOrbTextTooltip" } },'
                 '{ "type": "ToggleButtonWidget", "name": "ShowManaOrbText", "fields": { "onClickMessage": "OptionsPanelMessage:SetShowManaOrbText" } }')
_clock = _t36_row('ShowClock',
                  '{ "type": "TextBoxWidget", "name": "ShowClockText", "fields": { "text": "@OptionsMenuShowClock" } },'
                  '{ "type": "ToggleButtonWidget", "name": "ShowClock", "fields": { "onClickMessage": "OptionsPanelMessage:SetShowClock" } }')
_ammo = _t36_row('SkillAmmoFeedback',
                 '{ "type": "TextBoxWidget", "name": "SkillAmmoFeedbackText", "fields": { "text": "@SkillAmmoFeedback" } },'
                 '{ "type": "ToggleButtonWidget", "name": "SkillAmmoFeedback", "fields": { "onClickMessage": "OptionsPanelMessage:SetSkillAmmoFeedback" } }')
# 原版顺序：生命球文本、法力球文本（→帧率）、技能弹药数量、显示时钟；变换后应为 生命&法力、技能弹药、帧率、时钟（时钟原样）
_page = '{\n  "children": [\n    ' + _life + ',\n    ' + _mana + ',\n    ' + _ammo + ',\n    ' + _clock + '\n  ]\n}'
out, n = layout._orbs_merge_showfps(lambda s: (s, 0))(_page)
assert n >= 4, n
_i_life = out.index('"name": "Row ShowLifeOrbText"')
_i_ammo = out.index('"name": "Row SkillAmmoFeedback"')
_i_mana = out.index('"name": "Row ShowManaOrbText"')
_i_clock = out.index('"name": "Row ShowClock"')
assert _i_life < _i_ammo < _i_mana, (_i_life, _i_ammo, _i_mana)   # 可见顺序：生命&法力文本 → 技能弹药数量 → 显示帧率
assert _i_clock > _i_mana                                     # 显示时钟行原样留在后面，没有被搬
assert _clock in out                                          # 时钟行内容一字未动
assert '@dmCustomShowOrbTextBoth' in out and '@OptionsMenuSettingShowFrameRate' in out
assert '"name": "TouchCustom_ShowFpsText"' in out and '"name": "TouchCustom_show_fps"' in out
assert '"name": "ShowManaOrbTextText"' not in out and '"name": "ShowManaOrbText"' not in out
assert 'TouchCustom_RowShowFps' not in out and 'Row ShowFrameRate' not in out

_bar = '''
{
  "type": "CarouselWidget", "name": "ItemNameDisplayMode",
  "fields": {
    "rect": "$OptionsCarouselRect",
    "focusIndicatorPadding": "$OptionsCarouselFocusIndicatorPadding",
    "focusIndicatorFilename": "$OptionsCarouselFocusIndicatorFilename",
    "states": [1, 2]
  }
},
{
  "type": "SettingsSliderWidget", "name": "Volume",
  "fields": {
    "focusIndicatorFilename": "$OptionsSliderFocusIndicatorFilename",
    "focusIndicatorOffset": { "x": 0, "y": -4 },
    "value": 1
  }
},
{
  "type": "ToggleButtonWidget", "name": "ShowClock",
  "fields": {
    "focusIndicatorFilename": "$OptionsToggleFocusIndicatorFilename",
    "onClickMessage": "x"
  }
}'''
bout, bn = layout._hide_drag_highlight(lambda s: (s, 0))(_bar)
assert bn >= 4, bn
assert '$OptionsCarouselFocusIndicatorFilename' not in bout and '$OptionsSliderFocusIndicatorFilename' not in bout
assert '$OptionsToggleFocusIndicatorFilename' in bout          # 开关/按钮的焦点框不动
assert '"focusIndicatorOffset"' not in bout

def _t36_title(nm):
    return ('{\n  "type": "TextBoxWidget", "name": "%s",\n  "fields": {\n    "rect": { "x": 0, "y": 0, "width": 308, "height": 86 },\n'
            '    "style": {\n      "fontFace": "Formal",\n      "pointSize": "$SmallPanelFontSize"\n    },\n'
            '    "useAltStyleIfDoesntFit": true,\n    "altStyle": {\n      "fontFace": "Formal",\n      "pointSize": "$SmallPanelFontSize"\n    }\n'
            '  }\n}' % nm)
_stats = '{\n  "children": [\n    ' + _t36_title('AttackDamageTitle') + ',\n    ' + _t36_title('AttackRatingTitle') + ',\n    ' + _t36_title('AttackDamageTitle') + ',\n    ' + _t36_title('AttackRatingTitle') + '\n  ]\n}'
sout, sn = layout._attack_title_font(lambda s: (s, 0))(_stats)
assert sn == 16, sn                      # 4 个标题 × (style + altStyle) × (fontFace 去掉 + pointSize 改) = 16 处
assert 'Formal' not in sout and sout.count('$MediumPanelFontSize') == 8, sout.count('$MediumPanelFontSize')
assert sout.count('"pointSize"') == 8
assert 'AttackDamageTitle' in sout and 'AttackRatingTitle' in sout

# 接线：三份布局都要套上新函数；65.1 的旧插行函数已删；宿主使用新的行名并补发法力球消息
assert 'gameplayoptionspanelhd.json' in layout.PRESENT and 'gameplayoptionspanelhd.json' in layout.PAD_ROOT
assert 'soundoptionspanelhd.json' in layout.PRESENT and 'soundoptionspanelhd.json' in layout.PAD_ROOT
assert 'characterstatspanelhd.json' in layout.PRESENT
assert not hasattr(layout, '_add_showfps_row')
assert 'Row ShowManaOrbText' in host and 'dm_ui_send_queue("OptionsPanelMessage:SetShowManaOrbText")' in host
assert 'Row SkillAmmoFeedback' in host and '显示技能上的弹药数量' in host    # 顺序日志 / 三行齐全判断用 SkillAmmoFeedback，不是时钟
assert '设置页用户界面分区可见开关顺序' in host
tsv = (root / 'i18n/ui_text.tsv').read_text(encoding='utf-8')
assert 'dmCustomShowOrbTextBoth\t' in tsv and 'dmCustomShowOrbTextBothHint\t' in tsv
tab = (root / 'host/dm_i18n_tab.h').read_text(encoding='utf-8')
assert 'dmCustomShowOrbTextBoth' in tab and 'dmCustomShowOrbTextBothHint' in tab
print('PASS touch layout options T36/T37: merged Life & Mana row, SkillAmmoFeedback row order, frames row reused row name, drag highlight, attack-title font, i18n keys and host wiring')
