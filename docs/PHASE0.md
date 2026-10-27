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
- **Target table:** `0x08030380` (payload offset `0x28380`), 4-byte entries (name index, param id). Each voice block
  starts with name `0x71` = `v/o`.
- **`v/o`** (fully traced):
  - **Ids:** param ids 221–226 (`0xdd`–`0xe2`), the desktop's `PAR_UNUSED_1..6`, one per voice. Mod-target entries
    are type 2 (float) at drum `+0x2ac`, snare `+0x158`, cymbal `+0x16c`, hi-hat `+0x178`. Those fields are appended
    after each voice's `pwm` (`FUN_08013404`).
  - **Path:** `FUN_0800f5c4` → `FUN_08011728` `0xd6` switch, cases `0x5d`–`0x62` (decompile about line 9676). From
    the mod matrix (`param_5 = 1`): `freq = FUN_080115c0(value)`, then the per-voice setter writes `freq` to osc,
    mod-osc and transient (`FUN_0800e40c(freq, n)` for drums 1–3, `FUN_0800ee88` snare, `FUN_0800df70` cymbal,
    `FUN_0800e664` hi-hat; offsets `+0x20`/`+0x4c`/…). From an ordinary edit it writes −1.0, meaning `v/o` off.
  - **Oscillator:** the module's `OscInfo` has a new float, −1.0 = off. In `osc_setFreq` (`0x0800eb64`), when it is
    set it replaces `freq`: `phaseInc ∝ vofreq · 2^((coarse−60)/12) · fine · pitchMod · modNodeValue`. So coarse 60
    plays the CV pitch unchanged and coarse/fine still transpose.
  - **Conversion** (`FUN_080115c0`): `v = (src·2 − 1)·amt + param/127`, which is `V/5` at amt 100% and base 0.
    1. If `|v − last| < 0.001`, return the cached frequency.
    2. `v += 0.005556` (rounds the truncation).
    3. Piecewise-linear lookup in a 6-point calibration table `cal` at `0x20020134` (in `.data`; defaults at flash
       `0x08033a44`): `[0.0039, 0.2031, 0.4033, 0.6011, 0.7983, 0.9824]`, one point per volt from 0 to 5 V.
    4. `semi = trunc(12k + (v − cal[k]) / ((cal[k+1] − cal[k]) / 12))`.
    5. `freq = 440 · 2^((semi − 57)/12)` if `semi < 120`, else 0.
    So `v/o` is **quantised to semitones**, 0 V = C0 (16.35 Hz) and +5 V = C5. Negative input clamps to C0.
  - **Calibration:** the table is never written anywhere. It's a fixed, apparently measured (uneven steps) factory
    constant, with no user calibration.
  - **To mirror:** these constants and the semitone quantisation. Possible superset ideas (each needs the user's
    sign-off): unquantised `v/o`, and a per-unit calibration menu.
- **ITCM:** `recon/ghidra/MapItcm.java` maps the 0x2DF8-byte ITCM image at `0x00000000` and decompiles it
  (32 functions; audio render `itcm_20c4`).
- **Still open:** the main-loop rate (how many audio blocks per mod-matrix pass; measure in Phase 1) and the menu pages
  from the manual.

## Step 5: photos (done, front panel left on)
In `~/Documents/Gear/lxr-02/photos/`; details in `HARDWARE_MAP_MODULE.md`.
1. **MCU:** STM32F765, line 2 most likely `VIH6` (TFBGA100, 2 MB), not certain. Phase 1 reads FLASHSIZE to settle it.
2. **DACs:** DA7/DA8, 10-pin, markings unreadable; the footprint fits CS4344.
3. **Trigger stage:** transistor + 2 × ~100 kΩ + ferrite + diode per jack, no visible capacitor; open-collector into the
   MCU pull-up.
4. **CV stage:** op-amps DA1/DA3/DA4 on the jack board (values not read).
5. **Debug:** 6-pin XP2 (`RST` … `GND`) on the MCU board, likely SWD; 5 pads on the back. Not probed (photos-only rule).

## MIDI verdict (provisional)
| Route | Verdict |
|-------|---------|
| USB MIDI | **No.** PA11/PA12 are panel buttons 12 and 13. USB would mean cutting two button traces and moving those buttons elsewhere, plus a jack and isolation. Not worth it. |
| Firmware-only (TS jack) | **Most promising.** The OP HH trigger is on **PD9** = USART3_RX (AF7). This is corroborated by the original LXR STM32F4 bootloader's `uart_init` ("UART3 (PD8 TX, PD9 RX)", `LXR02Open/knowledge_files/LXR-master/mainboard/LxrStm32_bootloader/src/uart.c:151`), but not yet checked in the F765 datasheet. The jack is TS: a TRS **Type A** cable puts the MIDI data line (pin 5) on the tip, which works with push-pull MIDI outputs (most current gear), not with outputs that only sink current. The stage already inverts, which the USART's RXINV handles. The cost is the OP HH trigger while MIDI mode is on. Photos: open-collector transistor, ~100 kΩ base resistors, no filter cap. Edges into the internal pull-up should be well under 1 µs against 32 µs bits, so it should pass. **Final check: a hardware test** (switch PD9 to USART3 RX with RXINV and send MIDI). |
| Small mod (optocoupler to spare RX) | Fallback if the trigger stage is too slow. Spare RX candidates depend on the photos (PB11 = USART3_RX is an LED here). |

## Headroom estimate
- **Flash:** if the part is VIH6 (likely), 2 MB: sectors 5–11 free, about 1.75 MB, enough for LXR02Open's 1.5 MB sample area unchanged. If VGH6: 768 KB free in sectors 5–7. LXR02Open's sample area starts at sector 6 (`0x08080000`, 512 KB
  on 1 MB), and 1.5 MB needs a 2 MB part. Moving it to sector 5 would give 768 KB.
- **RAM:** about 330 KB of SRAM1 free; DTCM about 23 KB free.
- **Reverb:** its buffers are fp16 allpass delays of `MAX_ALLPASS_SIZE` 0x800 each (`FxRamBuffer.h`, `Reverb.c`), on
  the order of tens of KB. RAM is not the limit; CPU is. Decide in Phase 3 with the load meter.
