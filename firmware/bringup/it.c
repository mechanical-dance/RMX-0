/*
 * lxr-0 bring-up image: interrupt handlers.
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "main.h"

void SysTick_Handler(void)
{
	HAL_IncTick();
}

void HardFault_Handler(void)
{
	while(1);
}
