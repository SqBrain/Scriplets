#!/usr/bin/env python3
"""Writes the binary 'PiPL' resource of an After Effects effect (Windows layout) and an .rc file
that embeds it, without PiPLtool.exe. Used by tools/build_win_mingw.sh.

Windows PiPL resource = WORD 1, then the property list: int32 version (0), int32 count, and for
each property: vendor (OSType), key (OSType), int32 id (0), int32 length (unpadded), data padded
to 4 bytes. All integers little-endian; OSTypes are 32-bit values, so their characters appear
reversed in the file ('8BIM' -> "MIB8"), exactly like PiPLtool's output.
"""
import argparse
import struct


def ostype(code: str) -> bytes:
    assert len(code) == 4, code
    return struct.pack("<I", int.from_bytes(code.encode("ascii"), "big"))


def pstring(s: str) -> bytes:
    b = s.encode("ascii")
    assert len(b) < 256
    return bytes([len(b)]) + b


def prop(key: str, data: bytes) -> bytes:
    pad = (-len(data)) % 4
    return ostype("8BIM") + ostype(key) + struct.pack("<ii", 0, len(data)) + data + b"\0" * pad


def build(args) -> bytes:
    props = [
        prop("kind", ostype("eFKT")),                                   # Kind { AEEffect }
        prop("name", pstring(args.name)),                               # Name
        prop("catg", pstring(args.category)),                           # Category
        prop("8664", args.entry.encode("ascii") + b"\0"),               # CodeWin64X86
        prop("ePVR", struct.pack("<hh", 2, 0)),                         # AE_PiPL_Version { 2, 0 }
        prop("eSVR", struct.pack("<hh", args.spec_major, args.spec_minor)),  # AE_Effect_Spec_Version
        prop("eVER", struct.pack("<I", args.version)),                  # AE_Effect_Version
        prop("eINF", struct.pack("<h", 0)),                             # AE_Effect_Info_Flags
        prop("eGLO", struct.pack("<I", args.outflags)),                 # AE_Effect_Global_OutFlags
        prop("eGL2", struct.pack("<I", args.outflags2)),                # AE_Effect_Global_OutFlags_2
        prop("eMNA", pstring(args.match_name)),                         # AE_Effect_Match_Name
        prop("aeRD", struct.pack("<i", 0)),                             # AE_Reserved_Info
    ]
    return struct.pack("<Hii", 1, 0, len(props)) + b"".join(props)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--name", required=True)
    ap.add_argument("--category", required=True)
    ap.add_argument("--match-name", required=True)
    ap.add_argument("--entry", default="EffectMain")
    ap.add_argument("--version", type=int, required=True)
    ap.add_argument("--outflags", type=int, required=True)
    ap.add_argument("--outflags2", type=int, required=True)
    ap.add_argument("--spec-major", type=int, required=True)
    ap.add_argument("--spec-minor", type=int, required=True)
    ap.add_argument("--bin", required=True, help="output binary resource")
    ap.add_argument("--rc", required=True, help="output .rc file")
    a = ap.parse_args()
    with open(a.bin, "wb") as f:
        f.write(build(a))
    with open(a.rc, "w") as f:
        f.write('16000 PiPL "%s"\n' % a.bin.replace("\\", "/"))


if __name__ == "__main__":
    main()
