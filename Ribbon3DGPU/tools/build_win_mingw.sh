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

echo "== PiPL resource"
python3 "$here/tools/make_pipl.py" --name "Ribbon 3D GPU" --category "Generate" --match-name "SQB Ribbon3D GPU" \
    --version "$ver" --outflags "$of" --outflags2 "$of2" --spec-major "$smaj" --spec-minor "$smin" \
    --support-url "https://github.com/SqBrain/Scriplets" \
    --bin "$gen/Ribbon3D_PiPL.bin" --rc "$gen/Ribbon3D_PiPL.rc"
"$WINDRES" -O coff -i "$gen/Ribbon3D_PiPL.rc" -o "$obj/pipl_res.o"

echo "== compile"
srcs=(src/core/RibbonGeometry.cpp src/core/RibbonPacket.cpp src/core/RibbonCPU.cpp
      src/ae/Ribbon3D.cpp src/ae/Ribbon3D_GPU.cpp src/ae/Ribbon3D_GPULibs.cpp)
objs=()
for s in "${srcs[@]}"; do
    o="$obj/$(basename "${s%.cpp}").o"
    "$CXX" "${flags[@]}" "${defs[@]}" "${inc[@]}" -c "$here/$s" -o "$o"
    objs+=("$o")
done

echo "== link"
"$CXX" -shared -static -static-libgcc -static-libstdc++ -s -o "$out/Ribbon3DGPU.aex" "${objs[@]}" "$obj/pipl_res.o" \
    -Wl,--subsystem,windows -Wl,--dynamicbase -Wl,--nxcompat -Wl,--high-entropy-va
echo "built $out/Ribbon3DGPU.aex ($(stat -c %s "$out/Ribbon3DGPU.aex") bytes)"
