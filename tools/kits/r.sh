#!/bin/zsh
# r.sh KIT SECONDS "events": render with its own engine state folder
D=${DYLIB:?set DYLIB to a host build of the engine (engine-lxr0.dylib)}; C=${CARD:?set CARD to a folder with samples/ and loops/}
n=$(basename $1 .SND); mkdir -p state/$n wav
LXR0_DIR=$PWD/state/$n ./render $D $1 $C wav/$n.wav $2 "$3"
