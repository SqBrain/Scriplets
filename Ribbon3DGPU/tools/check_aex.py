#!/usr/bin/env python3
"""Sanity check of a built Ribbon3DGPU.aex (needs `pip install pefile`):
exports, imported DLLs (only system DLLs allowed: GPU libraries must be loaded at run time),
and the decoded PiPL resource."""
import struct
import sys

import pefile

ALLOWED_IMPORTS = {"kernel32.dll", "msvcrt.dll", "user32.dll", "advapi32.dll", "ucrtbase.dll"}
REQUIRED_EXPORTS = {"EffectMain"}


def decode_pipl(data: bytes):
    word, version, count = struct.unpack_from("<Hii", data, 0)
    off, props = 10, []
    for _ in range(count):
        vendor, key, pid, length = struct.unpack_from("<IIii", data, off)
        off += 16
        payload = data[off:off + length]
        off += length + (-length) % 4
        k = key.to_bytes(4, "big").decode("ascii", "replace")
        props.append((vendor.to_bytes(4, "big").decode("ascii", "replace"), k, pid, payload))
    return word, version, props, off == len(data)


def show(payload: bytes, key: str) -> str:
    if key in ("name", "catg", "eMNA"):
        return repr(payload[1:1 + payload[0]].decode())
    if key == "8664":
        return repr(payload.rstrip(b"\0").decode())
    if key == "kind":
        return repr(struct.unpack("<I", payload)[0].to_bytes(4, "big").decode())
    if len(payload) == 4 and key in ("ePVR", "eSVR"):
        return "%d.%d" % struct.unpack("<hh", payload)
    if len(payload) == 4:
        return "%d (0x%08X)" % (struct.unpack("<I", payload)[0], struct.unpack("<I", payload)[0])
    return payload.hex()


def main(path: str) -> int:
    pe = pefile.PE(path)
    ok = True
    print("machine: 0x%X, DLL: %s" % (pe.FILE_HEADER.Machine, pe.is_dll()))
    exports = {e.name.decode() for e in pe.DIRECTORY_ENTRY_EXPORT.symbols if e.name}
    print("exports:", ", ".join(sorted(exports)))
    if not REQUIRED_EXPORTS <= exports or not ({"PluginDataEntryFunction2", "PluginDataEntryFunction"} & exports):
        print("FAIL: missing EffectMain / PluginDataEntryFunction(2)")
        ok = False
    imports = sorted(i.dll.decode().lower() for i in pe.DIRECTORY_ENTRY_IMPORT)
    print("imports:", ", ".join(imports))
    extra = [d for d in imports if d not in ALLOWED_IMPORTS and not d.startswith("api-ms-win-")]
    if extra:
        print("FAIL: unexpected DLL dependencies:", extra)
        ok = False
    pipl = None
    for t in pe.DIRECTORY_ENTRY_RESOURCE.entries:
        if t.name is not None and str(t.name).upper() == "PIPL":
            for n in t.directory.entries:
                for lang in n.directory.entries:
                    rva, size = lang.data.struct.OffsetToData, lang.data.struct.Size
                    pipl = (n.id, pe.get_data(rva, size))
    if not pipl:
        print("FAIL: no PiPL resource")
        return 1
    rid, data = pipl
    word, version, props, exact = decode_pipl(data)
    print("PiPL resource id %d: header word %d, list version %d, %d properties, size matches: %s" %
          (rid, word, version, len(props), exact))
    for vendor, key, pid, payload in props:
        print("  %s/%s  %s" % (vendor, key, show(payload, key)))
    if rid != 16000 or word != 1 or not exact:
        ok = False
    print("OK" if ok else "FAILED")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1]))
