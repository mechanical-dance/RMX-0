#pragma once
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
typedef enum {HAL_OK,HAL_ERROR} HAL_StatusTypeDef;
typedef struct {uint32_t TypeErase,Sector,NbSectors,VoltageRange;} FLASH_EraseInitTypeDef;
#define FLASH_TYPEERASE_SECTORS 0
#define FLASH_SECTOR_6 6
#define FLASH_VOLTAGE_RANGE_3 2
#define FLASH_TYPEPROGRAM_WORD 2
#define FLASH_OPTCR_nDBANK (1u<<29)
struct fl{uint32_t OPTCR;}; extern struct fl flash_regs; 
#define FLASH (&flash_regs)
HAL_StatusTypeDef HAL_FLASH_Program(uint32_t,uintptr_t,uint64_t);
HAL_StatusTypeDef HAL_FLASHEx_Erase(FLASH_EraseInitTypeDef*,uint32_t*);
#define HAL_FLASH_Unlock() ((void)0)
#define HAL_FLASH_Lock() ((void)0)
#define HAL_TIM_Base_Stop_IT(x) ((void)0)
#define HAL_I2S_DMAStop(x) ((void)0)
#define HAL_Delay(x) ((void)0)
void NVIC_SystemReset(void) __attribute__((noreturn));
