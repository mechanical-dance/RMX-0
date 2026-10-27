/*
 * lxr-0 module firmware, Phase 2a: the desktop engine played from the module's jacks.
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Boot follows the desktop's main (Core/Src/main.c) without the desktop UI, sequencer
 * UI, MIDI and USB: ITCM copy, caches, clocks, peripherals, LCD, SD mount, /proj00,
 * soundengine_init, globals, then kit 0 into the edit buffer. The main loop plays the
 * trigger jacks (module_triggers.c) and renders audio, as the stock module does
 * (FUN_0800dc68 renders from the main loop, not an ISR).
 *
 * Display: line 1 the kit number and name, line 2 "cpu NN% pk NNNN" (share of time in
 * the engine, and the worst single engine call in microseconds, over the last 250 ms).
 * DRUM held + SELECT n plays voice n (stock behaviour, FUN_08008b00). Turning the
 * encoder loads the next or previous kit (test-image shortcut, not the stock UI).
 */
#include "main.h"
#include "fatfs.h"
#include "lcd.h"
#include "delay.h"
#include "Parameters.h"
#include "Preset/PresetManager.h"
#include "menu/menu.h"
#include "Soundengine/Soundengine.h"
#include "../bringup/cpu_meter.h"
#include "module.h"
#include "encoder.h"
#include <stdio.h>
#include <string.h>

#define BUTTON_DRUM 4

extern TIM_HandleTypeDef htim14;

//the bootloader jumps here with its SD DMA interrupts still enabled (as bringup/main.c)
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

static void show(const char *l1, const char *l2)
{
	lcd_clear();
	lcd_home();
	lcd_string((char *)l1);
	lcd_setcursor(0, 2);
	lcd_string((char *)l2);
}

//desktop main's f_mount error messages
static uint8_t mountSd(void)
{
	const FRESULT res = f_mount((FATFS *)&SDFatFS, (TCHAR const *)SDPath, 1);
	switch(res)
	{
		case FR_OK:
			return 1;
		case FR_NO_FILESYSTEM:
			show("SD error:", "No FS found!");
			break;
		case FR_DISK_ERR:
			show("SD error:", "Disk error!");
			break;
		case FR_NOT_READY:
			show("SD error:", "No Card!");
			break;
		default:
			show("SD error:", "unknown!");
			break;
	}
	HAL_Delay(1000);
	return 0;
}

//DRUM held + SELECT n plays voice n, on the press edge
static void auditionButtons(void)
{
	static int8_t last = -1;
	const int8_t b = panel_buttonPressed();
	const uint8_t drumHeld = (GPIOB->IDR & GPIO_PIN_6) != 0;
	if(b != last && drumHeld && b > MODULE_BUTTON_SELECT1 - 1 && b < MODULE_BUTTON_SELECT1 + 7)
		trigger_noteOn(b - MODULE_BUTTON_SELECT1, 60, 127);
	last = b;
}

static void showStatus(void)
{
	char line[21];
	const uint8_t cpu = cpuMeter_percent();
	const uint32_t peakUs = cpuMeter_peakCycles() / (SystemCoreClock / 1000000);

	char name[9];
	memcpy(name, savefileNames[0][KIT_NAMES], 8);
	name[8] = 0;
	snprintf(line, sizeof line, "%03u %s%s", moduleKit_current, name, moduleKit_hasMatrix ? " m" : "");
	display_print(0, line);
	snprintf(line, sizeof line, "cpu %2u%% pk %4lu", cpu, peakUs);
	display_print(1, line);
}

int main(void)
{
	quiesceBootloaderState();

	extern const unsigned char _itcm_text_start;
	extern const unsigned char _itcm_text_end;
	extern const unsigned char _itcm_data;
	memcpy((void *)&_itcm_text_start, &_itcm_data, (int)(&_itcm_text_end - &_itcm_text_start));

	SCB_EnableICache();
	SCB_EnableDCache();

	HAL_Init();
	SystemClock_Config();
	DWT_Delay_Init();
	cpuMeter_init();

	MX_GPIO_Init();
	MX_DMA_Init();
	MX_I2S2_Init();
	MX_I2S3_Init();
	MX_FATFS_Init();
	MX_RNG_Init();
	MX_TIM14_Init();
	MX_TIM2_Init();
	MX_SDMMC1_SD_Init();

	lcd_init();
	lcd_clear();
	lcd_home();
	MX_FATFS_ReInit();
	show("lxr-0 module", "Phase 2a");

	const uint8_t sdOk = mountSd();
	HAL_Delay(1000);

	sprintf(currentFolder, "/proj00");
	if(sdOk)
	{
		FILINFO fno;
		if(f_stat(currentFolder, &fno) == FR_OK)
			f_chdir(currentFolder);
	}

	soundengine_init();

	if(sdOk && !preset_loadGlobals())
	{
		show("glo.cfg error", "Using defaults.");
		preset_loadDefaultGlobals();
		HAL_Delay(1000);
	}

	morphParameters = mainParameters[0];
	if(!sdOk || (moduleKit_load(moduleKit_lastUsed()) != FR_OK && moduleKit_step(1) != FR_OK))
	{
		show("Kit read error", "/proj00");
		HAL_Delay(1000);
		sprintf(savefileNames[0][KIT_NAMES], "no kit");
	}
	lcd_clear();
	display_init();

	HAL_TIM_Base_Start_IT(&htim14);
	triggers_init();
	HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

	uint32_t nextDisplay = 0;
	while(1)
	{
		triggers_dispatch();
		cpuMeter_begin();
		soundengine_process();
		cpuMeter_end();

		panel_tick();
		auditionButtons();

		const int8_t turn = encode_read4();
		if(turn && sdOk)
			moduleKit_step(turn);

		const uint32_t now = HAL_GetTick();
		if((int32_t)(now - nextDisplay) >= 0)
		{
			nextDisplay = now + 250;
			showStatus();
		}
	}
}

void Error_Handler(void)
{
	//all LEDs on, as the bring-up image
	for(uint8_t i = 0; i < MODULE_NUM_BUTTONS; i++)
		panel_ledSet(i, 1);
	while(1);
}
