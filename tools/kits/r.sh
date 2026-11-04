#!/bin/zsh
# r.sh KIT SECONDS "events": render with its own engine state folder
D=${DYLIB:-/Users/jenghis/.claude/jobs/c0176e65/tmp/base/engine-0.5.8.dylib}; C=/Users/jenghis/.claude/jobs/c0176e65/tmp/card
n=$(basename $1 .SND); mkdir -p state/$n wav
LXR0_DIR=$PWD/state/$n ./render $D $1 $C wav/$n.wav $2 "$3"
