"""Apply reviewed Chinese text edits with per-entry source verification."""
import hashlib, json, os
from annot_strings import Strings

def build(romfs, outdir, repl):
    strings = Strings(romfs, repl)
    path = os.path.join(os.path.dirname(__file__), 'cn_user_edits.json')
    with open(path, encoding='utf-8') as f:
        edits = json.load(f)
    for edit in edits:
        name, key = edit['file'], edit['key']
        original = strings.get(name, key)
        if original is None or hashlib.sha1(original.encode('utf-8')).hexdigest() != edit['source_sha1']:
            raise SystemExit('cn_user_edits: source mismatch: %s / %s' % (name, key))
        if strings.set(name, key, lambda _: edit['value']) != 1:
            raise SystemExit('cn_user_edits: expected unique key: %s / %s' % (name, key))
    own_dir = os.path.join(outdir, "cn_user_edits")
    os.makedirs(own_dir, exist_ok=True)
    return strings.write(own_dir)
