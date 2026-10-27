"""Portrait-only solid backgrounds beneath the native belt potion sprites."""

def portrait_belt_backings(g, width, height, unit):
    if unit <= 0 or len(g['slots']) < 5:
        raise ValueError('invalid belt geometry')
    result = []
    for i in range(4):
        x0, y0, _, _ = g['slots'][i + 1]
        result.append({
            'type': 'RectangleWidget', 'name': 'TouchPortraitBeltBacking%d' % i,
            'fields': {'anchor': {'x': 0.5, 'y': 1},
                'rect': {'x': round((x0 + g['IX0'] - width / 2) / unit + 30000, 1),
                         'y': round((y0 + g['IY0'] - height) / unit, 1),
                         'width': round((g['IX1'] - g['IX0'] + 1) / unit, 1),
                         'height': round((g['IY1'] - g['IY0'] + 1) / unit, 1)},
                'color': [0.025, 0.025, 0.025, 1.0]}})
    return result
