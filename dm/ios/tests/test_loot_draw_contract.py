#!/usr/bin/env python3
"""Verify the candidate loot-label draw site in an actual IPA or main image.

Read-only: python test_loot_draw_contract.py <IPA-or-main-image>
This establishes an offline code contract, not a runtime or visual test.
"""
import struct
import sys
import zipfile
from pathlib import Path

TEXT_END = 0x1B786A0
EXPECTED = {
    0xC017A0: "f90f1bf8", # label-manager draw entry
    0xC017BC: "38000094", # rebuild label records
    0xC01854: "680240f9",
    0xC01858: "0001158b", # x0 = label record
    0xC01874: "01900091", # x1 = record + 0x24
    0xC01878: "02500091", # x2 = record + 0x14
    0xC0187C: "f5e7e097", # BL 0x43b850 (one original draw)
    0xC042AC: "08298052", # record stride 0x148
    0xC042B4: "6866089b",
    0xC042B8: "01910091", # x1 = record + 0x24 in builder
    0xC042D0: "c4dbe097", # BL 0x43b1e0 (build text and rect)
    0x43B20C: "3f000039", # clear first byte of output buffer
    0x43B214: "02108052", # name buffer capacity = 128 bytes
    0x43B22C: "85690094", # BL 0x455840 (build name into buffer)
    0x43B850: "ff8301d1", # draw entry
    0x43B908: "e2801094", # draw background
    0x43B954: "77c82094", # draw text
}
def ipa_main(path):
    with zipfile.ZipFile(path) as z:
        data = z.read("Payload/DmIOS.app/Frameworks/Guest.framework/Guest")
    magic, _, _, _, ncmd, _, _, _ = struct.unpack_from("<IiiIIIII", data)
    if magic != 0xFEEDFACF:
        raise ValueError("Guest is not a little-endian 64-bit Mach-O")
    off, sections = 32, {}
    for _ in range(ncmd):
        cmd, size = struct.unpack_from("<II", data, off)
        if cmd == 0x19:
            nsects = struct.unpack_from("<I", data, off + 64)[0]
            so = off + 72
            for _ in range(nsects):
                name = data[so:so+16].split(b"\0")[0].decode()
                seg = data[so+16:so+32].split(b"\0")[0].decode()
                _, length, fileoff = struct.unpack_from("<QQI", data, so+32)
                sections[(seg, name)] = data[fileoff:fileoff+length]
                so += 80
        off += size
    keys = [("__TEXT","__dmmainx0"),("__DMmainW","__img"),("__TEXT","__dmmainx1")]
    if any(k not in sections for k in keys):
        raise ValueError("Guest lacks expected main-image sections")
    return b"".join(sections[k] for k in keys)
def bl_target(offset, word):
    if word & 0xFC000000 != 0x94000000:
        return None
    imm = word & 0x03FFFFFF
    if imm & 0x02000000:
        imm -= 0x04000000
    return offset + imm * 4
def verify(image):
    if len(image) < TEXT_END:
        raise ValueError("main image is shorter than expected")
    for offset, expected in EXPECTED.items():
        actual = image[offset:offset+4].hex()
        if actual != expected:
            raise ValueError("instruction mismatch at %#x: %s != %s" % (offset, actual, expected))
    assert bl_target(0xC017BC, struct.unpack_from("<I", image, 0xC017BC)[0]) == 0xC0189C
    assert bl_target(0xC042D0, struct.unpack_from("<I", image, 0xC042D0)[0]) == 0x43B1E0
    assert bl_target(0x43B22C, struct.unpack_from("<I", image, 0x43B22C)[0]) == 0x455840
    assert bl_target(0xC0187C, struct.unpack_from("<I", image, 0xC0187C)[0]) == 0x43B850
    sites = [off for off in range(0, TEXT_END, 4)
             if bl_target(off, struct.unpack_from("<I", image, off)[0]) == 0x43B850]
    if sites != [0xC0187C]:
        raise ValueError("unexpected loot drawing call sites: %s" % sites)
    print("PASS loot draw contract: builder, record arguments and unique draw call verified")
    print("NOTE: static match only; no on-device rendering, FPS or pickup behavior tested")
if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit(__doc__)
    path = Path(sys.argv[1])
    image = ipa_main(path) if zipfile.is_zipfile(path) else path.read_bytes()
    verify(image)
