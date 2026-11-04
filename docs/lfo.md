---
title: LFOs
nav_order: 8
---

# LFOs

## Routing

An LFO reaches a sound through the mod matrix: set a slot's source to `lf1`–`lf6`, then its destination and amount. One LFO can feed several slots. The LFO page sets the shape, rate, retrigger, sync and offset.

In this firmware, the LFO page's own target and amount settings don't route anything, and velocity only scales volume: the velocity destination setting has no effect. Both are the same on the stock firmware. Matrix routes take effect when the kit loads or when set from the menu.

## Shapes

Eleven shapes follow the original eight (`sin tri sup sdn sqr rnd xup xdn`) on the LFO waveform knob. Rate, retrigger voice and offset work as before, except where noted.

| Shape | Behaviour |
|---|---|
| `nse` | Smooth random. Moves through a new random value each cycle on a smooth curve, with no steps. A retrigger continues from where it is, with no jump |
| `xtr` | Exponential triangle. A curved rise to a sharp peak at half a cycle, then a curved fall |
| `si1 tr1 su1 sd1 xu1 xd1 xt1` | One-shot sine, triangle, saw up, saw down, exponential up, exponential down and exponential triangle. Each hit of the retrigger voice runs one cycle, then the LFO holds its last value. Use them as extra envelopes |
| `sq1` | One-shot gate: on for the first half of the cycle, then off |
| `rn1` | Random per hit. Each hit moves smoothly to a new random value within one cycle, then holds |

For the one-shots and `rn1`, offset is a delay before the shot starts, up to one cycle. With retrigger off, the LFO holds still.
