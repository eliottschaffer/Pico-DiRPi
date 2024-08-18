#ifndef SETTING_H
#define SETTING_H

#include <stdio.h>
#include "hardware/i2c.h"
#include "pico/binary_info.h"
#include <time.h>
#include <stdlib.h>
#include "sd_card.h"
#include "ff.h"
#include "pico/stdlib.h"
#include <string.h>
#include "pico/cyw43_arch.h"

#include "struct.h"

#define I2C_PORT i2c1

extern int addr;
extern uint8_t OE;
extern bool LOW;
extern bool HIGH;
extern bool CH1;
extern bool CH2;

extern int mem_depth;

extern uint8_t data0[2];

extern uint32_t event;

extern uint32_t run_number;

extern FRESULT fr;
extern FATFS fs;
extern FIL fil;
extern int ret;
extern char filename[];

extern uint8_t threshold[2];
extern uint8_t nQuiet[2];
extern uint8_t nMinWidth[2];
extern uint8_t iStartSearch;
extern uint8_t pulsenum;

extern uint8_t Npulse[2];


extern char myBuff[256];
extern char datetime_buf[256];
extern char *datetime_str;


extern struct setting_t run_setting;

extern struct pulsedata_t pulsedata_Ch1;
extern struct pulsedata_t pulsedata_Ch2;


// WIFI Credentials - take care if pushing to github!
extern char ssid[];
extern char pass[];
extern uint32_t country;
extern uint32_t auth;

extern datetime_t t;


#endif