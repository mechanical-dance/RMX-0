#pragma once
#include <string.h>
#include <stdint.h>
#include <stdio.h>
extern uint8_t lcd_process_display_interrupt;
extern char lcd_last[64]; static inline void lcd_clear(void){lcd_last[0]=0;} static inline void lcd_home(void){}
static inline void lcd_setcursor(uint8_t a,uint8_t b){}
extern char lcd_last[64];
static inline void lcd_string(const char*s){strncat(lcd_last,s,20);strcat(lcd_last,"|");}
