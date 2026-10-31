/*
 * lxr0_reverbBus.h - lxr-0: parallel global reverb bus.
 *
 * Per-voice post-fader sends feed Erica's Reverb (SPIN_SEMI 0 layout) with its own buffer in SRAM1,
 * so it runs alongside the FX bus. Settings are kit parameters PAR_LXR0_VERB_*, read every block.
 * The bus is bypassed while the render load exceeds 95 % of the block time.
 */
#ifndef LXR0_REVERBBUS_H_
#define LXR0_REVERBBUS_H_

#include <stdint.h>
#include "lxr0.h"

#if LXR0_REVERB
void lxr0Verb_init(void);
// start of mixer_calcNextSampleBlock: start the load timer, apply changed settings, clear the send sum
void lxr0Verb_beginBlock(void);
// after a voice block is rendered: add it to the send sum (voice 0-5)
void lxr0Verb_addSend(uint8_t voice, const int16_t* data);
// end of mixer_calcNextSampleBlock: run the reverb into the routed output, update the load guard
void lxr0Verb_endBlock(int16_t* output, int16_t* output2);
// defaults for kits saved without the reverb tail (bytesRead = parameter bytes read from the file)
void lxr0Verb_kitDefaults(uint8_t* para, uint32_t bytesRead);

extern volatile uint8_t lxr0Verb_load;		// last block's render load, %
extern volatile uint8_t lxr0Verb_bypassed;	// 1 while the load guard has the bus off
uint8_t lxr0Verb_takePeakLoad(void);		// highest load since the last call, %
uint8_t lxr0Verb_ownLoad(void);				// the bus's own average cost, %
#endif

#endif /* LXR0_REVERBBUS_H_ */
