#!/usr/bin/env python3
"""Read-only validation of a regenerated loot gate against its baseline.

Usage: python test_loot_gate_contract.py <baseline-dir> <gate-dir>
Tests offline rewrites only, not execution or on-device behavior.
"""
from collections import Counter
from pathlib import Path
import struct,sys
SITE=0xC0187C
SVC=0x8F
def word(b,off):return struct.unpack_from("<I",b,off)[0]
def branch(b,off):
    w=word(b,off)
    if w & 0xFC000000 != 0x14000000:raise ValueError("expected unconditional B")
    imm=w&0x3ffffff
    if imm&0x2000000:imm-=0x4000000
    return off+4*imm
def gates(path):
    result=[]
    for l in path.read_text(encoding="utf8").splitlines():
        t=l.split("\t")
        if len(t)==4:result.append((int(t[0],0),t[1],int(t[2],0),int(t[3],0)))
    return result
def verify(base,new):
    a=(base/"main.patched.bin").read_bytes()
    b=(new/"main.patched.bin").read_bytes()
    before=gates(base/"main.gates.tsv"); after=gates(new/"main.gates.tsv")
    site=[g for g in after if g[1:]==("svc",SVC,SITE)]
    if len(site)!=1:raise ValueError("missing or duplicate loot gate")
    if any(g[1:]==("svc",SVC,SITE) for g in before):raise ValueError("unexpected baseline loot gate")
    if Counter(g[1:] for g in after)-Counter([("svc",SVC,SITE)]) != Counter(g[1:] for g in before):
        raise ValueError("unrelated gate semantics changed")
    if a[SITE:SITE+4].hex()!="f5e7e097":raise ValueError("baseline draw call changed")
    gate=branch(b,SITE)
    ret=site[0][0]
    if not (gate <= ret < gate+128):raise ValueError("gate return address out of block")
    if word(b,ret-4)!=0xD63F03C0:raise ValueError("gate does not invoke dispatcher")
    if word(b,ret)!=0xA8C17BFD:raise ValueError("gate does not restore frame/link registers")
    if branch(b,ret+4)!=SITE+4:raise ValueError("gate does not resume original loop")
    print("PASS loot gate: unique target, original gate semantics, ABI return and loop continuation")
    print("NOTE: static rewrite validation only; real-device draw still needs verification")
if __name__=="__main__":
    if len(sys.argv)!=3:raise SystemExit(__doc__)
    verify(Path(sys.argv[1]),Path(sys.argv[2]))
