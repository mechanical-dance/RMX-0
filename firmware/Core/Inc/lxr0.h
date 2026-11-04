/*
 * lxr0.h - lxr-0 feature flags (personal additions to Erica Synths' LXR module firmware).
 * Included by main.h, Parameters.h and menu/menu.h, so every file sees the same parameter list.
 */
#ifndef LXR0_H_
#define LXR0_H_

// MIDI in on the OP HH jack (PD9 = USART3_RX), switched by the PAR_MIDI_MODE global
#define LXR0_MIDI 1
#define LXR0_MIDI_RESYNC 0xFD // an undefined real-time byte: USART3_IRQHandler tells the parser that bytes were lost
// parallel global reverb bus: per-voice sends, own buffer, bypassed above 95 % render load
#define LXR0_REVERB 1
// user samples as oscillator waveforms: installed from the SD card (/samples, /loops) to sectors 6-11
#define LXR0_SAMPLES 1
// restart menu: shift + edit + perform, then fill the bar with the encoder
#define LXR0_RESTART 1
// a retrigger fades out the voice's last output sample instead of dropping it, and the note starts as from silence
#define LXR0_DECLICK 1
// voice links: drums 1-3 grouped as unison, chord or poly (lxr0_voiceLink.c)
#define LXR0_LINK 1
// LFO shapes after the vendor's eight: smooth random, exponential triangle, one-shots (lxr0_lfo.c)
#define LXR0_LFO 1
// pan follows its value while a note sounds, on all six voices (lxr0_pan.c)
#define LXR0_PAN 1
// version on the boot screen (docs/RELEASES.md)
#define LXR0_VERSION "1.0"

#endif /* LXR0_H_ */
