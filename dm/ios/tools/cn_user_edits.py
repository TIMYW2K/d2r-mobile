"""Apply the user-approved Simplified Chinese terminology edit."""
import hashlib
from annot_strings import Strings

def build(romfs, outdir, repl):
    strings = Strings(romfs, repl)
    old = strings.get('item-modifiers.json', 'ModStr1h')
    if old != '%+d 命中值' or hashlib.sha1(old.encode('utf-8')).hexdigest()[:8] != '338c0036':
        raise SystemExit('cn_user_edits: ModStr1h original text does not match')
    n = strings.set('item-modifiers.json', 'ModStr1h', lambda _: '%+d 准确率')
    if n != 1:
        raise SystemExit('cn_user_edits: expected exactly one ModStr1h')
    return strings.write(outdir)
