#!/usr/bin/env bash
# Cross-builds Ribbon3DGPU.aex for Windows x64 with MinGW-w64 (e.g. from Linux).
#   tools/build_win_mingw.sh <AE SDK "Examples" dir> [out dir]
# Needs: x86_64-w64-mingw32-g++-posix, x86_64-w64-mingw32-windres, objcopy, cmake, python3.
# GPU back ends (CUDA via embedded PTX, OpenCL via the embedded source) are loaded at run time,
# so no CUDA toolkit or OpenCL SDK is involved.
set -euo pipefail
here="$(cd "$(dirname "$0")/.." && pwd)"
sdk="$(cd "${1:?usage: build_win_mingw.sh <AE SDK Examples dir> [out dir]}" && pwd)"
out="$(mkdir -p "${2:-$here/build-win}" && cd "${2:-$here/build-win}" && pwd)"
CXX="${CXX:-x86_64-w64-mingw32-g++-posix}"
WINDRES="${WINDRES:-x86_64-w64-mingw32-windres}"
OBJCOPY="${OBJCOPY:-x86_64-w64-mingw32-objcopy}"
gen="$out/generated"
obj="$out/obj"
mkdir -p "$gen" "$obj"

inc=(-I"$sdk/Headers" -I"$sdk/Headers/SP" -I"$sdk/Util" -I"$here/src" -I"$here/src/ae" -I"$gen")
defs=(-DMSWindows -DWIN32 -D_WINDOWS -DNOMINMAX -DWIN32_LEAN_AND_MEAN -DRB_HAS_OPENCL -DRB_HAS_CUDA)
flags=(-std=c++17 -O2 -fno-strict-aliasing -Wall -Wno-unknown-pragmas -Wno-multichar)

echo "== kernel source"
cmake -DKERNEL_DIR="$here/src/kernel" -DOUT="$gen/RibbonKernelSource.h" -P "$here/cmake/EmbedKernel.cmake"

echo "== PiPL values from the SDK headers"
"$CXX" "${flags[@]}" "${defs[@]}" "${inc[@]}" -c "$here/tools/pipl_values.cpp" -o "$obj/pipl_values.o"
"$OBJCOPY" -O binary --only-section=.rbpipl "$obj/pipl_values.o" "$gen/pipl_values.bin"
read -r ver of of2 smaj smin < <(python3 -c "import struct,sys; print(*struct.unpack('<5I', open(sys.argv[1],'rb').read()[:20]))" "$gen/pipl_values.bin")
echo "   version=$ver outflags=$of outflags2=$of2 spec=$smaj.$smin"

# Two effects from the same sources: "Ribbon 3D GPU" and, with RB_MULTI_MASK, "Ribbon 3D GPU Multi-Mask".
build_variant() {
    local file="$1" name="$2" match="$3"
    shift 3
    local vdefs=("$@")
    local vobj="$obj/$file"
    mkdir -p "$vobj"
    echo "== $name: PiPL resource"
    python3 "$here/tools/make_pipl.py" --name "$name" --category "Generate" --match-name "$match" \
        --version "$ver" --outflags "$of" --outflags2 "$of2" --spec-major "$smaj" --spec-minor "$smin" \
        --support-url "https://github.com/SqBrain/Scriplets" \
        --bin "$gen/${file}_PiPL.bin" --rc "$gen/${file}_PiPL.rc"
    "$WINDRES" -O coff -i "$gen/${file}_PiPL.rc" -o "$vobj/pipl_res.o"

    echo "== $name: compile"
    local srcs=(src/core/RibbonGeometry.cpp src/core/RibbonPacket.cpp src/core/RibbonCPU.cpp
          src/ae/Ribbon3D.cpp src/ae/Ribbon3D_GPU.cpp src/ae/Ribbon3D_GPULibs.cpp src/ae/Ribbon3D_Nulls.cpp src/ae/Ribbon3D_Presets.cpp)
    local objs=()
    for s in "${srcs[@]}"; do
        local o="$vobj/$(basename "${s%.cpp}").o"
        "$CXX" "${flags[@]}" "${defs[@]}" "${vdefs[@]}" "${inc[@]}" -c "$here/$s" -o "$o"
        objs+=("$o")
    done

    echo "== $name: link"
    "$CXX" -shared -static -static-libgcc -static-libstdc++ -s -o "$out/$file.aex" "${objs[@]}" "$vobj/pipl_res.o" \
        -Wl,--subsystem,windows -Wl,--dynamicbase -Wl,--nxcompat -Wl,--high-entropy-va
    echo "built $out/$file.aex ($(stat -c %s "$out/$file.aex") bytes)"
}

build_variant Ribbon3DGPU "Ribbon 3D GPU" "SQB Ribbon3D GPU"
build_variant Ribbon3DGPU_MultiMask "Ribbon 3D GPU Multi-Mask" "SQB Ribbon3D GPU MultiMask" -DRB_MULTI_MASK
