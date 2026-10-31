/*
 * lxr0.h - lxr-0 feature flags (personal additions to Erica Synths' LXR module firmware).
 * Included by main.h, Parameters.h and menu/menu.h, so every file sees the same parameter list.
 */
#ifndef LXR0_H_
#define LXR0_H_

// MIDI in on the OP HH jack (PD9 = USART3_RX), switched by the PAR_MIDI_MODE global
#define LXR0_MIDI 1
// parallel global reverb bus: per-voice sends, own buffer, bypassed above 95 % render load
#define LXR0_REVERB 1
// version on the boot screen (docs/RELEASES.md)
#define LXR0_VERSION "0.3.4"

#endif /* LXR0_H_ */
