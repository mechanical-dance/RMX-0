/*
 * lxr-0: CPU load meter (see cpu_meter.h).
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "cpu_meter.h"
#include "stm32f7xx.h"

static volatile uint32_t window_start = 0;
static volatile uint32_t window_busy = 0;
static volatile uint32_t block_start = 0;
static volatile uint32_t block_peak = 0;
static uint32_t last_peak = 0;

static uint32_t irq_save(void)
{
	uint32_t primask = __get_PRIMASK();
	__disable_irq();
	return primask;
}

static void irq_restore(uint32_t primask)
{
	__set_PRIMASK(primask);
}

//from LXR02Open audioCodec_initCycleCounter()
void cpuMeter_init(void)
{
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	DWT->LAR = 0xC5ACCE55;
	DWT->CYCCNT = 0;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

	window_start = DWT->CYCCNT;
	window_busy = 0;
	block_peak = 0;
}

void cpuMeter_begin(void)
{
	block_start = DWT->CYCCNT;
}

void cpuMeter_end(void)
{
	//unsigned subtraction is safe across the ~20 s CYCCNT wrap at 216 MHz
	const uint32_t elapsed = DWT->CYCCNT - block_start;
	window_busy += elapsed;
	if(elapsed > block_peak)
		block_peak = elapsed;
}

//snapshot under a brief IRQ mask, divide in main context (as audioCodec_getQueueFreePercent)
uint8_t cpuMeter_percent(void)
{
	const uint32_t primask = irq_save();
	const uint32_t now = DWT->CYCCNT;
	const uint32_t total = now - window_start;
	const uint32_t busy = window_busy;
	last_peak = block_peak;
	window_start = now;
	window_busy = 0;
	block_peak = 0;
	irq_restore(primask);

	if(total == 0)
		return 0;
	return (uint8_t)(((uint64_t)busy * 100u) / total);
}

uint32_t cpuMeter_peakCycles(void)
{
	return last_peak;
}
