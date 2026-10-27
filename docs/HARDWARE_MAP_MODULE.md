# LXR-Drum module: hardware map

Recovered from `LXRE_V2_V1.5_09_11_2023.img` (Ghidra 12.2, raw Cortex-M import at `0x08008000`). Addresses are flash
addresses in the image. Function names are Ghidra's (`FUN_<addr>`); `main` is `FUN_0800b940`. Desktop column:
`LXR02Open/knowledge_files/hardware_archive/HARDWARE_MAP.md` and `lxr-desktop-app/Core/Src/main.c`.

Status: from the image only. Nothing here is confirmed on the board yet (photos pending).

## MCU and memory
| Item | Module | Source | Desktop |
|------|--------|--------|---------|
| Core | STM32F76x, SP `0x20080000` (512 KB RAM) | vector 0 | STM32F765 |
| Package | Uses PD8–PD15, so not the desktop's TFBGA100 (PD8–15 not bonded). LQFP100 likely, matching `STM32F765VGTX` in Erica's linker script | GPIO init, `0x800bc58`/`0x800bc60` | TFBGA100 (VIH6) |
| App | `0x08008000`–`0x0803F954` (222 KB), sectors 1–4 | image header | same base |
| Flash writes | None: no FLASH_KEYR unlock keys in the image. Kits live on SD only | byte search | — |
| ITCM | 0x2DF8 bytes copied from `0x0803cb5c` to `0x00000000` at start of `main`; the audio render runs there (`0x000020c4`) | `0x800bc78`–`0x800bc80`, `FUN_0800dc68` | — |
| DTCM | data `0x20000000`–`0x20009088`, bss to `0x2001a418` (105 of 128 KB) | Reset_Handler pool `0x08017290` | — |
| SRAM1 | data `0x20020000`–`0x200201c4`, bss to `0x2002bf78` (48 KB); free up to stack top `0x20080000` (~330 KB) | same | — |

## Audio out (4 jacks = 2 stereo DACs)
| Signal | Pins | Source | Desktop |
|--------|------|--------|---------|
| I2S2 | PB12, PB13, PB15 AF5; MCK PC6 AF5; DMA1 Stream4 | `FUN_0800c41c` | same |
| I2S3 | PA15, PB5, PC10 AF6; MCK PC7 AF6; DMA1 Stream5 | `FUN_0800c41c` | same |
| Rate | 44.1 kHz master TX, MCLK on | `main`, handles `0x20020468`/`0x200204a8` | same |

The audio block is rendered from the main loop: `FUN_0800dc68` polls a half-buffer flag between every task and renders
into `0x200090c8` (L) / `0x200092c8` (R), 0x100 bytes per half. It isn't rendered in an ISR.

## Trigger inputs (7) and accents (6)
Triggers are falling-edge EXTI with pull-ups on PD9–PD15 (`0x800bc60`: pins `0xfe00`, mode `0x210000`), so the input
stage inverts. EXTI15_10 is at NVIC priority 0, EXTI9_5 at 3. Each ISR reads the voice's accent pin at that moment and
calls `FUN_080133dc(voice, 60, accent_low ? 127 : 63)`.

| Trigger pin | EXTI | Voice | Accent pin | Source |
|-------------|------|-------|------------|--------|
| PD15 | 15 | 0 (DRUM1) | PD8 | `vec_56` (`0x800dd2c`) |
| PD11 | 11 | 1 (DRUM2) | PD6 | `vec_56` |
| PD10 | 10 | 2 (DRUM3) | PD4 | `vec_56` |
| PD14 | 14 | 3 (SNARE) | PD5 | `vec_56` |
| PD13 | 13 | 4 (CLP/CYM) | PD3 | `vec_56` |
| PD12 | 12 | 5 (CH HH) | PD7 | `vec_56` |
| PD9  | 9  | 6 (OP HH) | PD7 | `vec_39` (`0x800de14`) |

The voice→jack names assume the engine's voice order matches the panel order. Check on hardware.
Accents are **digital** (active low at the MCU, pull-up), not ADC. CH and OP HH share one accent jack. The mod matrix
reads accents live through `FUN_0800dcd0(n)` (same pins, value 127|63).

## ADC1: 4 endless pots and 5 CV inputs
13-channel scan, 12-bit, left-aligned, DMA2 Stream0 into `0x200090ac` (`FUN_080174c4(hadc, 0x200090ac, 13)`). Ranks are
from `0x800bfd8`–`0x800c038`; analog pins from `FUN_0800c324`.

| Rank | Ch | Pin | Use | Desktop |
|------|----|-----|-----|---------|
| 1–2 | 8, 9 | PB0, PB1 | pot 1 A/B | RV1 |
| 3–4 | 14, 15 | PC4, PC5 | pot 2 A/B | RV2 |
| 5–6 | 12, 13 | PC2, PC3 | pot 3 A/B | RV3 |
| 7–8 | 10, 11 | PC0, PC1 | pot 4 A/B | RV4 |
| 9–13 | 4, 3, 2, 1, 0 | PA4–PA0 | CV1–CV5 (order to confirm) | sliders RV6–RV10 |

Pot ranks sample at 3 cycles, CV ranks at 28. The desktop's CH5/PA5 is gone (PA5 is an LED here).
- Pots: `FUN_0800cae0`, the desktop-style angle tracker (`adc·180/4096`, mirrored to 360° by the other channel).
- CVs: `FUN_0800cbd0`, one CV per call, round-robin. ±5 LSB (12-bit) deadband, `cv = 1.0 − adc12/4096` stored as
  float at `0x20020618[n]` (getter `FUN_0800cc44`). So the input stage inverts: −5 V → ~0.0, +5 V → ~1.0 (to confirm).

## Buttons (14), LEDs (14) and encoder
All buttons are direct GPIO, active high, scanned by `FUN_0800c920` from the main loop (no shift registers, no SPI1).
A change to 1 calls the press handler `FUN_08008b00(i)`; a change to 0 calls `FUN_08008be8(i)`. Each button's LED is
bit `i` of the bitmap at `0x20020614` (set by `FUN_0800aa58(on, i)`), written out by BSRR at the end of
`FUN_0800c920`.

| i | Button pin | Panel (manual p.5) | LED pin | Evidence |
|---|------------|--------------------|---------|----------|
| 0 | PC13 | EDIT | PE4 | menu mode 0 |
| 1 | PB14 | PERFORM | PB10 | menu mode 1 |
| 2 | PB8 | MOD | PE3 | menu mode 2 |
| 3 | PB2 | FX | PB11 | menu mode 5 |
| 4 | PB6 | DRUM | PC15 | held + SELECT plays a voice |
| 5–11 | PE2, PB9, PE1, PE0, PE6, PE5, PB3 | SELECT: OSC/DRUM1 … MIX/OP HH (voice `i−5`) | PA5, PA6, PA7, PC14, PB7, PB4, PC11 | `FUN_08008b00` cases 5–11 |
| 12 | **PA11** | SHIFT | PA8 | read as a modifier in cases 0/2; own case lights its LED only |
| 13 | **PA12** | SAVE/LOAD | PA9 | mode 3, cycling 3 → 4 → 7 (load → save → …) |

The mode → EDIT/PERFORM/MOD/FX names follow the panel order matching the scan order. Not yet checked against each
mode's page strings. The LED for bit 7 (PA7) may have inverted polarity in the decompile; check on hardware.

- Encoder: A PE13, B PE14 (no pull), polled in SysTick (`vec_15`) at 1 kHz. The desktop uses TIM1 input capture instead.
- Encoder switch: PE15 (pull-up), debounced 3×10 ms in SysTick. Same pins as the desktop and Erica's bootloader.

## LCD and SD
| Item | Pins | Source | Desktop |
|------|------|--------|---------|
| LCD (HD44780-style 4-bit; panel is 2×20 OLED) | RS PE12, E PE11, D4–D7 PE7–PE10 | `0x800bc38` | same |
| SD (SDMMC1, 1-bit) | D0 PC8, CK PC12, CMD PD2 (AF12); DMA2 Stream3/6 | `FUN_0800c610` | same pins |
| SD detect | PD1 (pull-up), read by `FUN_08017f70(GPIOD, 2)` | `0x800bc58`, decompile line ~17890 | same |
| SD CS | PD0 input | `0x800bc70` | same |

## Not present in the module app
- No USART (no USART3 at `0x40004800`); PB10/PB11 (desktop MIDI) drive LEDs.
- No USB OTG (`0x50000000` never referenced); PA11/PA12 are buttons.
- No SPI1 (desktop LED/button shift registers).
- Also used: RNG (`0x50060800`); TIM2 (period 1000), TIM14; ADC3/TIM1/8–12 are referenced only inside the HAL.
