#ifndef STRUCT_H
#define STRUCT_H

#include <stdio.h>
#include <stdlib.h>
#include "pico/binary_info.h"
#include <string.h>
#include <time.h>
#include "lwipopts.h"
#include "pico/cyw43_arch.h"

struct setting_t
{
    bool TrgCh1;
    bool TrgCh2;
    bool sfttrg;
    bool extrg;
    bool Prescale;
    bool Position;
    uint8_t PotVal1;
    uint8_t PotVal2;
    uint8_t PotVal3;
    uint8_t PotVal4;
    uint8_t DACVal1;
    uint8_t DACVal2;
    uint8_t clckspeed;
    uint32_t time;
    uint8_t temp;
    uint32_t Trg1Cnt;
    uint32_t Trg2Cnt;
};



typedef struct NTP_T_ {
    ip_addr_t ntp_server_address;
    bool dns_request_sent;
    struct udp_pcb *ntp_pcb;
    absolute_time_t ntp_test_time;
    alarm_id_t ntp_resend_alarm;
} NTP_T;


#endif