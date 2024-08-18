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
#include <string.h>
#include <math.h>
#include "lwip/apps/httpd.h"
#include "lwip/apps/HTTP_Client.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#include "struct.h"
#include "util.h"
#include "net_util.h"
#include "settings.h"
#include "ws.h"

#define TEST_TCP_SERVER_IP "192.168.45.197"
#define TCP_PORT 8080


#define TCP_DISCONNECTED 0
#define TCP_CONNECTING   1
#define TCP_CONNECTED    2


uint8_t id[2] = {0x00, 0x01};




static err_t tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
    printf("tcp_client_sent %u\n", len);
    return ERR_OK;
}

static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err) {
    TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
    if (err != ERR_OK) {
        printf("Connect failed %d\n", err);
    // return tcp_result(arg, err);
}

// Write HTTP GET Request with Websocket upgrade 
state->buffer_len = sprintf((char*)state->buffer, "GET /data HTTP/1.1\r\nHost: 192.168.45.197:8080\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==\r\nSec-WebSocket-Protocol: chat, superchat\r\nSec-WebSocket-Version: 13\r\n\r\n");
err = tcp_write(state->tcp_pcb, state->buffer, state->buffer_len, TCP_WRITE_FLAG_COPY);

state->connected = TCP_CONNECTED;

printf("Connected\r\n");
    return ERR_OK;
}

static err_t tcp_client_poll(void *arg, struct tcp_pcb *tpcb) {
    printf("tcp_client_poll\n");
    return ERR_OK;
}

static void tcp_client_err(void *arg, err_t err) {
    TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
    state->connected = TCP_DISCONNECTED;
    if (err != ERR_ABRT) {
        printf("tcp_client_err %d\n", err);
    } else {
        printf("tcp_client_err abort %d\n", err);
    }
}

static err_t tcp_client_close(void *arg) {
    TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
    err_t err = ERR_OK;
    if (state->tcp_pcb != NULL) {
        tcp_arg(state->tcp_pcb, NULL);
        tcp_poll(state->tcp_pcb, NULL, 0);
        tcp_sent(state->tcp_pcb, NULL);
        tcp_recv(state->tcp_pcb, NULL);
        tcp_err(state->tcp_pcb, NULL);
        err = tcp_close(state->tcp_pcb);
        if (err != ERR_OK) {
            printf("close failed %d, calling abort\n", err);
            tcp_abort(state->tcp_pcb);
            err = ERR_ABRT;
        }
        state->tcp_pcb = NULL;
    }
    state->connected = TCP_DISCONNECTED;
    return err;
}

err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    printf("recv \r\n");
    TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
    if (!p) {
        // return tcp_result(arg, -1);
    }

    // this method is callback from lwIP, so cyw43_arch_lwip_begin is not required, however you
    // can use this method to cause an assertion in debug mode, if this method is called when
    // cyw43_arch_lwip_begin IS needed
    cyw43_arch_lwip_check();
    state->rx_buffer_len = 0;

    if(p == NULL) {
        // Close
        tcp_client_close(arg);
        return ERR_OK;
    }

    if (p->tot_len > 0) {
        for (struct pbuf *q = p; q != NULL; q = q->next) {
            if((state->rx_buffer_len + q->len) < BUF_SIZE) {
                WebsocketPacketHeader_t header;
                ParsePacket(state->tcp_pcb, &header, (char *)q->payload, q->len);
                memcpy(state->rx_buffer + state->rx_buffer_len, (uint8_t *)q->payload + header.start, header.length);
                state->rx_buffer_len += header.length;
            }
        }
        printf("tcp_recved \r\n");
        tcp_recved(tpcb, p->tot_len);
    }
    pbuf_free(p);

    return ERR_OK;
}

static err_t connect_wifi(void *arg) {
    TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;

    if(state->connected != TCP_DISCONNECTED) return ERR_OK;

    state->tcp_pcb = tcp_new_ip_type(IP_GET_TYPE(&state->remote_addr));
    if (!state->tcp_pcb) {
        return false;
    }

    tcp_arg(state->tcp_pcb, state);
    tcp_poll(state->tcp_pcb, tcp_client_poll, 1);
    tcp_sent(state->tcp_pcb, tcp_client_sent);
    tcp_recv(state->tcp_pcb, tcp_client_recv);
    tcp_err(state->tcp_pcb, tcp_client_err);

    state->buffer_len = 0;
    // cyw43_arch_lwip_begin/end should be used around calls into lwIP to ensure correct locking.
    // You can omit them if you are in a callback from lwIP. Note that when using pico_cyw_arch_poll
    // these calls are a no-op and can be omitted, but it is a good practice to use them in
    // case you switch the cyw43_arch type later.
    cyw43_arch_lwip_begin();
    state->connected = TCP_CONNECTING;
    err_t err = tcp_connect(state->tcp_pcb, &state->remote_addr, TCP_PORT, tcp_client_connected);
    cyw43_arch_lwip_end();
        
    return ERR_ABRT;
}




// char text[BUF_SIZE] = "Disconnected";


struct setting_t run_setting = {
    .TrgCh1 = 0,
    .TrgCh2 = 0,
    .sfttrg = 0,
    .extrg = 0,
    .Prescale = 0,
    .Position = 0,
    .PotVal1 = 0,
    .PotVal2 = 0,
    .PotVal3 = 0,
    .PotVal4 = 0,
    .DACVal1 = 0,
    .DACVal2 = 0,
    .clckspeed = 0,
    .time = 0,
    .temp = 0,
    .Trg1Cnt = 0,
    .Trg1Cnt = 0
};


datetime_t t;



int main(){
    stdio_init_all();

    //Wifi and Socket Setup
    // Connect to network
    char ssid[] = "OnePlus7";
    char pass[] = "12345678";


    printf("Before Connectiong\n");

    if (cyw43_arch_init_with_country(CYW43_COUNTRY_USA)) {
        printf("failed to initialise\n");
        return 1;
    }

    cyw43_arch_enable_sta_mode();

    if (cyw43_arch_wifi_connect_timeout_ms(ssid, pass, CYW43_AUTH_WPA2_MIXED_PSK, 10000)) {
        return 1;
    }

    printf("Before TCP settup\n");

    // Set up TCP
    TCP_CLIENT_T *state = (TCP_CLIENT_T *)calloc(1, sizeof(TCP_CLIENT_T));
    if (!state) {
        printf("failed to allocate state\n");
        return false;
    }

    printf("After TCP settup\n");

    ip4addr_aton(TEST_TCP_SERVER_IP, &state->remote_addr);

    printf("Connecting to %s port %u\n", ip4addr_ntoa(&state->remote_addr), TCP_PORT);
    connect_wifi(state);

    sleep_ms(10000);


    printf("Before ID sending\n");



    //Identification, Uses Identification frame, here ID is 0x00 0x01 = 1
    if(state->connected == TCP_CONNECTED){
        printf("sending ID frame");
        state->buffer_len = BuildPacket((char *)state->buffer, BUF_SIZE, WEBSOCKET_OPCODE_BIN, id, sizeof(id), 1, 0x01);
        err_t err = tcp_write(state->tcp_pcb, state->buffer, state->buffer_len, TCP_WRITE_FLAG_COPY);
        if (err != ERR_OK) {
            printf("Failed to write data %d\n", err);
        }
    }

    while(state->connected != TCP_CONNECTED) {
            printf("making sure ID is sent\n"); 
            connect_wifi(state);
            if(state->connected == TCP_CONNECTED){
            printf("sending ID frame");
            state->buffer_len = BuildPacket((char *)state->buffer, BUF_SIZE, WEBSOCKET_OPCODE_BIN, id, sizeof(id), 1, 0x01);
            err_t err = tcp_write(state->tcp_pcb, state->buffer, state->buffer_len, TCP_WRITE_FLAG_COPY);
            if (err != ERR_OK) {
                printf("Failed to write data %d\n", err);
            }
            break;
        }
    }




    // Set up NTP and get the time once
    ntp_setup();

    sleep_ms(3000);


    while(!SD_init()){
        printf("Initialation failed\n");
        sleep_ms(5000);
    }


    get_run_num();
    
    printf("We are on Run %u\n", run_number);

    


    i2c_start();

    Trigger_Set(1,0,0);

    Thr_DAQ_Set(1,10);

    Thr_DAQ_Set(2,300);


    // Bias_DAC_Set(1,0);  Calibration DAC
    // Bias_DAC_Set(2,0);

    gpio_init(OE);
    gpio_set_dir(OE, GPIO_IN);
    gpio_pull_up(OE);
    
    Prescale(HIGH);

    //Pedistal
    DigitPot(1, 255);
    DigitPot(2, 255);
    
    //SIPM Bias
    DigitPot(3, 85);
    DigitPot(4, 85);


    //DAQ, Test for calibration

    printf("before gpio expander");
    GPIO_expader(40, 1, 4095);


    SLWCK(HIGH);
    MRpulse();
    DaqHalt(LOW);


    sleep_ms(5000);

    printf("Entering Loop\n");


    while (true) {

        bool OE_val = gpio_get(OE);

        // printf("Getting OE val\n");
        if (OE_val == LOW) {
            DaqHalt(HIGH);

            printf("event %d\n", event);

            uint32_t Start_Loop =  time_us_32();


            printf("before rtc\n");

            rtc_get_datetime(&t);

            printf("before epoch_time\n");
            set_epoch_time(&t);

            printf("OE* Flipped\n");
            
            printf("before malloc\n");

            // Allocate memory for the data array using malloc
            uint8_t  *CH1_raw = (uint8_t  *)malloc(mem_depth * sizeof(uint8_t ));
                        // Allocate memory for the data array using malloc
            uint8_t  *CH2_raw = (uint8_t  *)malloc(mem_depth * sizeof(uint8_t ));

            // Ensure memory allocation was successful
            if (CH1_raw == NULL || CH2_raw == NULL) {
                printf("Memory allocation failed\n");
                return 1;
            }


            printf("before data_acc\n");

            uint32_t Start_Data_acc =  time_us_32();
            data_acc(CH1_raw, CH2_raw);
            uint32_t End_Data_acc =  time_us_32();
        
            free(CH1_raw);
            free(CH2_raw);

            
            uint32_t Start_Compression =  time_us_32();

            uint8_t  *CH1_data = (uint8_t  *)malloc(mem_depth * sizeof(uint8_t ));
            uint8_t  *CH2_data = (uint8_t  *)malloc(mem_depth * sizeof(uint8_t ));

            
            // Ensure memory allocation was successful
            if (CH1_data == NULL || CH2_data == NULL) {
                printf("Memory allocation failed\n");
                return 1;
            }


            uint16_t ch1_length = compression(CH1_raw, CH1_data);
            uint16_t ch2_length = compression(CH2_raw, CH2_data);

            
            printf("Ch1 = %u\n Ch2 = %u\n",ch1_length, ch2_length);


            uint32_t End_Compression =  time_us_32();

            uint32_t Start_Sending =  time_us_32();

            if(state->connected == TCP_CONNECTED) {
                // write to tcp
                state->buffer_len = BuildPacket((char *)state->buffer, BUF_SIZE, WEBSOCKET_OPCODE_BIN, CH1_data, ch1_length, 1, 0x02);
                err_t err = tcp_write(state->tcp_pcb, state->buffer, state->buffer_len, TCP_WRITE_FLAG_COPY);
                if (err != ERR_OK) {
                    printf("Failed to write data %d\n", err);
                }
                tcp_output(state->tcp_pcb); // Trigger an immediate send
                //Ch2
                state->buffer_len = BuildPacket((char *)state->buffer, BUF_SIZE, WEBSOCKET_OPCODE_BIN, CH2_data, ch2_length, 1, 0x02);
                err = tcp_write(state->tcp_pcb, state->buffer, state->buffer_len, TCP_WRITE_FLAG_COPY);
                if (err != ERR_OK) {
                    printf("Failed to write data %d\n", err);
                }
                tcp_output(state->tcp_pcb);
            }

            uint32_t End_Sending =  time_us_32();

            event ++;    

            free(CH1_data);
            free(CH2_data);
            
            uint32_t END_Loop =  time_us_32();

            uint32_t time_loop = END_Loop - Start_Loop;
            uint32_t time_data = End_Data_acc - Start_Data_acc;
            uint32_t time_compressing = End_Compression - Start_Compression;
            uint32_t time_sending = End_Sending - Start_Sending;

            printf("full loop time: %u \n data time: %u \n compression time %u \n Sending Time: %u\n", time_loop, time_data, time_compressing, time_sending);

            printf("Exiting Loop\n");
            SLWCK(HIGH);
            MRpulse();
            DaqHalt(LOW);
        } else {       
            // Print buffer contents and reset it
            if(state->rx_buffer_len) {
                printf("%.*s \r\n", state->rx_buffer_len, (char*)state->rx_buffer);
                state->rx_buffer_len = 0;
            }
            if(state->connected == TCP_DISCONNECTED) {        
                // Reconnect
                connect_wifi(state);
            }

            sleep_ms(10);  // Adjust this interval based on your requirements
        }
    }
    return 0;

}
