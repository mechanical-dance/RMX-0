# lxr-0 features: voice links, glide, MIDI

This covers firmware 0.4.13 to 0.5.6. Drums 1–3 are the three drum voices; the snare, cymbal and hi-hat are not part of links.

## Voice links

Two or three of drums 1–3 can be linked into one instrument. The link is saved with the kit.

### Forming a group

1. Hold **DRUM**.
2. Hold the first drum's button and press a second drum's button. The two are linked.
3. While still holding both, press the third drum to add it.

Holding a different pair while DRUM is held replaces the group. If the link mode was off, forming a group sets it to unison. When you let go, the display shows the group's link settings.

While DRUM is held, the LEDs show the selected drum. When the selected drum is in the group, the whole group lights.

### Editing

With a linked drum selected, the knobs and encoder edit every drum in the group. Each drum moves by the same amount, so the differences between them are kept, and each one stops at its own limit.

These stay per drum: volume, pan, output, reverb send, the LFO page, and the CV, velocity and LFO routes.

- **Edit one drum on its own:** with DRUM held, double-tap a linked drum. Its LED blinks, and edits go to that drum only.
- **Back to the group:** with DRUM held, press the drum once.
- **Edit a drum outside the group:** select it as usual. The link keeps running.

### Link settings

These are on the drum transient page of drums 1, 2 or 3. All three pages show the same kit-wide values.

| Screen | Setting | Values | Effect |
|---|---|---|---|
| 1 | `mod` Mode | `off`, `uni`, `chd`, `ply` | Off, unison, chord, poly |
| 2 | `det` Detune | 0–127 | Unison: spreads the drums up to ±50 cents |
| 2 | `wid` Width | 0–127 | Unison and chord: spreads the drums across the stereo field |
| 2 | `syn` Sync | off, on | On: copies the lead's sound to the other drums, and edits set the same value on every drum instead of moving each by the same amount. In chord mode this also copies the tuning, which removes the intervals |
| 2 | `alc` Alloc | `old`, `hi` | Poly, all drums busy: replace the oldest note or the highest |

### How a group plays

- **The lead:** the lowest-numbered linked drum takes the trigger, accent, pitch CV and MIDI notes for the group. The other linked drums ignore their own triggers, CV and MIDI channel while linked.
- **Unison:** every linked drum plays each note, spread by detune and width. The level is scaled so that linking doesn't make the sound louder.
- **Chord:** every linked drum plays each note. The intervals come from each drum's own tuning, so tune drums 2 and 3 to set the chord. Each coarse step is one semitone: drum 1 at N with drums 2 and 3 at N+4 and N+7 gives a major chord, and N+3 and N+7 a minor one.
- **Poly:** each note goes to one drum and keeps its pitch. A drum whose note has ended is used first. When all are busy, `alc` decides which note is replaced.

## Glide (0.5.4, types 0.5.6)

Glide is on the second screen of the pitch mod page of drums 1–3. Both settings are kit-wide.

- **`gld`, 0–127:** sets a time T from 25 ms (1) to 8 s (127), with about 0.45 s at 64. 0 is off.
- **`typ`:** how the pitch moves.

| Type | Behaviour |
|---|---|
| `lcr` Linear constant rate | T per octave, even in pitch. Bigger jumps take longer: an octave takes 12 times as long as a semitone. Minimoog-style lead glide. |
| `lct` Linear constant time | Every slide takes T, whatever the interval. Good for acid-style slides at low settings. |
| `exp` Exponential | An RC lag: fast at first, then easing into the note, within 1 % of it after T. Classic analog portamento. |

- **Always on:** like an analog portamento, it glides on every pitch change, through silence and repeated hits. A new note in the middle of a slide continues from wherever the pitch is.
- **What slides:**
  - Unison and chord: the whole group, with detune and chord intervals kept.
  - No link: drum 1.
  - Poly: nothing slides.
- **What it follows:** note pitch, pitch CV and tuning edits. A jump in pitch CV slides like a new note. Slow CV movement, such as vibrato, is smoothed more as glide goes up.
- **On top of the slide:** the pitch envelope, transient and FM still play on every note.

Kits are 341 bytes. 0.5.4–0.5.5 kits keep their glide setting and load as `lcr`; older kits load with glide off.

## LFO shapes (0.5.5)

Eleven shapes follow the original eight (`sin tri sup sdn sqr rnd xup xdn`) on the LFO waveform knob. Rate, retrigger voice and offset work as before, except where noted.

| Shape | Behaviour |
|---|---|
| `nse` | Smooth random. Moves through a new random value each cycle on a smooth curve, with no steps and no pauses. A retrigger continues from where it is, with no jump. |
| `xtr` | Exponential triangle. Curved rise to a sharp peak at half a cycle, then a curved fall. |
| `si1 tr1 su1 sd1 xu1 xd1 xt1` | One-shot versions of sine, triangle, saw up, saw down, exponential up, exponential down and exponential triangle. Each hit of the retrigger voice runs one cycle, then the LFO holds its last value. Use them as extra envelopes. |
| `sq1` | One-shot gate: on for the first half of the cycle, then off. |
| `rn1` | Random per hit. Each hit moves smoothly to a new random value within one cycle, then holds. |

For the one-shots and `rn1`:
- **Offset:** a delay before the shot starts, up to one cycle.
- **Retrigger off:** the LFO holds still.

## MIDI

The OP HH jack works as a MIDI input when MIDI in is turned on in the global settings (**SHIFT + SAVE/LOAD**).

- **Drum channels:** each voice listens on its own channel. Channel and note are set per voice on the global page. The note `any` plays every note at its pitch; a specific note plays the voice only on that note.
- **Global channel:** plays the drum that is currently selected on the panel. If that drum is linked, it plays the group. Set it to `off` to turn it off.
- **Links:** a linked group plays from its lead drum's channel. For poly over MIDI, set the lead drum's note to `any`.
- **Note-off:** ignored. Each note plays its envelope through.

To play a group from one keyboard, put the lead drum on the keyboard's channel. Keep the other voices, and the global channel, off that channel.

## Retrigger and sample trims (0.4.13)

- **Retrigger:** retriggering a voice that is still sounding no longer causes a step or click in the output.
- **Trimmed samples:** a sample with a trim fades in at its start, fades out at its end, and crossfades at a loop point.

A hit from silence and an untrimmed sample sound exactly as before.
