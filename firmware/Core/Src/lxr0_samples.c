/*
 * lxr0_samples.c - lxr-0: sample flash table and SD installer (see lxr0_samples.h).
 *
 * Flash (sectors 6-11, single bank): header word pair, table, then the audio. The header is written
 * last, so an interrupted install reads as "no samples". A firmware update erases these sectors too.
 *
 * Copyright (C) 2026 lxr-0, on top of the LXR firmware (C) 2012-2026 Erica Synths
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "lxr0_samples.h"

#if LXR0_SAMPLES
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "stm32f7xx_hal.h"
#include "fatfs.h"
#include "lcd.h"
#include "main.h"
#include "menu/menu.h"
#include "Preset/PresetManager.h"
#include "Soundengine/cs4344_cs5343.h"

#ifndef LXR0_SAMPLE_FLASH_BASE
#define LXR0_SAMPLE_FLASH_BASE 0x08080000u	// sector 6
#endif
#define SAMPLES_SIZE	0x180000u		// sectors 6-11, 1.5 MB
#define SAMPLES_END			(LXR0_SAMPLE_FLASH_BASE + SAMPLES_SIZE)
#define TABLE_MAGIC			0x3052584cu		// "LXR0"
#define TABLE_ADDR			(LXR0_SAMPLE_FLASH_BASE + 8u)
#define DATA_ADDR			(TABLE_ADDR + LXR0_SAMPLE_MAX*sizeof(Lxr0Sample))
#define PAD_BYTES			4u				// zeros after each sample, read by the interpolation
#define IO_FRAMES			512u
#define PATH_SIZE			24	// "/samples/" + 8.3 name
#define MAX_FRAME_BYTES		6u				// stereo 24 bit

static uint8_t numSamples;

uint8_t lxr0Samples_init(void)
{
	const uint32_t* header = (const uint32_t*)LXR0_SAMPLE_FLASH_BASE;
	numSamples = 0;
	if(header[0] != TABLE_MAGIC || header[1] > LXR0_SAMPLE_MAX)
		return 0;

	const Lxr0Sample* table = (const Lxr0Sample*)TABLE_ADDR;
	for(uint32_t i=0; i<header[1]; i++)
	{
		const uintptr_t start = (uintptr_t)table[i].data;
		if(start < DATA_ADDR || start >= SAMPLES_END || (start & 3u) || table[i].frames == 0
				|| table[i].frames > (SAMPLES_END - start - PAD_BYTES)/2u || table[i].rate == 0)
			break;
		numSamples++;
	}
	return numSamples;
}

const Lxr0Sample* lxr0Samples_get(uint8_t waveform)
{
	const uint8_t i = (uint8_t)(waveform - LXR0_SAMPLE_WAVE_START);
	if(waveform < LXR0_SAMPLE_WAVE_START || i >= numSamples)
		return 0;
	return &((const Lxr0Sample*)TABLE_ADDR)[i];
}

//--------------------------------- installer ---------------------------------

typedef struct
{
	char		file[13];
	uint8_t		channels;
	uint8_t		bytesPerSample;
	uint8_t		looped;
	uint32_t	dataOffset;
	uint32_t	frames;
	uint32_t	rate;
} InstallEntry;

static InstallEntry entries[LXR0_SAMPLE_MAX];
static Lxr0Sample table[LXR0_SAMPLE_MAX];
static uint8_t ioBuffer[IO_FRAMES*MAX_FRAME_BYTES] __attribute__((aligned(4)));
static uintptr_t writeAddr;
static uint32_t pendingWord;
static uint8_t pendingHalf;
static uint8_t foldersFound;
static FRESULT scanError;

static uint16_t le16(const uint8_t* p) { return (uint16_t)(p[0] | (p[1]<<8)); }
static uint32_t le32(const uint8_t* p) { return p[0] | (p[1]<<8) | (p[2]<<16) | ((uint32_t)p[3]<<24); }

static void show(const char* line1, const char* line2)
{
	lcd_clear();
	lcd_home();
	lcd_string(line1);
	lcd_setcursor(0,2);
	lcd_string(line2);
}

static void makePath(char* path, const char* folder, const char* file)
{
	snprintf(path, PATH_SIZE, "%s/%s", folder, file);
}

// reads the fmt and data chunks. 0 if this isn't a PCM wav of 1 or 2 channels at 16 or 24 bit
static uint8_t parseWav(const char* path, InstallEntry* e)
{
	uint8_t b[26];
	UINT n;
	uint8_t ok = 0, haveFmt = 0, haveData = 0;
	uint32_t dataBytes = 0;

	if(f_open(&SDFile, path, FA_READ) != FR_OK)
		return 0;

	if(f_read(&SDFile, b, 12, &n) != FR_OK || n != 12 || memcmp(b, "RIFF", 4) || memcmp(&b[8], "WAVE", 4))
		goto done;

	while(!(haveFmt && haveData))
	{
		if(f_read(&SDFile, b, 8, &n) != FR_OK || n != 8)
			goto done;
		const uint32_t size = le32(&b[4]);
		const uint32_t pos = (uint32_t)f_tell(&SDFile);
		const uint32_t fileSize = (uint32_t)f_size(&SDFile);
		const uint8_t isFmt = !memcmp(b, "fmt ", 4);
		const uint8_t isData = !memcmp(b, "data", 4);

		if(isFmt)
		{
			const UINT want = size < sizeof(b) ? (UINT)size : sizeof(b);
			if(size < 16 || f_read(&SDFile, b, want, &n) != FR_OK || n != want)
				goto done;
			uint16_t format = le16(b);
			if(format == 0xFFFE && want >= 26) // WAVE_FORMAT_EXTENSIBLE: the sub format comes first in its GUID
				format = le16(&b[24]);
			e->channels = (uint8_t)le16(&b[2]);
			e->rate = le32(&b[4]);
			e->bytesPerSample = (uint8_t)(le16(&b[14])/8);
			if(format != 1 || e->channels < 1 || e->channels > 2 || e->rate == 0
					|| (e->bytesPerSample != 2 && e->bytesPerSample != 3))
				goto done;
			haveFmt = 1;
		}
		else if(isData)
		{
			e->dataOffset = pos;
			dataBytes = size > fileSize - pos ? fileSize - pos : size; // truncated or streamed files
			haveData = 1;
		}

		if(size > fileSize - pos)
		{
			if(!isData) goto done;
			continue; // the data chunk runs to the end of the file
		}
		if(f_lseek(&SDFile, pos + size + (size & 1u)) != FR_OK)
			goto done;
	}

	e->frames = dataBytes / (e->channels * e->bytesPerSample);
	ok = e->frames != 0;

done:
	f_close(&SDFile);
	return ok;
}

// adds the folder's wavs in name order, so waveform numbers don't depend on the directory order.
// Two passes: _FS_LOCK is 1, so the folder must be closed before a file can be opened.
static uint8_t scanFolder(const char* folder, uint8_t looped, uint8_t count, uint16_t* skipped)
{
	DIR dir;
	FILINFO info;
	char path[PATH_SIZE];
	const uint8_t first = count;

	const FRESULT res = f_opendir(&dir, folder);
	if(res == FR_NO_PATH || res == FR_NO_FILE)
		return count;
	if(res != FR_OK)
	{
		scanError = res;
		return count;
	}
	foldersFound++;

	while(f_readdir(&dir, &info) == FR_OK && info.fname[0])
	{
		const size_t len = strlen(info.fname);
		if((info.fattrib & (AM_DIR | AM_HID)) || len < 5 || len > 12 || strcasecmp(&info.fname[len-4], ".wav"))
			continue; // AM_HID: the "._" files macOS leaves next to each file
		if(count >= LXR0_SAMPLE_MAX)
		{
			(*skipped)++;
			continue;
		}

		uint8_t at = count;
		while(at > first && strcmp(entries[at-1].file, info.fname) > 0)
		{
			entries[at] = entries[at-1];
			at--;
		}
		memset(&entries[at], 0, sizeof(entries[at]));
		strcpy(entries[at].file, info.fname);
		entries[at].looped = looped;
		count++;
	}
	f_closedir(&dir);

	uint8_t out = first;
	for(uint8_t i=first; i<count; i++)
	{
		makePath(path, folder, entries[i].file);
		if(!parseWav(path, &entries[i]))
		{
			(*skipped)++;
			continue;
		}
		if(out != i)
			entries[out] = entries[i];
		out++;
	}
	return out;
}

static uint64_t flashBytes(const InstallEntry* e)
{
	return ((uint64_t)e->frames*2u + PAD_BYTES + 3u) & ~(uint64_t)3u;
}

// drops what doesn't fit, keeping the order
static uint8_t fitToFlash(uint8_t count, uint16_t* skipped)
{
	uintptr_t addr = DATA_ADDR;
	uint8_t out = 0;
	for(uint8_t i=0; i<count; i++)
	{
		const uint64_t end = (uint64_t)addr + flashBytes(&entries[i]);
		if(end > SAMPLES_END)
		{
			(*skipped)++;
			continue;
		}
		addr = (uintptr_t)end;
		entries[out++] = entries[i];
	}
	return out;
}

static uint8_t programWord(uintptr_t addr, uint32_t value)
{
	if(addr < LXR0_SAMPLE_FLASH_BASE || addr >= SAMPLES_END || (addr & 3u))
		return 0;
	return HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, value) == HAL_OK;
}

static uint8_t putSample(int16_t s)
{
	if(!pendingHalf)
	{
		pendingWord = (uint16_t)s;
		pendingHalf = 1;
		return 1;
	}
	pendingHalf = 0;
	const uint8_t ok = programWord(writeAddr, pendingWord | ((uint32_t)(uint16_t)s << 16));
	writeAddr += 4;
	return ok;
}

// 16 bit of the frame at p: the top two bytes of each sample, both channels averaged
static int16_t mixFrame(const uint8_t* p, uint8_t channels, uint8_t bytesPerSample)
{
	const uint8_t skip = (uint8_t)(bytesPerSample - 2);
	int32_t v = (int16_t)le16(&p[skip]);
	if(channels == 2)
		v = (v + (int16_t)le16(&p[bytesPerSample + skip])) / 2;
	return (int16_t)v;
}

static uint8_t installOne(const char* folder, const InstallEntry* e, Lxr0Sample* out)
{
	char path[PATH_SIZE];
	const uint32_t frameBytes = (uint32_t)e->channels * e->bytesPerSample;
	uint32_t remaining = e->frames;
	uint8_t ok = 0;

	makePath(path, folder, e->file);
	if(f_open(&SDFile, path, FA_READ) != FR_OK)
		return 0;
	if(f_lseek(&SDFile, e->dataOffset) != FR_OK)
		goto done;

	out->data = (const int16_t*)writeAddr;
	out->frames = e->frames;
	out->rate = e->rate;
	out->looped = e->looped;
	memset(out->name, ' ', sizeof(out->name));
	for(uint8_t i=0; i<sizeof(out->name) && e->file[i] && e->file[i] != '.'; i++)
		out->name[i] = e->file[i];

	while(remaining)
	{
		const uint32_t frames = remaining > IO_FRAMES ? IO_FRAMES : remaining;
		UINT n;
		if(f_read(&SDFile, ioBuffer, (UINT)(frames*frameBytes), &n) != FR_OK || n != frames*frameBytes)
			goto done;
		for(uint32_t i=0; i<frames; i++)
			if(!putSample(mixFrame(&ioBuffer[i*frameBytes], e->channels, e->bytesPerSample)))
				goto done;
		remaining -= frames;
	}

	if(pendingHalf && !putSample(0)) // fill the word
		goto done;
	if(!programWord(writeAddr, 0))
		goto done;
	writeAddr += PAD_BYTES;
	ok = 1;

done:
	f_close(&SDFile);
	return ok;
}

static uint8_t eraseSamples(void)
{
	FLASH_EraseInitTypeDef erase;
	uint32_t sectorError = 0;
	memset(&erase, 0, sizeof(erase));
	erase.TypeErase = FLASH_TYPEERASE_SECTORS;
	erase.Sector = FLASH_SECTOR_6;
	erase.NbSectors = 6;
	erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
	return HAL_FLASHEx_Erase(&erase, &sectorError) == HAL_OK;
}

static uint8_t writeTable(uint8_t count)
{
	const uint32_t* words = (const uint32_t*)table;
	for(uint32_t i=0; i<count*sizeof(Lxr0Sample)/4u; i++)
		if(!programWord(TABLE_ADDR + i*4u, words[i]))
			return 0;
	return programWord(LXR0_SAMPLE_FLASH_BASE + 4u, count)
		&& programWord(LXR0_SAMPLE_FLASH_BASE, TABLE_MAGIC);
}

// an install that can't start: the message, then back to the menu with audio still running
static void refuse(const char* line1, const char* line2)
{
	show(line1, line2);
	HAL_Delay(3000);
	HAL_TIM_Base_Stop_IT(&htim2);
	lcd_process_display_interrupt = 1;
	menu_lxr0RedrawScreen();
}

// Audio runs from the timer while the card is scanned, as it does during a kit load. Only an install that
// goes ahead stops it, and only that one resets (which restarts audio, caches and the card). The current
// kit is autosaved first, so the reset brings it back as a power cycle does.
void lxr0Samples_installFromSd(void)
{
	char line[17];
	uint16_t skipped = 0;
	uint8_t ok = 0;

	lcd_process_display_interrupt = 0;
	while(sendDisplayBuffer() != 0);
	HAL_TIM_Base_Start_IT(&htim2);

	show("Samples", "Scanning...");
	foldersFound = 0;
	scanError = FR_OK;
	const uint8_t scanned = scanFolder("/loops", 1, scanFolder("/samples", 0, 0, &skipped), &skipped);
	const uint8_t count = fitToFlash(scanned, &skipped);

	if(count == 0)
	{
		if(scanError != FR_OK)
			snprintf(line, sizeof(line), "Card error %u", scanError);
		else if(foldersFound == 0)
			snprintf(line, sizeof(line), "No /samples dir");
		else
			snprintf(line, sizeof(line), "%u wavs unusable", skipped);
		refuse(line, "Flash unchanged");
		return;
	}
	if(!(FLASH->OPTCR & FLASH_OPTCR_nDBANK)) // sector numbers below are the single bank layout
	{
		refuse("Flash dual bank", "Flash unchanged");
		return;
	}
	if(preset_saveTmpKit() != FR_OK) // also stops the timer
	{
		HAL_TIM_Base_Start_IT(&htim2);
		refuse("Kit not saved", "Flash unchanged");
		return;
	}

	HAL_TIM_Base_Stop_IT(&htim2);
	memset(dma_buffer, 0, sizeof(dma_buffer));
	memset(dma_buffer2, 0, sizeof(dma_buffer2));
	HAL_I2S_DMAStop(&hi2s2);
	HAL_I2S_DMAStop(&hi2s3);

	show("Samples", "Erasing flash...");
	HAL_FLASH_Unlock();
	if(eraseSamples())
	{
		writeAddr = DATA_ADDR;
		pendingHalf = 0;
		ok = 1;
		for(uint8_t i=0; i<count && ok; i++)
		{
			snprintf(line, sizeof(line), "%u/%u", i+1, count);
			show(line, entries[i].file);
			ok = installOne(entries[i].looped ? "/loops" : "/samples", &entries[i], &table[i]);
		}
		ok = ok && writeTable(count);
	}
	HAL_FLASH_Lock();

	if(ok)
	{
		snprintf(line, sizeof(line), "%u ok %u skip", count, skipped);
		show("Samples done", line);
	}
	else
	{
		show("Install failed", "No samples now");
	}
	HAL_Delay(3000);
	NVIC_SystemReset();
}
#endif
