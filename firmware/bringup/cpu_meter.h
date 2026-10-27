/*
 * lxr-0: CPU load meter.
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * DWT cycle-counter window accounting, after LXR02Open's queue meter
 * (Core/Hardware/AudioCodecManager.c: audioCodec_initCycleCounter,
 * audioCodec_accountQueueState, audioCodec_getQueueFreePercent).
 * Wrap cpuMeter_begin()/cpuMeter_end() around the block being measured
 * (the audio render, from Phase 2 on). cpuMeter_percent() returns the busy
 * share of the cycles since its last call and starts a new window.
 */
#ifndef CPU_METER_H_
#define CPU_METER_H_

#include <stdint.h>

void cpuMeter_init(void);
void cpuMeter_begin(void);
void cpuMeter_end(void);
uint8_t cpuMeter_percent(void);
/** worst single block since the last cpuMeter_percent(), in cycles */
uint32_t cpuMeter_peakCycles(void);

#endif /* CPU_METER_H_ */
