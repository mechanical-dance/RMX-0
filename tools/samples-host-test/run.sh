#!/bin/sh
# Runs the sample installer and playback on the host against a simulated flash:
# test.c with a stand-in file system, ff_test.c with the firmware's real FatFs and its ffconf.h.
set -e
SRC="$(cd "$(dirname "$0")" && pwd)"
FW="$SRC/../../firmware"
CORE="$FW/Core"
W="$(mktemp -d)"; trap 'rm -rf "$W"' EXIT
cd "$W"
python3 "$SRC/mk.py"
FLAGS="-std=gnu11 -O1 -g -w -fsanitize=undefined -DLXR0_SAMPLE_FLASH_BASE=0x600000000ull"
cc $FLAGS -I"$SRC/shim" -I"$CORE/Inc" "$SRC/test.c" "$CORE/Src/lxr0_samples.c" "$CORE/Src/Soundengine/DSPAudio/lxr0_sampleOsc.c" -o t
./t card empty big
cc $FLAGS -I"$SRC/shim-real-fatfs" -I"$SRC/shim" -I"$FW/FATFS/Target" -I"$FW/Middlewares/Third_Party/FatFs/src" -I"$CORE/Inc" \
  "$SRC/ff_test.c" "$FW/Middlewares/Third_Party/FatFs/src/ff.c" "$CORE/Src/lxr0_samples.c" -o f
cd card; A=""; for x in samples/* loops/*; do A="$A $x=$x"; done
"$W/f" $A | tee "$W/out"
grep -q "installed 5" "$W/out"
echo "REAL FATFS PASS"
