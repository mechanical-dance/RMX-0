/*
 * lxr-0 module firmware: non-blocking display refresh.
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Mirrors the desktop's sendDisplayBuffer (Core/Src/menu/menu.c), called from the TIM14
 * tick like menu_tick: the main loop writes editDisplayBuffer, the tick sends the
 * changed characters one LCD step at a time, so the audio render never waits on the
 * display. The cursor stays off (the module UI doesn't use one yet).
 */
#include "main.h"
#include "lcd.h"
#include "module.h"
#include <string.h>

static char currentDisplayBuffer[2][16];
static char editDisplayBuffer[2][17];
static volatile uint8_t display_refresh_state = 0;
static volatile uint8_t display_needs_refresh = 0;
static volatile uint8_t refresh_i, refresh_j;

void display_init(void)
{
	memset(editDisplayBuffer, ' ', sizeof editDisplayBuffer);
	memset(currentDisplayBuffer, 127, sizeof currentDisplayBuffer);
	display_needs_refresh = 1;
}

//row 0 or 1; pads with spaces to 16
void display_print(uint8_t row, const char *text)
{
	uint8_t j = 0;
	for(; j < 16 && text[j]; j++)
		editDisplayBuffer[row][j] = text[j];
	for(; j < 16; j++)
		editDisplayBuffer[row][j] = ' ';
	display_needs_refresh = 1;
}

void display_tick(void)
{
	switch(display_refresh_state)
	{
	case 0:
		if(display_needs_refresh)
		{
			display_refresh_state++;
			display_needs_refresh = 0;
		}
		break;
	case 1:
		if(lcd_turnOn_nonblock(1, 0))
		{
			display_refresh_state++;
			refresh_i = refresh_j = 0;
		}
		break;
	case 2:
		if(currentDisplayBuffer[refresh_i][refresh_j] != editDisplayBuffer[refresh_i][refresh_j])
		{
			if(editDisplayBuffer[refresh_i][refresh_j] == '\0')
				editDisplayBuffer[refresh_i][refresh_j] = ' ';
			display_refresh_state++;
		}
		else
		{
			refresh_j++;
			if(refresh_j >= 16)
			{
				refresh_j = 0;
				refresh_i++;
				if(refresh_i >= 2)
					display_refresh_state = 0;
			}
		}
		break;
	case 3:
		if(lcd_setcursor_nonBlock(refresh_j, (uint8_t)(refresh_i + 1)))
			display_refresh_state++;
		break;
	case 4:
		if(lcd_data_nonBlock(editDisplayBuffer[refresh_i][refresh_j]))
		{
			currentDisplayBuffer[refresh_i][refresh_j] = editDisplayBuffer[refresh_i][refresh_j];
			refresh_j++;
			display_refresh_state = 2;
			if(refresh_j >= 16)
			{
				refresh_j = 0;
				refresh_i++;
				if(refresh_i >= 2)
					display_refresh_state = 0;
			}
		}
		break;
	}
}
