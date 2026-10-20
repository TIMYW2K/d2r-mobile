#!/usr/bin/env python3
"""Resolve the prepared-package release to use for this source commit.

Prefer the release tagged prepared-manual-<HEAD12>; if the commit only changed non-generation files (docs, iOS
sources, codemagic.yaml itself) fall back to the newest prepared-manual-* release -- the pack records the generation
inputs and the build verifies them, so an older pack is still valid as long as those inputs did not change.

Prints three lines: tag, asset URL, sha256 (hex, no "sha256:" prefix)."""
import json, sys, urllib.request

repo = sys.argv[1] if len(sys.argv) > 1 else 'TIMYW2K/d2r-mobile'
short = sys.argv[2] if len(sys.argv) > 2 else ''

req = urllib.request.Request('https://api.github.com/repos/%s/releases?per_page=40' % repo,
                             headers={'User-Agent': 'codemagic'})
rels = json.load(urllib.request.urlopen(req, timeout=60))
cands = [r for r in rels if r['tag_name'].startswith('prepared-manual-') and r['assets']]
exact = [r for r in cands if r['tag_name'] == 'prepared-manual-' + short]
pick = (exact or cands or [None])[0]
if not pick:
    sys.exit('no prepared package release found')
asset = pick['assets'][0]
print(pick['tag_name'])
print(asset['browser_download_url'])
print((asset.get('digest') or '').split(':')[-1])
