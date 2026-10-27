/*
 * lxr-0 module firmware: exception and interrupt handlers.
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * The desktop's Core/Src/stm32f7xx_it.c without the USB, SPI1 and front-panel handlers.
 * The trigger EXTI handlers are in module_triggers.c.
 */
#include "main.h"
#include "Soundengine/Soundengine.h"
#include "Soundengine/Sequencer/sequencer.h"
#include "Soundengine/MIDI/Uart.h"
#include "module.h"
#include "encoder.h"

extern DMA_HandleTypeDef hdma_adc1;
extern DMA_HandleTypeDef hdma_spi2_tx;
extern DMA_HandleTypeDef hdma_spi3_tx;
extern DMA_HandleTypeDef hdma_sdmmc1_rx;
extern DMA_HandleTypeDef hdma_sdmmc1_tx;
extern SD_HandleTypeDef hsd1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim14;
extern volatile uint32_t systick_ticks;

void NMI_Handler(void) { while(1); }
void HardFault_Handler(void) { while(1); }
void MemManage_Handler(void) { while(1); }
void BusFault_Handler(void) { while(1); }
void UsageFault_Handler(void) { while(1); }
void SVC_Handler(void) {}
void DebugMon_Handler(void) {}
void PendSV_Handler(void) {}

//the encoder is polled here, as in the desktop and the stock module (vec_15)
void SysTick_Handler(void)
{
	HAL_IncTick();
	encoder_callback();
}

void DMA1_Stream4_IRQHandler(void) { HAL_DMA_IRQHandler(&hdma_spi2_tx); }
void DMA1_Stream5_IRQHandler(void) { HAL_DMA_IRQHandler(&hdma_spi3_tx); }

//render while a save blocks the main loop; the desktop's handler without its debug pin
//(PE2 is the SELECT 1 button here)
void TIM2_IRQHandler(void)
{
#if CALC_SAMPLES_IN_INTERRUPT
	seq_tick();
	soundengine_calcNextSampleBlock();
	uart_processMidi();
#endif
	__HAL_TIM_CLEAR_IT(&htim2, TIM_IT_UPDATE);
}

void TIM8_TRG_COM_TIM14_IRQHandler(void)
{
	HAL_TIM_IRQHandler(&htim14);
	systick_ticks++;
	//the desktop runs menu_tick 4x per tick; the display refresh is the part the module uses
	for(uint8_t i = 0; i < 4; i++)
		display_tick();
}

void SDMMC1_IRQHandler(void) { HAL_SD_IRQHandler(&hsd1); }
void DMA2_Stream0_IRQHandler(void) { HAL_DMA_IRQHandler(&hdma_adc1); }
void DMA2_Stream3_IRQHandler(void) { HAL_DMA_IRQHandler(&hdma_sdmmc1_rx); }
void DMA2_Stream6_IRQHandler(void) { HAL_DMA_IRQHandler(&hdma_sdmmc1_tx); }
