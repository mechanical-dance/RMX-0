---
title: MIDI input
nav_order: 3
---

# MIDI input

The OP HH trigger jack can work as a MIDI input. Use a TRS type A cable (tip is MIDI pin 5). Notes and CCs are received. MIDI clock, transport and MIDI out are not supported.

## Setup

Open the global settings menu with **SHIFT + LOAD/SAVE**. Each further **SHIFT + LOAD/SAVE** press moves to the next sub-page, and the encoder scrolls to a sub-page's second screen. Settings are saved when you leave the menu.

| Where | Setting | Values |
|---|---|---|
| Sub-page 1, 2nd screen | `mid` | OP HH works as a trigger input or as MIDI in |
| Sub-page 1, 2nd screen | `ch` | Global channel: `off` or 1–16 |
| Sub-page 2 | `t1`–`t7` | Each voice's channel |
| Sub-page 3 | `n1`–`n7` | Each voice's note. `any` plays every note at its own pitch; a specific note plays the voice only on that note |

## How notes play

- **Voice channels:** a note on a voice's channel plays that voice, with velocity.
- **Global channel:** plays the drum selected on the panel. If that drum is [linked](voice-links.md), it plays the group. Set it to `off` when you don't want it.
- **Linked drums** play from the lead drum's channel. For poly over MIDI, set the lead drum's note to `any`.
- **Note-off** releases the key for [legato](glide-legato.md#legato) and poly voice allocation. It doesn't stop the sound: each note plays its envelope through.
- **During a kit load,** incoming notes are held and play late rather than being lost.

To play a group from a keyboard, put the lead drum on the keyboard's channel, and keep the other voices and the global channel off that channel.
