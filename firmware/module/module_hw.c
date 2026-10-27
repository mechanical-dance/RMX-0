/*
 * lxr-0 module firmware: clocks and peripheral init.
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * The peripheral inits are the desktop's (Core/Src/main.c, MX_*_Init), without SPI1 (shift
 * registers), USART3 (MIDI) and USB, which the stock module doesn't use. GPIO is the
 * module's (docs/HARDWARE_MAP_MODULE.md; mirrors the stock image's GPIO block at
 * 0x800bc10..0x800bc70, addresses refer to LXRE_V2_V1.5).
 */
#include "main.h"
#include "module.h"

ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;
I2S_HandleTypeDef hi2s2;
I2S_HandleTypeDef hi2s3;
DMA_HandleTypeDef hdma_spi2_tx;
DMA_HandleTypeDef hdma_spi3_tx;
RNG_HandleTypeDef hrng;
SD_HandleTypeDef hsd1;
DMA_HandleTypeDef hdma_sdmmc1_rx;
DMA_HandleTypeDef hdma_sdmmc1_tx;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim14;
//not initialised (no MIDI port on the stock module); the instance is set so the MIDI
//driver's register accesses go to the (clock-gated) USART3 instead of address 0 = ITCM
UART_HandleTypeDef huart3 = { .Instance = USART3 };

//the desktop's SystemClock_Config without the USART3 clock (the stock module's
//FUN_0800b860 sets the same PLL: HSE 8 MHz, M8 N216 P2 Q9, overdrive, 216 MHz)
void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
	RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

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

	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SDMMC1 | RCC_PERIPHCLK_I2S | RCC_PERIPHCLK_CLK48;
	PeriphClkInitStruct.PLLI2S.PLLI2SN = 113;
	PeriphClkInitStruct.PLLI2S.PLLI2SP = RCC_PLLP_DIV2;
	PeriphClkInitStruct.PLLI2S.PLLI2SR = 5;
	PeriphClkInitStruct.PLLI2S.PLLI2SQ = 2;
	PeriphClkInitStruct.PLLI2SDivQ = 1;
	PeriphClkInitStruct.I2sClockSelection = RCC_I2SCLKSOURCE_PLLI2S;
	PeriphClkInitStruct.Clk48ClockSelection = RCC_CLK48SOURCE_PLL;
	PeriphClkInitStruct.Sdmmc1ClockSelection = RCC_SDMMC1CLKSOURCE_CLK48;
	if(HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
		Error_Handler();
}

void MX_DMA_Init(void)
{
	__HAL_RCC_DMA2_CLK_ENABLE();
	__HAL_RCC_DMA1_CLK_ENABLE();

	HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, 5, 0);
	HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);
	HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 5, 0);
	HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
	HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 3, 0);
	HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
	HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);
	HAL_NVIC_SetPriority(DMA2_Stream6_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA2_Stream6_IRQn);
}

static void i2s_init(I2S_HandleTypeDef *h, SPI_TypeDef *instance)
{
	h->Instance = instance;
	h->Init.Mode = I2S_MODE_MASTER_TX;
	h->Init.Standard = I2S_STANDARD_PHILIPS;
	h->Init.DataFormat = I2S_DATAFORMAT_16B;
	h->Init.MCLKOutput = I2S_MCLKOUTPUT_ENABLE;
	h->Init.AudioFreq = I2S_AUDIOFREQ_44K;
	h->Init.CPOL = I2S_CPOL_HIGH;
	h->Init.ClockSource = I2S_CLOCK_PLL;
	if(HAL_I2S_Init(h) != HAL_OK)
		Error_Handler();
}

void MX_I2S2_Init(void) { i2s_init(&hi2s2, SPI2); }
void MX_I2S3_Init(void) { i2s_init(&hi2s3, SPI3); }

void MX_RNG_Init(void)
{
	hrng.Instance = RNG;
	if(HAL_RNG_Init(&hrng) != HAL_OK)
		Error_Handler();
}

//the HAL_SD_Init itself is done by FatFs (BSP_SD_Init)
void MX_SDMMC1_SD_Init(void)
{
	hsd1.Instance = SDMMC1;
	hsd1.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
	hsd1.Init.ClockBypass = SDMMC_CLOCK_BYPASS_DISABLE;
	hsd1.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_ENABLE;
	hsd1.Init.BusWide = SDMMC_BUS_WIDE_1B;
	hsd1.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_ENABLE;
	hsd1.Init.ClockDiv = 1;
}

//renders audio while a save blocks the main loop (USE_INTERRUPT_ONLY_DURING_SAVE_PROJ)
void MX_TIM2_Init(void)
{
	TIM_ClockConfigTypeDef sClockSourceConfig = {0};
	TIM_MasterConfigTypeDef sMasterConfig = {0};

	htim2.Instance = TIM2;
	htim2.Init.Prescaler = 1;
	htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim2.Init.Period = 1500;
	htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if(HAL_TIM_Base_Init(&htim2) != HAL_OK)
		Error_Handler();
	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if(HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
		Error_Handler();
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if(HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
		Error_Handler();
}

//the desktop's 8 kHz tick (systick_ticks)
void MX_TIM14_Init(void)
{
	TIM_OC_InitTypeDef sConfigOC = {0};

	htim14.Instance = TIM14;
	htim14.Init.Prescaler = 13500;
	htim14.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim14.Init.Period = 1;
	htim14.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim14.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if(HAL_TIM_Base_Init(&htim14) != HAL_OK)
		Error_Handler();
	if(HAL_TIM_OC_Init(&htim14) != HAL_OK)
		Error_Handler();
	sConfigOC.OCMode = TIM_OCMODE_TOGGLE;
	sConfigOC.Pulse = 0;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
	if(HAL_TIM_OC_ConfigChannel(&htim14, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
		Error_Handler();
}

//the stock module's GPIO block (0x800bc10..0x800bc70), as in bringup/main.c, plus the
//trigger EXTI: falling edge, pull-up (0x800bc60: pins 0xfe00, mode 0x210000)
void MX_GPIO_Init(void)
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

	//encoder switch, SD detect, accents: inputs with pull-up
	g.Mode = GPIO_MODE_INPUT;
	g.Pull = GPIO_PULLUP;
	g.Pin = 0x8000; HAL_GPIO_Init(GPIOE, &g);
	g.Pin = 0x01FA; HAL_GPIO_Init(GPIOD, &g);

	//triggers: falling-edge EXTI with pull-up
	g.Mode = GPIO_MODE_IT_FALLING;
	g.Pin = 0xFE00; HAL_GPIO_Init(GPIOD, &g);

	//EXTI15_10 at priority 0, EXTI9_5 at 3 (stock NVIC setup)
	HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
	HAL_NVIC_SetPriority(EXTI9_5_IRQn, 3, 0);
}
