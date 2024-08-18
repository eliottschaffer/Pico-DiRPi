#ifndef UTIL_H
#define UTIL_H

#include "struct.h"
#include "pico/util/datetime.h"


// Function prototypes
void SLWCK(bool state);
void MRpulse(void);
void DaqHalt(bool state);
void i2c_start(void);
void  *data_acc(uint8_t  *data1, uint8_t  *data2);
void SetMUXCode(uint8_t  val);
void SLWCK_Cycle(void);
bool SD_init(void);
bool  SD_mount(bool value);

void Header_setup(void);
int compression(uint8_t* raw_data, uint8_t* compressed_data);

int Findpulses(uint8_t* raw_data, bool chan);

void Prescale(bool state);
void DigitPot(uint8_t chan, uint8_t N);
void GPIO_expader(uint8_t speed_Mhz, uint8_t Position , uint16_t local_mem_depth);

void Trigger_Set(bool Ch1, bool Ch2, bool extTrig);

void Thr_DAQ_Set(uint8_t chan, uint16_t voltage);

void Bias_DAC_Set(uint8_t chan, uint16_t voltage);

uint32_t get_run_num(void);

#endif