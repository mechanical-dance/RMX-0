---
title: Voice links
nav_order: 4
---

# Voice links

Two or three of drums 1–3 can be linked and played as one instrument: in unison, as a chord, or polyphonically. The snare, cymbal and hi-hats are not part of links. The link is saved with the kit.

## Linking drums

1. Hold **DRUM**.
2. Hold one drum's button and press a second drum's button. The two are linked.
3. While still holding both, press the third drum to add it.

Holding a different pair while DRUM is held replaces the group. If the link mode was off, linking sets it to unison. When you let go, the display shows the link settings.

While DRUM is held, the LEDs show the selected drum. When the selected drum is in the group, the whole group lights.

## Unlinking

Set `lnk` to `off`. Selecting a drum outside the group does not unlink it: the group keeps playing.

## Editing a group

With a linked drum selected, the knobs and encoder edit every drum in the group. Each drum moves by the same amount, so the differences between them are kept, and each stops at its own limit.

These stay per drum: volume, pan, output, reverb send, the LFO page, and the CV, velocity and LFO routes.

- **Edit one drum on its own:** with DRUM held, double-tap a linked drum. Its LED blinks, and edits go to that drum only.
- **Back to the group:** with DRUM held, press the drum once.

## Link settings

On the transient page of drum 1, 2 or 3. All three pages show the same kit-wide values.

| Screen | Setting | Values | Effect |
|---|---|---|---|
| 1 | `lnk` Mode | `off`, `uni`, `chd`, `ply` | Off, unison, chord, poly |
| 2 | `det` Detune | 0–127 | Unison: spreads the drums up to ±50 cents |
| 2 | `wid` Width | 0–127 | Unison and chord: spreads the drums across the stereo field |
| 2 | `syn` Sync | `off`, `on` | Copies the lead's sound to the other drums, and edits set the same value on every drum instead of moving each by the same amount. In chord mode this also copies the tuning, which removes the intervals |
| 2 | `alc` Alloc | `old`, `hi` | Poly, all drums busy: replace the oldest note or the highest |

## How a group plays

- **The lead** is the lowest-numbered linked drum. Its trigger, accent, pitch CV and MIDI notes play the group. The other linked drums ignore their own triggers, CV and MIDI channel while linked.
- **Unison:** every linked drum plays each note, spread by detune and width. Each drum plays at its own volume, so a group is louder than one drum.
- **Chord:** every linked drum plays each note. The intervals come from each drum's own tuning, and each coarse step is a semitone. With drum 1 at N, drums 2 and 3 at N+4 and N+7 give a major chord; N+3 and N+7 a minor one.
- **Poly:** each note goes to one drum and keeps its pitch. A drum whose note has ended is used first, then one whose MIDI key has been released. When all are busy, `alc` decides which note is replaced. Triggers count as released at once.
