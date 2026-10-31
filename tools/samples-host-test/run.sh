#!/bin/sh
# Runs the sample installer and playback on the host against files and a simulated flash.
set -e
SRC="$(cd "$(dirname "$0")" && pwd)"
CORE="$SRC/../../firmware/Core"
W="$(mktemp -d)"; trap 'rm -rf "$W"' EXIT
cd "$W"
python3 "$SRC/mk.py"
cc -std=gnu11 -O1 -g -fsanitize=undefined -Wno-deprecated-declarations -DLXR0_SAMPLE_FLASH_BASE=0x600000000ull \
  -I"$SRC/shim" -I"$CORE/Inc" "$SRC/test.c" "$CORE/Src/lxr0_samples.c" "$CORE/Src/Soundengine/DSPAudio/lxr0_sampleOsc.c" -o t
./t card empty big
