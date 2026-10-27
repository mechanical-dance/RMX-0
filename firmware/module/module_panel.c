/*
 * lxr-0 module firmware: buttons and LEDs.
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * All direct GPIO (docs/HARDWARE_MAP_MODULE.md "Buttons"). Index order is the stock scan order
 * of FUN_0800c920: EDIT PERFORM MOD FX DRUM, SELECT 1..7, SHIFT, SAVE/LOAD. LED i belongs to
 * button i (bitmap 0x20020614). Checked on hardware with the bring-up image.
 */
#include "main.h"
#include "module.h"

typedef struct
{
	GPIO_TypeDef *port;
	uint16_t pin;
} Pin;

static const Pin buttons[MODULE_NUM_BUTTONS] = {
	{GPIOC, GPIO_PIN_13}, {GPIOB, GPIO_PIN_14}, {GPIOB, GPIO_PIN_8}, {GPIOB, GPIO_PIN_2},
	{GPIOB, GPIO_PIN_6},  {GPIOE, GPIO_PIN_2},  {GPIOB, GPIO_PIN_9}, {GPIOE, GPIO_PIN_1},
	{GPIOE, GPIO_PIN_0},  {GPIOE, GPIO_PIN_6},  {GPIOE, GPIO_PIN_5}, {GPIOB, GPIO_PIN_3},
	{GPIOA, GPIO_PIN_11}, {GPIOA, GPIO_PIN_12},
};

static const Pin leds[MODULE_NUM_BUTTONS] = {
	{GPIOE, GPIO_PIN_4},  {GPIOB, GPIO_PIN_10}, {GPIOE, GPIO_PIN_3}, {GPIOB, GPIO_PIN_11},
	{GPIOC, GPIO_PIN_15}, {GPIOA, GPIO_PIN_5},  {GPIOA, GPIO_PIN_6}, {GPIOA, GPIO_PIN_7},
	{GPIOC, GPIO_PIN_14}, {GPIOB, GPIO_PIN_7},  {GPIOB, GPIO_PIN_4}, {GPIOC, GPIO_PIN_11},
	{GPIOA, GPIO_PIN_8},  {GPIOA, GPIO_PIN_9},
};

//trigger LED pulse length in ms (lxr-0 choice; the stock pulse length isn't decoded yet)
#define PULSE_MS 30

static volatile uint32_t pulseUntil[MODULE_NUM_BUTTONS];

void panel_ledSet(uint8_t i, uint8_t on)
{
	leds[i].port->BSRR = on ? leds[i].pin : (uint32_t)leds[i].pin << 16U;
}

void panel_ledPulse(uint8_t i)
{
	pulseUntil[i] = HAL_GetTick() + PULSE_MS;
	panel_ledSet(i, 1);
}

//from the main loop
void panel_tick(void)
{
	const uint32_t now = HAL_GetTick();
	for(uint8_t i = 0; i < MODULE_NUM_BUTTONS; i++)
	{
		if(pulseUntil[i] && (int32_t)(now - pulseUntil[i]) >= 0)
		{
			pulseUntil[i] = 0;
			panel_ledSet(i, 0);
		}
	}
}

//buttons are active high (FUN_0800c920)
int8_t panel_buttonPressed(void)
{
	for(int8_t i = 0; i < MODULE_NUM_BUTTONS; i++)
	{
		if(buttons[i].port->IDR & buttons[i].pin)
			return i;
	}
	return -1;
}
