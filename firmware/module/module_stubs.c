/*
 * lxr-0 module firmware: desktop features the module doesn't have.
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * TriggerOut.c (trigger outputs, clock input) and the menu helpers presetManager.c
 * calls. The number formatters are copied from Core/Src/menu/menu.c.
 */
#include "main.h"
#include "Soundengine/TriggerOut.h"

//TriggerOut.c: no trigger outputs or clock input on the module
uint8_t trigger_clockReceived = 0;
uint8_t trigger_dividerClockOut1 = 0;
uint8_t trigger_prescalerClockInput = 0;
void trigger_triggerVoice(uint8_t voice, triggerMode mode) { (void)voice; (void)mode; }
void trigger_clockTick(uint8_t onOff, uint8_t cnt) { (void)onOff; (void)cnt; }
void trigger_reset(uint8_t value) { (void)value; }
void trigger_ResetPosition() {}
void trigger_tickPhaseCounter() {}
uint8_t trigger_isGateModeOn() { return 0; }
void trigger_setGatemode(uint8_t onOff) { (void)onOff; }
void trigger_allOff() {}

//menu.c: the module's glo.cfg layout isn't decoded yet, so globals aren't applied (Phase 2b)
void menu_sendAllGlobals() {}

//menu.c
void numtostrpu_2Bytes(char *buf, uint8_t num, char pad)
{
    (void)pad;
    if(num > 99) {
        buf[0]='>';
        buf[1]='>';
        return;
    }
    if(num > 9) {
        buf[0]=(char)('0'+(num / 10));
        num %=10;
    }
    else
    {
        buf[0]='0';
    }
    buf[1]=(char)('0'+num);
}

//menu.c
void numtostrpu(char *buf, uint8_t num, char pad)
{
    if(num > 99) {
        buf[0]=(char)('0'+(num / 100));
        num %= 100;
    } else {
        buf[0]=pad;
    }
    if(num > 9) {
        buf[1]=(char)('0'+(num / 10));
        num %=10;
    } else if(buf[0]==pad) {
        buf[1]=pad;
    } else
        buf[1]='0';
    buf[2]=(char)('0'+num);
}
