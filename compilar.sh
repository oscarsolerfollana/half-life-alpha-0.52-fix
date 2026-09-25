#!/bin/sh
# Compila build/winmm.dll (32 bits) con el mingw i686 del w64devkit.
set -e
# W64DEVKIT = carpeta del w64devkit (si no, se usa el gcc que haya en el PATH)
[ -n "$W64DEVKIT" ] && export PATH="$W64DEVKIT/bin:$PATH"
cd "$(dirname "$0")"
mkdir -p build
i686-w64-mingw32-gcc -O2 -Wall -shared -o build/winmm.dll \
    src/cargador.c src/hlalpha.c src/winmm_stubs.c src/winmm.def \
    -luser32 -Wl,--enable-stdcall-fixup -static-libgcc
echo "build/winmm.dll compilada"
