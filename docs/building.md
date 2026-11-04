---
title: Building from source
nav_order: 10
---

# Building from source

## Firmware

You need CMake 3.21+, Ninja, the Arm GNU Toolchain (`arm-none-eabi-gcc`) and Python 3, all on `PATH`. `firmware/README.md` lists install commands for macOS, Windows and Linux.

```sh
cd firmware
cmake --preset release
cmake --build --preset release
```

The update file is `firmware/build/release/LXRE_RMX0_V<version>.img`. Install it as described in [Installing](install.md).

Each RMX-0 feature sits behind a flag in `firmware/Core/Inc/lxr0.h` (`LXR0_MIDI`, `LXR0_REVERB` and so on), and every change to Erica's code is marked `lxr-0` in a comment. The version on the start-up screen and in the file name is `LXR0_VERSION` in the same file.

## Tests

The tests in `tools/engine-tests/` drive the real firmware on a computer: its engine, menus, buttons, knobs, LCD and MIDI input. They load a host build of the firmware made by [PHX-D01](https://github.com/mechanical-dance/PHX-D01) (macOS).

1. Build the host library with PHX-D01, pointing it at this repository's firmware (see PHX-D01's README for its own requirements):

   ```sh
   make VENDOR0=/path/to/this/repo/firmware build/engine-lxr0.dylib
   ```

2. Build and run a test, giving it an empty state folder, a kit file and a folder with `samples/` and `loops/`:

   ```sh
   cc -O1 -o link_menu_test tools/engine-tests/link_menu_test.c
   LXR0_DIR=$(mktemp -d) ./link_menu_test engine-lxr0.dylib KIT.SND card/
   ```

Each test prints `ok` or `FAIL` per check. `tools/samples-host-test/run.sh` tests the sample installer against a simulated flash, and `tools/kits/` holds the scripts used to build and render example kits.

## Design notes

How each feature was designed and tested is in `plans/design-notes.md` in the repository.
