/*
 * lxr-0 module firmware: the desktop UI symbols the engine links against.
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * The desktop engine calls into its menu, LED, button and fader code (Core/Src/menu/menu.c,
 * ledHandler.c, buttonHandler.c, IO/adcPots.c), which the module doesn't build. This file
 * defines the shared parameter storage those files own, and the calls the engine makes,
 * with the module's behaviour. Most are no-ops: the module has no sequencer UI.
 */
#include "main.h"
#include "Parameters.h"
#include "frontPanelParser_ui.h"
#include "Soundengine/MIDI/CortexParser.h"
#include "Preset/PresetManager.h"
#include "menu/menu.h"

//from menu/menu.c
uint8_t menu_selectedPresetNr[NUM_PRESET_LOCATIONS];
uint8_t menu_loadedPresetNr[NUM_CURRENT];
uint8_t menu_shownPattern = 0;
uint8_t menu_activeBar = 0;
uint8_t menu_songMenuPosition = 0;
uint8_t menu_activePage = 0;
uint8_t menu_playedPattern = 0;
uint8_t mainParameters[NUM_KIT_SLOTS][END_OF_SOUND_PARAMETERS];
uint8_t activeParameters[NUM_PARAMS];
uint8_t *morphParameters;

//from stm32f7xx_it.c (the desktop's 8 kHz tick, counted in TIM14)
INDTCMZ volatile uint32_t systick_ticks;

void menu_repaint() {}
void menu_repaintAll() {}
void menu_switchSubPage(uint8_t subPageNr, uint8_t toggle) { (void)subPageNr; (void)toggle; }
uint8_t menu_getSubPage() { return 0; }
uint8_t menu_getActiveVoice() { return 0; }
void menu_setShownPattern(uint8_t patternNr) { menu_shownPattern = patternNr; }
uint8_t menu_getViewedPattern() { return menu_shownPattern; }

uint8_t buttonHandler_getMode() { return 0; }

void led_setValue(uint8_t val, uint8_t ledNr) { (void)val; (void)ledNr; }
void led_initPatternSelectLeds() {}
void led_updateSeqLeds() {}
void led_clearSequencerLeds() {}
void led_setActive_step(uint8_t stepNr) { (void)stepNr; }
void led_clearActive_step() {}
void led_setBlinkLed(const uint8_t ledNr, const uint8_t onOff) { (void)ledNr; (void)onOff; }
void led_clearAllBlinkLeds() {}
void led_blinkHandler() {}
void led_resetBlinkTimer() {}
uint8_t led_idx2StepLed(uint8_t idx) { return idx; }

//the module has no per-voice faders: full level (IO/adcPots.c with USE_FADER_VOLUME 0)
float adc_getFaderVolume(uint8_t idx)
{
	(void)idx;
	return 1.f;
}

//engine -> UI messages (note-on LED pulses, sequencer state): the module UI doesn't use them yet
void frontPanel_parseData(uint8_t data) { (void)data; }

//UI -> engine, as frontPanelParser.c's frontPanel_sendData (the non-AVR path). The module
//ignores the kit's per-LFO and velocity targets (confirmed on hardware, decisions.md
//2026-10-26); modulation goes through the mod matrix only.
void frontPanel_sendData(uint8_t status, uint8_t data1, uint8_t data2)
{
	if(status == CC_LFO_TARGET || status == CC_VELO_TARGET)
		return;
	frontParser_parseUartData(status);
	frontParser_parseUartData(data1);
	frontParser_parseUartData(data2);
}
