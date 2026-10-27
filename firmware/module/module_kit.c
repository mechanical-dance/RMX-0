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
#include <stdio.h>
#include <string.h>

_Static_assert(8 + END_OF_SOUND_PARAMETERS == 255, "desktop kit size");

uint8_t moduleKit_matrix[MODULE_MATRIX_BYTES];
uint8_t moduleKit_hasMatrix = 0;
uint8_t moduleKit_current = 0;

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

//kit n+dir, skipping missing numbers, wrapping over 0..127
FRESULT moduleKit_step(int8_t dir)
{
	uint8_t n = moduleKit_current;
	for(uint8_t tries = 0; tries < MODULE_NUM_KITS; tries++)
	{
		n = (uint8_t)((n + MODULE_NUM_KITS + dir) % MODULE_NUM_KITS);
		if(moduleKit_load(n) == FR_OK)
			return FR_OK;
	}
	return FR_NO_FILE;
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
