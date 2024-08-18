#include "util.h"
#include "struct.h"
#include "settings.h"

#include <stdio.h>
#include "hardware/i2c.h"
#include "pico/binary_info.h"
#include "pico/cyw43_arch.h"
#include <time.h>
#include <stdlib.h>
#include "sd_card.h"
#include "ff.h"
#include "pico/stdlib.h"
#include "pico/util/datetime.h"
#include "hardware/rtc.h"
#include "lwipopts.h"
#include "lwip/apps/httpd.h"
#include "lwip/apps/HTTP_Client.h"
#include <string.h>

int addr = 0x20;
uint8_t OE = 17;
bool LOW = 0;
bool HIGH = 1;

uint32_t run_number = 1;


bool CH1 = 0;
bool CH2 = 1;

int mem_depth = 4096;

uint32_t event = 1;

FRESULT fr;
FATFS fs;
FIL fil;
int ret;
char filename[] = "test01.txt";

uint8_t threshold[2] = {100,100};
uint8_t nQuiet[2] = {5,5};
uint8_t nMinWidth[2] = {4,4};
uint8_t iStartSearch = 5;
uint8_t pulsenum = 0;

uint8_t Npulse[2];

uint8_t data0[2];

char myBuff[256];
char datetime_buf[256];
char *datetime_str = &datetime_buf[0];


// WIFI Credentials - take care if pushing to github!
char ssid[] = "OnePlus7";
char pass[] = "12345678";
uint32_t country = CYW43_COUNTRY_USA;
uint32_t auth = CYW43_AUTH_WPA2_MIXED_PSK;

