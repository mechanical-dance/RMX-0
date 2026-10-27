/*
 * lxr-0 bring-up image for the LXR-Drum module.
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * First own code on the module. It proves the toolchain, the LXRERIMG packaging and the
 * module pin map (docs/HARDWARE_MAP_MODULE.md), and runs the CPU meter:
 *   line 1: "F2048 t123 c4%"   flash size in KB (FLASHSIZE register), uptime in s, CPU meter
 *                              (share of time spent refreshing the display)
 *   line 2: "b12 t0000000 e"   pressed button index, the 7 trigger jacks (PD9..PD15),
 *                              encoder switch
 * The 14 button LEDs chase in button-index order; a held button lights its own LED.
 * Clock, GPIO and pin numbers mirror the stock module image (LXRE_V2_V1.5): addresses in
 * the comments refer to it.
 */
#include "main.h"
#include "lcd.h"
#include "delay.h"
#include "cpu_meter.h"
#include <stdio.h>

typedef struct
{
	GPIO_TypeDef *port;
	uint16_t pin;
} Pin;

//button scan order of FUN_0800c920: EDIT PERFORM MOD FX DRUM, SELECT 1..7, SHIFT, SAVE/LOAD
static const Pin buttons[14] = {
	{GPIOC, GPIO_PIN_13}, {GPIOB, GPIO_PIN_14}, {GPIOB, GPIO_PIN_8}, {GPIOB, GPIO_PIN_2},
	{GPIOB, GPIO_PIN_6},  {GPIOE, GPIO_PIN_2},  {GPIOB, GPIO_PIN_9}, {GPIOE, GPIO_PIN_1},
	{GPIOE, GPIO_PIN_0},  {GPIOE, GPIO_PIN_6},  {GPIOE, GPIO_PIN_5}, {GPIOB, GPIO_PIN_3},
	{GPIOA, GPIO_PIN_11}, {GPIOA, GPIO_PIN_12},
};

//LED bit i belongs to button i (bitmap 0x20020614, written at the end of FUN_0800c920)
static const Pin leds[14] = {
	{GPIOE, GPIO_PIN_4},  {GPIOB, GPIO_PIN_10}, {GPIOE, GPIO_PIN_3}, {GPIOB, GPIO_PIN_11},
	{GPIOC, GPIO_PIN_15}, {GPIOA, GPIO_PIN_5},  {GPIOA, GPIO_PIN_6}, {GPIOA, GPIO_PIN_7},
	{GPIOC, GPIO_PIN_14}, {GPIOB, GPIO_PIN_7},  {GPIOB, GPIO_PIN_4}, {GPIOC, GPIO_PIN_11},
	{GPIOA, GPIO_PIN_8},  {GPIOA, GPIO_PIN_9},
};

#define FLASHSIZE_KB (*(const uint16_t *)0x1FF0F442)

static void SystemClock_Config(void);
static void GPIO_Init(void);

//the bootloader jumps here with its SD DMA interrupts still enabled
static void quiesceBootloaderState(void)
{
	__disable_irq();
	for(int i = 0; i < 8; i++)
	{
		NVIC->ICER[i] = 0xFFFFFFFF;
		NVIC->ICPR[i] = 0xFFFFFFFF;
	}
	EXTI->IMR = 0;
	EXTI->PR = 0xFFFFFFFF;
	__enable_irq();
}

static void led_set(uint8_t i, uint8_t on)
{
	leds[i].port->BSRR = on ? leds[i].pin : (uint32_t)leds[i].pin << 16U;
}

static int8_t button_pressed(void)
{
	//buttons are active high (FUN_0800c920 calls the press handler on a 1)
	for(int8_t i = 0; i < 14; i++)
	{
		if(buttons[i].port->IDR & buttons[i].pin)
			return i;
	}
	return -1;
}

int main(void)
{
	quiesceBootloaderState();
	HAL_Init();
	SystemClock_Config();
	DWT_Delay_Init();
	cpuMeter_init();
	GPIO_Init();

	lcd_init();
	lcd_clear();
	lcd_home();
	lcd_string("lxr-0 bring-up");
	HAL_Delay(1000);

	uint8_t chase = 0;
	uint32_t nextChase = 0;
	uint32_t nextDisplay = 0;
	char line[17];

	while(1)
	{
		const uint32_t now = HAL_GetTick();
		const int8_t held = button_pressed();

		if(now >= nextChase)
		{
			nextChase = now + 150;
			chase = (chase + 1) % 14;
		}
		for(uint8_t i = 0; i < 14; i++)
			led_set(i, held >= 0 ? i == held : i == chase);

		if(now >= nextDisplay)
		{
			nextDisplay = now + 100;

			//triggers are inverted at the MCU (falling-edge EXTI in the stock image): show 1 = high at the jack
			const uint32_t trig = (~GPIOD->IDR >> 9) & 0x7F;
			const uint8_t encSw = (GPIOE->IDR & GPIO_PIN_15) == 0;

			//CPU share of the display refresh over the last 100 ms window
			const uint8_t cpu = cpuMeter_percent();

			cpuMeter_begin();
			snprintf(line, sizeof line, "F%u t%lu c%u%%", FLASHSIZE_KB, now / 1000, cpu);
			lcd_setcursor(0, 1);
			lcd_string(line);
			lcd_string("    ");
			if(held >= 0)
				snprintf(line, sizeof line, "b%02d t", held);
			else
				snprintf(line, sizeof line, "b-- t");
			lcd_setcursor(0, 2);
			lcd_string(line);
			for(int b = 0; b < 7; b++)
				lcd_data((trig >> b) & 1 ? '1' : '0');
			lcd_data(' ');
			lcd_data(encSw ? 'e' : ' ');
			cpuMeter_end();
		}
	}
}

//mirrors the stock module's SystemClock_Config (FUN_0800b860): HSE 8 MHz, PLL M8 N216 P2 Q9,
//overdrive, AHB /1, APB1 /4, APB2 /2, flash latency 7 -> 216 MHz
static void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 8;
	RCC_OscInitStruct.PLL.PLLN = 216;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 9;
	RCC_OscInitStruct.PLL.PLLR = 2;
	if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
		Error_Handler();
	if(HAL_PWREx_EnableOverDrive() != HAL_OK)
		Error_Handler();

	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
	                            | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
	if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK)
		Error_Handler();
}

//mirrors the stock module's GPIO block in main (0x800bc10..0x800bc70), without the EXTI and
//alternate functions this image doesn't use
static void GPIO_Init(void)
{
	GPIO_InitTypeDef g = {0};

	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOE_CLK_ENABLE();
	__HAL_RCC_GPIOH_CLK_ENABLE();

	HAL_GPIO_WritePin(GPIOE, 0x1F98, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOC, 0xC800, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA, 0x03E0, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB, 0x0C90, GPIO_PIN_RESET);

	//buttons and encoder A/B: inputs, no pull
	g.Mode = GPIO_MODE_INPUT;
	g.Pull = GPIO_NOPULL;
	g.Pin = 0x6067; HAL_GPIO_Init(GPIOE, &g);
	g.Pin = 0x2000; HAL_GPIO_Init(GPIOC, &g);
	g.Pin = 0x434C; HAL_GPIO_Init(GPIOB, &g);
	g.Pin = 0x1800; HAL_GPIO_Init(GPIOA, &g);
	g.Pin = 0x0001; HAL_GPIO_Init(GPIOD, &g);

	//LEDs: push-pull outputs
	g.Mode = GPIO_MODE_OUTPUT_PP;
	g.Speed = GPIO_SPEED_FREQ_LOW;
	g.Pin = 0x0018; HAL_GPIO_Init(GPIOE, &g);
	g.Pin = 0xC800; HAL_GPIO_Init(GPIOC, &g);
	g.Pin = 0x03E0; HAL_GPIO_Init(GPIOA, &g);
	g.Pin = 0x0C90; HAL_GPIO_Init(GPIOB, &g);

	//LCD
	g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	g.Pin = 0x1F80; HAL_GPIO_Init(GPIOE, &g);

	//encoder switch, SD detect, accents, triggers: inputs with pull-up
	g.Mode = GPIO_MODE_INPUT;
	g.Pull = GPIO_PULLUP;
	g.Pin = 0x8000; HAL_GPIO_Init(GPIOE, &g);
	g.Pin = 0x01FA; HAL_GPIO_Init(GPIOD, &g);
	g.Pin = 0xFE00; HAL_GPIO_Init(GPIOD, &g);
}

void Error_Handler(void)
{
	//all LEDs on: clock setup failed
	for(uint8_t i = 0; i < 14; i++)
		led_set(i, 1);
	while(1);
}
