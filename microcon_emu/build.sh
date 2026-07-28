#!/bin/bash
# Direct compilation script for microcon_emu - Cygwin only

set -e

cd "$(dirname "$0")"

CXX="g++"
CXXFLAGS="-Wall -Wextra -g"
LDFLAGS="-lpthread"

SOURCES=(main.cpp Proxy.cpp Core.cpp CPU.cpp CPU_Instructions.cpp Memory.cpp Clock.cpp EmuPool.cpp X86Emitter.cpp)
TARGET="microcon_emu.exe"

echo "Compiling microcon_emu..."

# Clean old objects
rm -f *.o "$TARGET"

# Compile all sources directly
"$CXX" $CXXFLAGS -o "$TARGET" "${SOURCES[@]}" $LDFLAGS

if [ -f "$TARGET" ]; then
    echo "Build successful: $TARGET"
else
    echo "Build failed"
    exit 1
fi
