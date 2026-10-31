# lxr-0 plan (from 2026-10-31)

Erica Synths published the module firmware (`~/code/vendor/lxr-02/lxr-eurorack-app`, GPL-3.0, v1.51).
`firmware/` is a copy of it (vendor e752339). No more reverse engineering: their implementation is the
reference, including accents. Every change is a small diff marked `lxr-0`, behind a flag in `main.h`,
so newer Erica releases can be merged.

Order of work:
1. **MIDI** (0.3.1): the engine's own MIDI code, on the OP HH trigger jack.
2. **Global reverb:** a parallel reverb bus with per-voice sends, the amount on the perform page's
   empty pot, a settings page in the FX menu, and a cut-off above 95 % CPU load.
3. **Samples** (0.4.0): an oscillator waveform, as on the original LXR. Files live on the SD card and
   are installed to internal flash.

Images: `releases/LXRE_LXR0_V0.<n>.<build>.img`, logged in `RELEASES.md`.

## MIDI (`LXR0_MIDI`)
- OP HH (PD9) becomes USART3_RX (AF7), 31250 baud, receive only. `RXINV` is set because the trigger
  input stage inverts (gate high reads low). Bytes with framing or noise errors are dropped.
- Switch: the unused `PAR_MIDI_MODE` global, shown as **mid** (Op HH / MIDI In) on the global page.
  `glo.cfg` keeps its 31-byte layout. In MIDI mode the OP HH trigger EXTI is masked.
- Re-enabled from the desktop code: `uart_processMidi`, `midiParser_parseUartData`, note-on handling
  (per-voice channel, per-voice note or "any", global channel → active voice). Left out: sequencer
  record/echo, MIDI clock and transport, MIDI out (no TX pin).
- Already active on the module before this change: CCs on the global channel go to `midiParser_ccHandler`.
- Global page (SHIFT + LOAD/SAVE): 1st screen `scr cv`, 2nd `mid ch` (OP HH MIDI, global channel),
  then sub-page 2 `ch1`–`ch7` and sub-page 3 `nt1`–`nt7` (note 0 = any, plays the incoming pitch).
  Saved to `glo.cfg` when you leave the page (stock behaviour).

### 0.3.1 hardware test
Cable: TRS Type A MIDI into the OP HH jack (tip = MIDI pin 5).
| Check | Pass |
|------|------|
| Boot | line 1 `lxr-0 0.3.1`, line 2 `LXR2 Drums V1.51` |
| `mid` off | OP HH triggers as on stock; all other jacks unchanged |
| `mid` on, set `nt1` = 36, `ch1` = your channel | note 36 plays Drum 1; velocity follows the note |
| Each voice | own note/channel plays only that voice |
| `nt` = any | voice plays at the incoming pitch |
| Power cycle | `mid` and the notes/channels are kept |
If nothing plays in MIDI mode: the input stage may not pass 31.25 kbaud, or the polarity guess is wrong
(build with `RXINV` off to tell). Defaults from `glo.cfg` are all voices on ch 1, note 48, so one note
48 plays every voice until the notes are set.

## Global reverb (`LXR0_REVERB`, 0.3.2)
- A second instance of Erica's `Reverb.c` (the FX bus reverb, `SPIN_SEMI 0`) with its own
  56 KB buffer in SRAM1, so it runs alongside whatever the FX bus does. `Reverb_init` keeps a buffer
  set before init (the only change to their file).
- Sends are post-fader (after decimation), per voice; both hats share voice 6's send.
- 12 kit parameters `PAR_LXR0_VERB_*` after `PAR_CV_SRC_18`: sends 1–6, amount (return level),
  out (as FX routing), pre-delay, decay, tone, reflection (as FX P1–P4). Saved at the end of the kit
  file: stock firmware reads only its own length; older kits load with sends 0 and bus defaults
  (amount 100, St1, pre 10, decay 90, tone 64, reflection 40). Project files pad kits to 512 bytes,
  so they stay compatible. Parameters above 255 aren't sent to the engine, so the bus polls them
  every block.
- Load guard: the render of each block is timed with the DWT cycle counter. Above 95 % of the block
  time the bus is bypassed; it comes back (with a cleared tail) after the load plus the reverb's own
  measured cost stays below 90 % for 0.5 s.
- Menus: `rev` (send) is slot 6 of each voice's MIX page (2nd screen); `rev` (amount) is the 4th pot on
  the perform page. FX presses cycle: FX type screen 1, screen 2, reverb `pre dcy ton ref`, reverb
  `out`, back to the FX type (0.3.4; 0.3.2/0.3.3 skipped the reverb's 2nd screen and showed the
  amount twice).
- Host check (scratchpad, 2026-10-31): impulse at decay 90 falls ~33 dB/s; feedback 1.0 stays bounded.
- Stereo (0.3.3): their code builds only the left output of the Dattorro/Spin tap table in the comment;
  `Reverb.stereo` adds the right-output taps on the same lines (taps past 2048 samples dropped or
  shortened by 1000, as the left does), with its own output EQ. Host check: L/R correlation 0.26,
  levels within 1 dB. Mono outputs (L1/R1/L2/R2) get the sum. The FX-bus reverb stays mono.
- Fixed (0.3.4): both loops ran `apDelay[1]`, so `apDelay[0]` was never written and its taps read 0.
  Loop 1 now runs `apDelay[0]`, as its comment says. Host check: denser, slightly longer tail, L/R
  correlation 0.23, feedback 1.0 still bounded.

### 0.3.2 hardware test
| Check | Pass |
|------|------|
| Boot | line 1 `lxr-0 0.3.2`; kits load as before |
| Stock kit, any send up | reverb on outs 1–2 (`out` = St1) |
| Perform pot 4 (`rev`) | changes the reverb level |
| FX pressed 2–3× | reverb page appears; pre/decay/tone/reflection audible |
| FX bus delay + reverb sends together | both run |
| Save a kit, reload, power cycle | sends and reverb settings kept |
| Same kit on stock firmware | loads, plays without reverb |
| Heavy kit (Penalty) + all sends up | no dropouts; if load passes 95 % the reverb cuts out, returns when load drops |
There is no load readout yet: if the reverb vanishes on a heavy kit, that's the guard.
0.3.3 adds: the reverb is stereo on St1/St2 (different left and right tails).

## Samples (`LXR0_SAMPLES`, 0.4.0)
- Design from the original LXR (`calcUserSampleOscBlock`, samples as waveforms from number 6 up, global,
  in flash). LXR02Open is its F765 port and confirmed the flash layout. Its code isn't copied: the
  installer uses this firmware's FatFs and HAL.
- Storage: sectors 6-11 (1.5 MB, about 17 s at 44.1 kHz or 34 s at 22 kHz). The linker now stops the app
  at sector 6. Table header written last, so an interrupted install reads as no samples. A firmware
  update erases these sectors (the bootloader erases 2-11): reinstall from the card afterwards.
- SD card: `/samples` (one-shot) then `/loops` (looped), each in name order, up to 120 in total. PCM WAV,
  16 or 24 bit (24 keeps the top 16), mono or stereo (summed), any rate. Files that don't fit or don't
  parse are skipped and counted. Names are FAT 8.3 (LFN is off), so a long file name shows as `NAME~1`.
- Install: LOAD/SAVE button cycles Load kit, Save kit, Save morph, `Load:Samples`; click the encoder.
  The current kit is autosaved first (`tmp.snd`, as the stock timer does), so the reset brings it back.
  Then audio stops, flash is erased and rewritten, and the module resets. With no usable files, or a chip
  set to dual bank, flash is left alone. If the kit can't be saved the install doesn't start.
- Playback: `osc.sampleHi` extends the phase past the old 32768-frame limit. Pitch as the original LXR and
  LXR02Open, so kits match PHX-D01: coarse 29 with fine 35 is original speed (-34 on the display). Other
  rates scale from there. (0.4.0 had a correction that put these kits 22 cents off; removed in 0.4.1.)
  One-shots stop at the end; the voice's amp envelope still applies, so long samples need a long decay.
- The waveform page shows `s<n>` and, while editing, the sample name.
- Not done: modulating the waveform (LFO/velocity/CV) only reaches the six built-ins. Kits store the
  waveform number, so changing what's on the card can change which sample a kit plays.
- Host check: `tools/samples-host-test/run.sh` runs the installer on real WAV files against a simulated
  flash, plus playback. Not covered there: SD, flash and menu on hardware, CPU cost.

### 0.4.x hardware test
| Check | Pass |
|------|------|
| Boot, no samples installed | line 1 `lxr-0 0.4.1`; kits and reverb as in 0.3.4 |
| Card with `/samples`, press LOAD/SAVE to `Load:Samples`, click | `Erasing flash...`, file count, `Samples done`, restart |
| Voice waveform past `Pwm` | `s0`, `s1`...; name while editing; sample plays on trigger |
| Coarse 29, fine 35 | original pitch; a 22.05 kHz file too |
| Edit the kit, install samples right away | same edits still there after the restart |
| Stereo file, 24-bit file | plays, nothing odd in level |
| File over 1 s, long decay | plays to the end (stock limit was 0.74 s) |
| File in `/loops`, long decay | loops without a jump in pitch |
| FM on a sample, retrigger mid-sample | no crash, restarts from the start |
| Power cycle | samples still there |
| Heavy kit with several long samples | no new dropouts (flash read cost is unmeasured) |

## Restart menu (`LXR0_RESTART`, 0.4.2)
For updating without powering the rack down: the SD card can be pulled while the module plays, and the
firmware remounts it when it comes back (stock behaviour).
- Hold SHIFT and press EDIT and PERF in either order. The first of the two still does its normal SHIFT
  action, so the page behind the menu may have changed.
- Turn the encoder right to fill the 16-cell bar, left to back off. A full bar restarts the module, which
  goes through the bootloader as at power-on: hold the encoder for firmware update mode.
- Any button leaves without restarting. The encoder click does nothing here. Audio keeps playing.
- If a card is in, the current kit is autosaved first. With no card it restarts anyway.

| Check | Pass |
|------|------|
| SHIFT + EDIT + PERF, both orders | menu opens, audio keeps playing |
| Turn right, then left | bar grows and shrinks; no parameter changes |
| Any button | menu closes, nothing else happens |
| Fill the bar, encoder held | bootloader update mode |
| Fill the bar, encoder not held | normal boot, same kit and edits |
| Pull the card, fill the bar | restarts without hanging |

## 0.4.3 fixes
- Sample scan: `_FS_LOCK` is 1 in this firmware's FatFs, so the open folder blocked every file and all WAVs
  failed. The scan now reads the names, closes the folder, then opens the files. `tools/samples-host-test`
  also runs the installer on the real FatFs now.
- An install that can't start (no folder, no usable files, kit not saved, dual bank) says why, keeps audio
  running and returns to the menu. Only a real install resets.
- Reverb guard: bypass needs 8 blocks in a row over 95 %, and the reverb's own cost is an average. It was a
  peak that stopped decaying once bypassed, so one spike could keep the reverb off until a restart.
- Reverb 2nd screen (FX, FX... to `out`) shows `cpu` (peak render load, %), `rev` (on/off: off means the guard
  has it bypassed) and `own` (the bus's own average load, %).
- Host check of the bus at maximum settings: output stays alive, about 14x quieter than defaults at St1.
  `out` at maximum is R2.
