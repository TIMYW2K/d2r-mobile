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

# ---- T36/T37/T42（65.1 / 67.1 / 70.1 实测）：设置页行布局 / 拖动高亮 / 角色页标题字体（合成布局文本，不依赖 RomFS） ----
# t36/t37/t41: settings-page row layout / drag highlight / character-sheet title font (synthetic layout text, no RomFS needed)
def _t41_row(name, body):
    return '{\n  "type": "TableRowWidget", "name": "Row %s",\n  "children": [ %s ]\n}' % (name, body)

_life = _t41_row('ShowLifeOrbText',
                 '{ "type": "TextBoxWidget", "name": "ShowLifeOrbTextText", "fields": { "text": "@OptionsMenuShowLifeOrbText", "tooltipString": "@OptionsMenuShowLifeOrbTextTooltip" } },'
                 '{ "type": "ToggleButtonWidget", "name": "ShowLifeOrbText", "fields": { "rect": "$OptionsToggleRect", "onClickMessage": "OptionsPanelMessage:SetShowLifeOrbText" } }')
_mana = _t41_row('ShowManaOrbText',
                 '{ "type": "TextBoxWidget", "name": "ShowManaOrbTextText", "fields": { "text": "@OptionsMenuShowManaOrbText", "tooltipString": "@OptionsMenuShowManaOrbTextTooltip" } },'
                 '{ "type": "ToggleButtonWidget", "name": "ShowManaOrbText", "fields": { "rect": "$OptionsToggleRect", "onClickMessage": "OptionsPanelMessage:SetShowManaOrbText" } }')
_ammo = _t41_row('SkillAmmoFeedback',
                 '{ "type": "TextBoxWidget", "name": "SkillAmmoFeedbackText", "fields": { "text": "@SkillAmmoFeedback" } },'
                 '{ "type": "ToggleButtonWidget", "name": "SkillAmmoFeedback", "fields": { "rect": "$OptionsToggleRect", "onClickMessage": "OptionsPanelMessage:SetSkillAmmoFeedback" } }')
_clock = _t41_row('ShowClock',
                  '{ "type": "TextBoxWidget", "name": "ShowClockText", "fields": { "text": "@OptionsMenuShowClock" } },'
                  '{ "type": "ToggleButtonWidget", "name": "ShowClock", "fields": { "rect": "$OptionsToggleRect", "onClickMessage": "OptionsPanelMessage:SetShowClock" } }')
_scroll = '{ "type": "ImageWidget", "name": "ScrollBarBackground", "fields": { "rect": "$OptionsScrollBarBackgroundRect" } }'
_page = '{\n  "children": [\n    ' + _life + ',\n    ' + _mana + ',\n    ' + _ammo + ',\n    ' + _clock + ',\n    ' + _scroll + '\n  ]\n}'
out, n = layout._settings_rows_showfps(lambda s: (s, 0))(_page)
assert n >= 4, n
# 生命球文本行整行删掉；可见顺序 = 技能弹药数量 → 显示帧率（时钟原样留在后面）
assert '"name": "Row ShowLifeOrbText"' not in out
_i_ammo = out.index('"name": "Row SkillAmmoFeedback"')
_i_fps = out.index('"name": "Row ShowManaOrbText"')
_i_clock = out.index('"name": "Row ShowClock"')
assert _i_ammo < _i_fps < _i_clock, (_i_ammo, _i_fps, _i_clock)
# 方案 C 帧率行：文字框保留原名、text 换显示帧率；原版 ShowManaOrbText 开关本身就是可见勾选框（2 个子控件）
assert '"name": "ShowManaOrbTextText"' in out and '"text": "@OptionsMenuSettingShowFrameRate"' in out
assert '@dmCustomShowOrbTextBoth' not in out and 'TouchCustom_ShowFpsText' not in out
assert out.count('"x": -99999') == 2, out.count('"x": -99999')
assert '"onClickMessage": "OptionsPanelMessage:SetShowLifeOrbText"' in out
assert '"onClickMessage": "OptionsPanelMessage:SetShowManaOrbText"' in out
assert '"onClickMessage": "PanelManager:ClosePanel:TouchCustom_show_fps"' in out
assert '"name": "TouchCustom_show_fps"' not in out
_ra, _rb = layout._row_span(out, 'ShowManaOrbText')
_fps_row = out[_ra:_rb]
assert _fps_row.count('"type":') == 3, _fps_row          # 行自身 + 文字框 + 开关
assert '"name": "ShowManaOrbText"' in _fps_row and '"name": "ShowLifeOrbText"' not in _fps_row
# 两颗原版开关作屏外隐藏锚，放在面板根最前面（面板按名字查找先找到锚）
assert out.index('"name": "ShowManaOrbText"') < out.index('"name": "ShowLifeOrbText"') < out.index('"name": "ScrollBarBackground"')

# 拖动条状高亮：只删 Carousel / Slider 的 focusIndicator 字段，开关的焦点框保留
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

def _t41_title(nm):
    return ('{\n  "type": "TextBoxWidget", "name": "%s",\n  "fields": {\n    "rect": { "x": 0, "y": 0, "width": 308, "height": 86 },\n'
            '    "style": {\n      "fontFace": "Formal",\n      "pointSize": "$SmallPanelFontSize"\n    },\n'
            '    "useAltStyleIfDoesntFit": true,\n    "altStyle": {\n      "fontFace": "Formal",\n      "pointSize": "$SmallPanelFontSize"\n    }\n'
            '  }\n}' % nm)
_stats = '{\n  "children": [\n    ' + _t41_title('AttackDamageTitle') + ',\n    ' + _t41_title('AttackRatingTitle') + ',\n    ' + _t41_title('AttackDamageTitle') + ',\n    ' + _t41_title('AttackRatingTitle') + '\n  ]\n}'
sout, sn = layout._attack_title_font(lambda s: (s, 0))(_stats)
assert sn == 16, sn                      # 4 个标题 × (style + altStyle) × (fontFace 去掉 + pointSize 改) = 16 处
assert 'Formal' not in sout and sout.count('$MediumPanelFontSize') == 8, sout.count('$MediumPanelFontSize')
assert sout.count('"pointSize"') == 8
assert 'AttackDamageTitle' in sout and 'AttackRatingTitle' in sout

# 接线：三份布局套新函数；旧合并函数已删；宿主有隐藏锚读状态 + 发消息 + FocusIndicator 隐藏
assert 'gameplayoptionspanelhd.json' in layout.PRESENT and 'gameplayoptionspanelhd.json' in layout.PAD_ROOT
assert 'soundoptionspanelhd.json' in layout.PRESENT and 'soundoptionspanelhd.json' in layout.PAD_ROOT
assert 'characterstatspanelhd.json' in layout.PRESENT
assert not hasattr(layout, '_orbs_merge_showfps') and hasattr(layout, '_settings_rows_showfps')
assert 'static void fi_show(' in host and 'FocusIndicator' in host and 'FocusManager' in host
assert 'dm_ui_send_queue(fx[i].msg)' in host and 'OptionsPanelMessage:SetShowLifeOrbText' in host and 'OptionsPanelMessage:SetShowManaOrbText' in host
assert 'find_w_off(sp, fx[i].name, 1)' in host          # 只认面板根的屏外锚，别拿到行里同名开关
assert '设置页用户界面分区可见开关顺序：显示技能上的弹药数量 → 显示帧率' in host
scan = (root / 'host/tests/native_write_scan.py').read_text(encoding='utf-8')
assert 'fi_show' in scan, 'native_write_scan 的 GUARDED 里应放行 fi_show'
tsv = (root / 'i18n/ui_text.tsv').read_text(encoding='utf-8')
tab = (root / 'host/dm_i18n_tab.h').read_text(encoding='utf-8')
assert 'dmCustomShowOrbTextBoth' not in tsv and 'dmCustomShowOrbTextBoth' not in tab   # 合并文案已删
print('PASS touch layout options T36/T37/T42: two visible rows (Skill Ammo -> Frame Rate), 2-child row + root anchors, orb text force-on wiring, drag highlight, attack-title font')
