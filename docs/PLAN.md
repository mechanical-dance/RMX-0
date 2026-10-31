# lxr-0 plan (from 2026-10-31)

Erica Synths published the module firmware (`~/code/vendor/lxr-02/lxr-eurorack-app`, GPL-3.0, v1.51).
`firmware/` is a copy of it (vendor e752339). No more reverse engineering: their implementation is the
reference, including accents. Every change is a small diff marked `lxr-0`, behind a flag in `main.h`,
so newer Erica releases can be merged.

Order of work:
1. **MIDI** (0.3.1): the engine's own MIDI code, on the OP HH trigger jack.
2. **Global reverb:** a parallel reverb bus with per-voice sends, the amount on the perform page's
   empty pot, a settings page in the FX menu, and a cut-off above 95 % CPU load.
3. **Samples:** our own design (not LXR02Open's). Decide on RAM vs SD streaming then.

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
  the perform page; pressing FX again after the FX type's screens opens the reverb page
  (`rev out` / `pre dcy ton ref`), and again returns to the FX type.
- Host check (scratchpad, 2026-10-31): impulse at decay 90 falls ~33 dB/s; feedback 1.0 stays bounded.
- Stereo (0.3.3): their code builds only the left output of the Dattorro/Spin tap table in the comment;
  `Reverb.stereo` adds the right-output taps on the same lines (taps past 2048 samples dropped or
  shortened by 1000, as the left does), with its own output EQ. Host check: L/R correlation 0.26,
  levels within 1 dB. Mono outputs (L1/R1/L2/R2) get the sum. The FX-bus reverb stays mono.
- Noted, not changed: both loops run `apDelay[1]`, so `apDelay[0]` is never written and its taps read 0.

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
