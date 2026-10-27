/*
 * lxr-0 module firmware: module-only hardware and I/O.
 * SPDX-License-Identifier: GPL-3.0-only
 */
#ifndef MODULE_H_
#define MODULE_H_

#include <stdint.h>
#include "ff.h"

#define MODULE_NUM_BUTTONS 14
#define MODULE_BUTTON_DRUM 4
#define MODULE_BUTTON_VOICE1 5	//the 7 voice buttons (OSC/DRUM1 .. MIX/OP HH) = buttons 5..11, voice i-5

void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_DMA_Init(void);
void MX_I2S2_Init(void);
void MX_I2S3_Init(void);
void MX_RNG_Init(void);
void MX_SDMMC1_SD_Init(void);
void MX_TIM2_Init(void);
void MX_TIM14_Init(void);

void trigger_noteOn(uint8_t voice, uint8_t note, uint8_t vel);
void triggers_init(void);
void triggers_dispatch(void);

void panel_ledSet(uint8_t i, uint8_t on);
void panel_ledPulse(uint8_t i);
void panel_tick(void);
uint8_t panel_buttonHeld(uint8_t i);
int8_t panel_buttonPressed(uint8_t first, uint8_t last);

void display_init(void);
void display_print(uint8_t row, const char *text);
void display_tick(void);

#define MODULE_NUM_KITS 128
#define MODULE_MATRIX_BYTES 54
extern uint8_t moduleKit_matrix[MODULE_MATRIX_BYTES];
extern uint8_t moduleKit_hasMatrix;
extern uint8_t moduleKit_current;
FRESULT moduleKit_load(uint8_t kitNr);
void moduleKit_scan(void);
uint8_t moduleKit_exists(uint8_t kitNr);
const char *moduleKit_fileName(uint8_t kitNr);
int16_t moduleKit_next(int16_t kitNr, int8_t dir);
uint8_t moduleKit_lastUsed(void);

#endif /* MODULE_H_ */
