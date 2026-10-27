# Phase 0: recon results

Pin map: `HARDWARE_MAP_MODULE.md`. All findings come from the image, and photos are still pending. Recon tooling is
in `recon/`; the Erica image and everything decompiled from it stay out of git (`.gitignore`).

## Steps 1–3: image, pins, flash
- Base `0x08008000` confirmed. Header size `0x37954` = payload. The reset vector (`0x08017231`) and every handler fall
  inside `[0x08008000, 0x0803F954)`.
- The app uses sectors 1–4. Sectors 5–7 (768 KB on a 1 MB part) are untouched. The app never writes flash.
- **Bootloader caveat:** Erica's open bootloader (`lxr-desktop-bootloader`, which has a `EURORACK_MODULE` build) runs
  `FLASH_If_Erase` from sector 2 with `NbSectors = 10` on every update (`Core/Src/flash_if.c`). Samples stored in
  internal flash are wiped by every firmware flash and have to be reloaded from SD afterwards. Whether the module's own
  bootloader matches this source can't be checked (it isn't in the image).

## Step 4: module behaviour to mirror
- **Triggers:** falling-edge EXTI at the MCU. Voice = fixed per pin, note 60. Velocity = 127 if the accent jack is high,
  else **63**. PHX-D01's module mode assumes 64 (decisions 2026-10-25). Logged, not fixed here.
- **Accents:** digital, sampled at the trigger. The mod matrix reads them live (127|63 × 1/127 → 1.0 | 0.496).
- **CVs:** 12-bit, `1 − adc/4096`, ±5 LSB deadband, one CV refreshed per main-loop pass (5-pass round robin).
- **Mod matrix** (`FUN_0800da88`): 18 slots, runs once per main-loop pass, not on a timer. Sources 0–4 CV, 5–10 the LFO
  outputs, 11–16 accents. Apply (`FUN_0800f5c4`): `(src·2 − 1)·amt/100 + param/127`, sent through the desktop's
  parameter-message path (`0xb0` for params < 128, `0xd6` for 128–255). Morph (`0x12e`) is handled separately.
- **Still open:** the `v/o` target scaling (the float maths sits behind coprocessor ops in the decompile, so it needs
  disassembly). Also the main-loop rate, meaning how many audio blocks run per mod-matrix pass; measure it in Phase 1.
  And the menu pages from the manual.

## Step 5: photos (needed from the user)
Both sides of each board, sharp enough to read markings. In particular:
1. MCU marking: VG = 1 MB, VI = 2 MB. Also LQFP100 vs BGA.
2. DAC chips: expect two CS4344 or similar.
3. The trigger input stage (one jack through to PD9–PD15): transistor and RC values.
4. The CV input stage (op-amp, resistors).
5. Any SWD pads or header, test points, and the traces from PA11/PA12 (MCU pins 70/71 on LQFP100) to the buttons.

## MIDI verdict (provisional)
| Route | Verdict |
|-------|---------|
| USB MIDI | **No.** PA11/PA12 are panel buttons 12 and 13. USB would mean cutting two button traces and moving those buttons elsewhere, plus a jack and isolation. Not worth it. |
| Firmware-only TRS/DIN | **Most promising.** The OP HH trigger is on **PD9**. To check against the datasheet's AF table: PD9 = USART3_RX (AF7). The stage already inverts, which the USART's RXINV handles. The cost is the OP HH trigger while MIDI mode is on. It depends on the trigger stage passing 31.25 kbaud (32 µs bits) cleanly: photo 3. |
| Small mod (optocoupler to spare RX) | Fallback if the trigger stage is too slow. Spare RX candidates depend on the photos (PB11 = USART3_RX is an LED here). |

## Headroom estimate
- **Flash:** 768 KB free in sectors 5–7 on a 1 MB part. LXR02Open's sample area starts at sector 6 (`0x08080000`, 512 KB
  on 1 MB), and 1.5 MB needs a 2 MB part. Moving it to sector 5 would give 768 KB.
- **RAM:** about 330 KB of SRAM1 free; DTCM about 23 KB free.
- **Reverb:** its buffers are fp16 allpass delays of `MAX_ALLPASS_SIZE` 0x800 each (`FxRamBuffer.h`, `Reverb.c`), on
  the order of tens of KB. RAM is not the limit; CPU is. Decide in Phase 3 with the load meter.
