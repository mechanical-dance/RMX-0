# Phase 1: build, bring-up image, debug access

Status 2026-10-27: flashed by the user. LED chase, button indices (SHIFT 12, SAVE/LOAD 13) and all 7 trigger pins confirmed on hardware; the trigger order matches the image's map, and OP HH is on PD9 (the MIDI candidate pin). Line 1 `F2048`, uptime counts, encoder switch works; CPU meter reads 9 % for the display refresh (the LCD driver busy-waits per character, so this is plausible). **Bring-up passed.**

## 1. Desktop firmware builds unchanged
`firmware/` is an identical copy of `lxr-desktop-app` (vendor a43cd4f). Build (Ninja isn't installed, so use Make):
```
cmake -S firmware -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```
It produces `LXR_V2_V1.7.img`. There's no official desktop image to compare byte for byte. `mkimage.py --magic LXRERIMG` applied to the stock module payload reproduces `LXRE_V2_V1.5_09_11_2023.img` exactly, so the packaging step is proven.

## 2. Bring-up image: `LXRE_LXR0_BRINGUP.img`
Target `lxr0_bringup` (`firmware/bringup/`), output `build/bringup/LXRE_LXR0_BRINGUP.img`, 14.3 KB. Header checked: magic, size, checksum. SP `0x20080000`, reset vector inside the image, vectors at `0x08008000`. It uses the stock module's clock (216 MHz from the 8 MHz HSE) and GPIO setup, plus the desktop LCD driver.

### Flash it (the user does this)
1. Keep a copy of `LXRE_V2_V1.5_09_11_2023.img` on your computer. That's the recovery image.
2. On the microSD card, leave **only one** `LXRE*.img` file: `LXRE_LXR0_BRINGUP.img`. The bootloader picks up any `LXRE*.img`.
3. Power off, insert the card, hold the encoder and power on. Wait until the bootloader finishes.
4. The update erases flash sectors 2 and up. The bootloader (sector 0) isn't touched.

### What to expect
| Check | Pass |
|------|------|
| Display, about 1 s after boot | `lxr-0 bring-up` |
| Line 1 | `F2048 t<seconds> c<n>%`. 2048 = 2 MB part, `t` counts up, `c` is small (a few %) |
| LEDs | All 14 chase in button-index order (the stock scan order, not panel order): top 4, the drum row, then SHIFT (12), SAVE/LOAD (13) |
| Hold a button | Only that button's LED lights; line 2 shows `bNN` with its index (00–13) |
| Gate into a trigger jack | Its digit on line 2 goes to `1`. Digits are raw pin order PD9..PD15, left to right: 1 OP HH, 2 D3, 3 D2, 4 CL HH, 5 CLP/CYM, 6 SNARE, 7 D1 (matches the trigger table in HARDWARE_MAP_MODULE.md) |
| Press the encoder | `e` at the end of line 2 |
| All LEDs on, display blank | `Error_Handler`: clock setup failed. Report this |
| Nothing at all | Re-flash the stock image (below) and report it |

A wrong LED or button index means a mapping error in `docs/HARDWARE_MAP_MODULE.md`. Note which one.

### Recovery
Put only `LXRE_V2_V1.5_09_11_2023.img` on the card, hold the encoder and power on. The bootloader isn't affected by anything this image does.

## 3. CPU meter
`firmware/bringup/cpu_meter.{h,c}`: DWT cycle-counter window accounting, after LXR02Open's `AudioCodecManager.c`. In the bring-up image it only times the display refresh, as a smoke test that `c` reads a plausible number. From Phase 2 on, it wraps the audio render: `cpuMeter_begin()`/`cpuMeter_end()` around the block, `cpuMeter_percent()` from the UI loop, and `cpuMeter_peakCycles()` for the worst block. The budget for a block is `216 MHz × block size / sample rate` cycles.

## 4. XP2 (SWD) continuity checklist, for when the user is ready
Read from the photos: 6-pin header XP2 on the MCU board, silkscreen `RST` … `GND`. The middle pin order isn't readable yet. The MCU is TFBGA100, so its balls can't be probed. Identify the pins against accessible points instead. **Module unpowered, out of the rack**, multimeter in continuity or diode mode:

| Test | Expected | Identifies |
|------|----------|------------|
| Each XP2 pin ↔ any jack sleeve | Beep on one pin | GND |
| Each pin ↔ DA5/DA6 regulator output (3.3 V side, the tab or output leg) | Beep or ~0 Ω on one pin | VDD/3V3 (the ST-Link's VREF goes here) |
| `RST` pin ↔ GND | Not shorted. Often a 100 nF cap, so the reading climbs | NRST |
| The remaining 2 pins, diode mode, black probe on GND, red probe on the pin | ~0.4–0.7 V (MCU ESD diode), not a short | SWDIO / SWCLK. They can't be told apart by meter; just try both orders |
| Any pin ↔ another pin | No beep (apart from GND–GND if doubled) | No bridges |
| Any 6th pin left | Possibly SWO (PB3) — note: PB3 is also button index 11 on this module | — |

Wire the STLINK-V3MINIE (STDC14) with 1.27→2.54 mm jumpers: VCC (VREF) → 3V3, GND → GND, SWDIO, SWCLK, NRST → RST. **The MINIE doesn't power the target**: power the module from the rack.

## 5. ST-Link commands (the user runs these; install first: `brew install open-ocd` (0.12.0))
Connect and check, read-only:
```
openocd -f interface/stlink.cfg -f target/stm32f7x.cfg \
  -c "adapter speed 1000" -c "reset_config srst_only srst_nogate connect_assert_srst" \
  -c init -c "reset halt" -c "stm32f7x options_read 0" -c "flash banks" -c exit
```
`options_read` shows RDP. Level 0 (`0xAA`) = open. If it reports level 1, **stop**: lowering RDP mass-erases the chip, bootloader included.

Back up the full 2 MB flash, bootloader included (read-only):
```
openocd -f interface/stlink.cfg -f target/stm32f7x.cfg -c "adapter speed 1000" \
  -c "reset_config srst_only srst_nogate connect_assert_srst" \
  -c init -c "reset halt" -c "dump_image lxre_full_backup.bin 0x08000000 0x200000" -c "reset run" -c exit
```
Keep `lxre_full_backup.bin` outside git (it holds Erica's closed bootloader and app).

Flash an app image over SWD (later; the SD-card route above works without the ST-Link):
```
openocd -c "set WORKAREASIZE 0x8000" -f interface/stlink.cfg -f target/stm32f7x.cfg \
  -c "adapter speed 1000" -c "reset_config srst_only srst_nogate connect_assert_srst" \
  -c "program build/bringup/lxr0_bringup.elf verify reset exit"
```
These are the same settings as the vendor `lxr_add_openocd_flash_target` (`firmware/cmake/firmware_outputs.cmake`). The work-area size and 1 MHz clock are there because larger values failed mid-programming. The app ELF only touches `0x08008000`+. Never use `flash protect 0 0 0 off` or a bootloader target unless that's the plan.
