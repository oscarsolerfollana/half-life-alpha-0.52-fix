#!/bin/sh
# Builds build/winmm.dll (32-bit) with the w64devkit i686 mingw.
set -e
# W64DEVKIT = w64devkit folder (otherwise whatever gcc is on the PATH is used)
[ -n "$W64DEVKIT" ] && export PATH="$W64DEVKIT/bin:$PATH"
cd "$(dirname "$0")"
mkdir -p build
i686-w64-mingw32-gcc -O2 -Wall -shared -o build/winmm.dll \
    src/loader.c src/hlalpha.c src/winmm_stubs.c src/winmm.def \
    -luser32 -Wl,--enable-stdcall-fixup -static-libgcc
echo "build/winmm.dll built"
