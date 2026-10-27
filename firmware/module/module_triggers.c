/*
 * lxr-0 module firmware: trigger and accent inputs.
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Mirrors the stock module (LXRE_V2_V1.5): the trigger ISRs (vec_56 at 0x800dd2c for
 * EXTI15_10, vec_39 at 0x800de14 for EXTI9_5) read the voice's accent jack and queue a
 * note-on with FUN_080133dc(voice, 60, accent ? 127 : 63). FUN_08013214, called at the
 * start of each audio block, plays the queued notes and pulses the voice button's LED.
 */
#include "main.h"
#include "module.h"
#include "Soundengine/DSPAudio/DrumVoice.h"
#include "Soundengine/DSPAudio/Snare.h"
#include "Soundengine/DSPAudio/CymbalVoice.h"
#include "Soundengine/DSPAudio/HiHat.h"

#define NUM_VOICES 7
#define TRIGGER_NOTE 60
#define VEL_ACCENT 127
#define VEL_NORMAL 63

typedef struct
{
	uint16_t trigPin;	//GPIOD
	uint16_t accentPin;	//GPIOD, active low at the MCU
} TriggerPins;

//docs/HARDWARE_MAP_MODULE.md "Trigger inputs"; CH and OP HH share one accent jack
static const TriggerPins pins[NUM_VOICES] = {
	{GPIO_PIN_15, GPIO_PIN_8},	//DRUM1
	{GPIO_PIN_11, GPIO_PIN_6},	//DRUM2
	{GPIO_PIN_10, GPIO_PIN_4},	//DRUM3
	{GPIO_PIN_14, GPIO_PIN_5},	//SNARE
	{GPIO_PIN_13, GPIO_PIN_3},	//CLP/CYM
	{GPIO_PIN_12, GPIO_PIN_7},	//CH HH
	{GPIO_PIN_9,  GPIO_PIN_7},	//OP HH
};

//0x200217a0..: ready flag, pending flags, notes, velocities
static volatile uint8_t ready = 0;
static volatile uint8_t pending[NUM_VOICES];
static volatile uint8_t notes[NUM_VOICES];
static volatile uint8_t vels[NUM_VOICES];
volatile uint8_t trigger_lastVel = 0;	//diagnostics: velocity of the last trigger

//FUN_080133dc
void trigger_noteOn(uint8_t voice, uint8_t note, uint8_t vel)
{
	if(voice >= NUM_VOICES)
		return;
	pending[voice] = 1;
	notes[voice] = note;
	vels[voice] = vel;
}

//FUN_08013384
void triggers_init(void)
{
	for(uint8_t i = 0; i < NUM_VOICES; i++)
		pending[i] = 0;
	ready = 1;
}

//FUN_08013214. The stock image also skips muted voices (FUN_080087d0) and only pulses the
//LED outside one UI mode (FUN_080087c0); the module UI has neither yet.
void triggers_dispatch(void)
{
	if(!ready)
		return;
	for(uint8_t v = 0; v < NUM_VOICES; v++)
	{
		if(pending[v] != 1)
			continue;
		pending[v] = 0;
		if(v < 3)
			Drum_trigger(v, vels[v], notes[v]);
		else if(v == 3)
			Snare_trigger(vels[v], notes[v]);
		else if(v == 4)
			Cymbal_trigger(vels[v], notes[v]);
		else
			HiHat_trigger(vels[v], v == 6, notes[v]);
		panel_ledPulse(MODULE_BUTTON_VOICE1 + v);
	}
}

static void trigger_isr(uint8_t voice)
{
	const uint8_t accent = (GPIOD->IDR & pins[voice].accentPin) == 0;
	trigger_lastVel = accent ? VEL_ACCENT : VEL_NORMAL;
	trigger_noteOn(voice, TRIGGER_NOTE, trigger_lastVel);
}

void EXTI9_5_IRQHandler(void)
{
	if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_9))
	{
		__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_9);
		trigger_isr(6);
	}
}

void EXTI15_10_IRQHandler(void)
{
	for(uint8_t v = 0; v < 6; v++)
	{
		if(__HAL_GPIO_EXTI_GET_IT(pins[v].trigPin))
		{
			__HAL_GPIO_EXTI_CLEAR_IT(pins[v].trigPin);
			trigger_isr(v);
		}
	}
}

//diagnostics: raw MCU level of the accent pins in jack order ACC1..ACC6 (1 = pin high)
void triggers_accentPins(char out[7])
{
	static const uint16_t accentJacks[6] = {GPIO_PIN_8, GPIO_PIN_6, GPIO_PIN_4, GPIO_PIN_5, GPIO_PIN_3, GPIO_PIN_7};
	const uint32_t idr = GPIOD->IDR;
	for(uint8_t i = 0; i < 6; i++)
		out[i] = (idr & accentJacks[i]) ? '1' : '0';
	out[6] = 0;
}
