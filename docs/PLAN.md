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
