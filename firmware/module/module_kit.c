/*
 * lxr-0 module firmware: module kits.
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Module kits are numbered 0..127 in /proj00, named "NN-NAME.SND" (3 digits from 100), and
 * are 255 bytes (desktop kit: 8-byte name + END_OF_SOUND_PARAMETERS) or 309 bytes (plus the
 * 54-byte mod matrix: 18 destinations, 18 amounts, 18 sources; PHX-D01 docs/module.md).
 * The desktop's preset_loadDrumsetSD can't be used directly: it stores kit n in slot n of
 * 64 and formats numbers with two digits. This mirrors it (preset_makeFileNameLoad,
 * preset_readDrumsetData, Core/Src/Preset/presetManager.c) with the kit always in slot 0.
 * Read-only: nothing here writes to the card.
 */
#include "main.h"
#include "fatfs.h"
#include "Parameters.h"
#include "Preset/PresetManager.h"
#include "menu/menu.h"
#include "menu/CcNr2Text.h"
#include "module.h"
#include "Soundengine/cs4344_cs5343.h"
#include <stdio.h>
#include <string.h>

_Static_assert(8 + END_OF_SOUND_PARAMETERS == 255, "desktop kit size");

uint8_t moduleKit_matrix[MODULE_MATRIX_BYTES];
uint8_t moduleKit_hasMatrix = 0;
uint8_t moduleKit_current = 0;

static uint8_t isKitSize(FSIZE_t size);

static FRESULT openKit(uint8_t kitNr)
{
	char pattern[13];
	DIR dj;
	FILINFO fno;
	snprintf(pattern, sizeof pattern, "%02u-*.snd", kitNr);
	FRESULT fr = f_findfirst(&dj, &fno, "", pattern);
	f_closedir(&dj);
	if(fr != FR_OK || !fno.fname[0])
		return FR_NO_FILE;
	return f_open((FIL *)&SDFile, fno.fname, FA_OPEN_EXISTING | FA_READ);
}

FRESULT moduleKit_load(uint8_t kitNr)
{
	unsigned int bytesRead;
	uint8_t *para = mainParameters[0];

	FRESULT res = openKit(kitNr);
	if(res != FR_OK)
		return res;
	//only 255- and 309-byte kits; the stock firmware also writes 302-byte scratch files
	//(TMP.SND, 100-TMP.SND) whose layout isn't decoded
	if(!isKitSize(f_size((FIL *)&SDFile)))
	{
		f_close((FIL *)&SDFile);
		return FR_NO_FILE;
	}

	//the main loop doesn't render while the card is read: silence the DMA buffers so the
	//codec doesn't loop the last block (they're in DTCM, not cached)
	memset(dma_buffer, 0, sizeof dma_buffer);
	memset(dma_buffer2, 0, sizeof dma_buffer2);

	res = f_read((FIL *)&SDFile, (void *)savefileNames[0][KIT_NAMES], 8, &bytesRead);
	if(res == FR_OK && bytesRead != 8)
		res = FR_DISK_ERR;
	if(res == FR_OK)
		res = f_read((FIL *)&SDFile, para, END_OF_SOUND_PARAMETERS, &bytesRead);
	if(res == FR_OK)
	{
		if(END_OF_SOUND_PARAMETERS - bytesRead)
			memset(para + bytesRead, 0, END_OF_SOUND_PARAMETERS - bytesRead);
		const uint8_t nmt = getNumModTargets();
		for(uint8_t i = 0; i < 6; i++)
		{
			if(para[PAR_VEL_DEST_1 + i] >= nmt)
				para[PAR_VEL_DEST_1 + i] = 0;
			if(para[PAR_TARGET_LFO1 + i] >= nmt)
				para[PAR_TARGET_LFO1 + i] = 0;
		}
		res = f_read((FIL *)&SDFile, moduleKit_matrix, MODULE_MATRIX_BYTES, &bytesRead);
		moduleKit_hasMatrix = res == FR_OK && bytesRead == MODULE_MATRIX_BYTES;
		if(!moduleKit_hasMatrix)
			memset(moduleKit_matrix, 0, MODULE_MATRIX_BYTES);
		res = FR_OK;
	}
	f_close((FIL *)&SDFile);

	if(res == FR_OK)
	{
		moduleKit_current = kitNr;
		preset_loadDrumsetRam(0, 0, 1);
	}
	return res;
}

//which kit numbers exist, from one directory scan at boot, so browsing doesn't read the card
static uint8_t kitIndex[MODULE_NUM_KITS / 8];
static char kitFileName[MODULE_NUM_KITS][9];

static uint8_t isKitSize(FSIZE_t size)
{
	return size == 8 + END_OF_SOUND_PARAMETERS || size == 8 + END_OF_SOUND_PARAMETERS + MODULE_MATRIX_BYTES;
}

void moduleKit_scan(void)
{
	DIR dj;
	FILINFO fno;
	memset(kitIndex, 0, sizeof kitIndex);
	if(f_findfirst(&dj, &fno, "", "*.snd") != FR_OK)
		return;
	while(fno.fname[0])
	{
		unsigned n = 0;
		const char *p = fno.fname;
		while(*p >= '0' && *p <= '9')
			n = n * 10 + (unsigned)(*p++ - '0');
		if(p != fno.fname && *p == '-' && n < MODULE_NUM_KITS && isKitSize(fno.fsize))
		{
			kitIndex[n / 8] |= (uint8_t)(1 << (n % 8));
			uint8_t j = 0;
			for(p++; *p && *p != '.' && j < 8; p++)
				kitFileName[n][j++] = *p;
			kitFileName[n][j] = 0;
		}
		if(f_findnext(&dj, &fno) != FR_OK)
			break;
	}
	f_closedir(&dj);
}

uint8_t moduleKit_exists(uint8_t kitNr)
{
	return kitNr < MODULE_NUM_KITS && (kitIndex[kitNr / 8] >> (kitNr % 8)) & 1;
}

//the name part of the file name (the card's 8.3 name), for browsing
const char *moduleKit_fileName(uint8_t kitNr)
{
	return moduleKit_exists(kitNr) ? kitFileName[kitNr] : "";
}

//the next existing kit from kitNr in direction dir, wrapping over 0..127
int16_t moduleKit_next(int16_t kitNr, int8_t dir)
{
	for(uint8_t tries = 0; tries < MODULE_NUM_KITS; tries++)
	{
		kitNr = (int16_t)((kitNr + MODULE_NUM_KITS + dir) % MODULE_NUM_KITS);
		if(moduleKit_exists((uint8_t)kitNr))
			return kitNr;
	}
	return moduleKit_current;
}

//the last kit, stored by the stock firmware as one byte in /state.idx
uint8_t moduleKit_lastUsed(void)
{
	unsigned int bytesRead;
	uint8_t idx = 0;
	if(f_open((FIL *)&SDFile, "/state.idx", FA_OPEN_EXISTING | FA_READ) != FR_OK)
		return 0;
	if(f_read((FIL *)&SDFile, &idx, 1, &bytesRead) != FR_OK || bytesRead != 1)
		idx = 0;
	f_close((FIL *)&SDFile);
	return idx < MODULE_NUM_KITS ? idx : 0;
}
