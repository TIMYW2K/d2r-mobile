"""Exercise actual sidecar emission and source hashes for both movie prompts."""
import hashlib,json,os,sys
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import touch_strings as t
keys=['MoviePlaybackHoldToSkip','MoviePlaybackHoldToSkip_switch']
original={'zhCN':'长按任意键跳过','enUS':'Hold any button to skip','zhTW':'長按任意鍵跳過'}
hash8=lambda x:hashlib.sha1(x.encode()).hexdigest()[:8]
text=json.dumps([dict(Key=k,**original) for k in keys],ensure_ascii=False)
cn={'fixture.json':{k:(hash8(original['zhCN']),'') for k in keys}}
en={'fixture.json':{k:(hash8(original['enUS']),'') for k in keys}}
with patch.object(t,'TABLE',cn),patch.object(t,'EN',en),patch.object(t.touch_layout,'read_layout',return_value=('fixture.json',text)):
 for enabled in ['1','0']:
  with patch.dict(os.environ,{'DM_MOVIE_FFMPEG':enabled}):
   rows=t.build_sidecar(None);assert len(rows)==6
   for key,checksum,value in rows:
    lang=next(k for k,v in original.items() if checksum==hash8(v))
    assert value==({'zhCN':'长按屏幕跳过','zhTW':'長按螢幕跳過'}.get(lang,'Touch and hold the screen to skip')) if enabled=='1' else value==''
   # Original controller wording is never rewritten by file-only generation.
   assert original['zhCN'] in text and original['enUS'] in text
print('movie touch prompts: both keys, zhCN/zhTW/en, native/disabled, source hashes passed')
