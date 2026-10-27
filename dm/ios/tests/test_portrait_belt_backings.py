"""Portrait potion backgrounds match the slot apertures and start off screen."""
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from hud_belt_backings import portrait_belt_backings

g={'slots':[(1033+i*120,1187,1147+i*120,1296) for i in range(6)],
   'IX0':2,'IX1':107,'IY0':7,'IY1':101}
u=1320/2160
nodes=portrait_belt_backings(g,2868,1320,u)
assert len(nodes)==4
for i,n in enumerate(nodes):
    assert n['type']=='RectangleWidget' and n['name']==f'TouchPortraitBeltBacking{i}'
    assert n['fields']['color']==[0.025,0.025,0.025,1.0]
    rect=n['fields']['rect']; x0,y0,_,_=g['slots'][i+1]
    assert abs((rect['x']-30000)*u+2868/2-(x0+g['IX0']))<0.1
    assert abs(rect['y']*u+1320-(y0+g['IY0']))<0.1
    assert abs(rect['width']*u-106)<0.1 and abs(rect['height']*u-95)<0.1
    assert rect['x']>29000
try: portrait_belt_backings(g,2868,1320,0)
except ValueError: pass
else: raise AssertionError('zero scale must fail')
print('Portrait belt backings: opaque RectangleWidget, aperture geometry, off-screen default passed')
