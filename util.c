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


static uint8_t MX0 = 18;
static uint8_t MX1 = 14;
static uint8_t MX2 =12;
static uint8_t MX1OUT = 11;
static uint8_t MX2OUT = 19;

char buf[100];


int compression(uint8_t* raw_data, uint8_t* compressed_data){
    if(raw_data[0] == 255) {
        compressed_data[0] =254;
    }else{
        compressed_data[0] = raw_data[0];
    }
    uint16_t nSame = 0;
    uint16_t compressed_counter = 1;
    for(uint16_t i = 1; i < mem_depth; i++){
        if(raw_data[i] == 255){
            raw_data[i] = 254;
        }
        if(raw_data[i] == raw_data[i-1] && (nSame<250)){ //Same
            nSame++;
        }else{ //Not same
        if(nSame == 0){ //different sequantial values
            compressed_data[compressed_counter] = raw_data[i];
            compressed_counter++;
            
        }else if (nSame > 0 && nSame < 4){  //short sequence
            for(uint8_t j = 0; j < nSame; j++){
                compressed_data[compressed_counter] = raw_data[i - 1];
                compressed_counter++;
            }
            compressed_data[compressed_counter] = raw_data[i];
            compressed_counter++;
            }else{
            
            compressed_data[compressed_counter] = raw_data[i-1];
            compressed_counter++;

            compressed_data[compressed_counter] = 255;
            compressed_counter++;
            
            compressed_data[compressed_counter] = nSame;
            compressed_counter++;

            compressed_data[compressed_counter] = raw_data[i];
            compressed_counter++;

        }
        nSame = 0;
    }

    }
    //Write Final Sequences
    if(nSame == 0){ //different sequantial values
        compressed_data[compressed_counter] = raw_data[mem_depth];
        compressed_counter++;
        
    }else if (nSame > 0 && nSame < 4){  //short sequence
        for(uint8_t j = 0; j < nSame; j++){
            compressed_data[compressed_counter] = raw_data[mem_depth - 1];
            compressed_counter++;
        }
        compressed_data[compressed_counter] = raw_data[mem_depth];
        compressed_counter++;
    }else{
        
        compressed_data[compressed_counter] = raw_data[mem_depth-1];
        compressed_counter++;

        compressed_data[compressed_counter] = 255;
        compressed_counter++;
        
        compressed_data[compressed_counter] = nSame;
        compressed_counter++;

        compressed_data[compressed_counter] = raw_data[mem_depth];
        compressed_counter++;

    }
    return compressed_counter;
}

void DaqHalt(bool state){
    gpio_init(15);
    gpio_set_dir(15, GPIO_OUT);
    gpio_put(15, state);
    return;
}


void SLWCK(bool state){
    gpio_init(16);
    gpio_set_dir(16, GPIO_OUT);
    gpio_put(16, state);
    return;
}


void MRpulse(void){
    gpio_init(26);
    gpio_set_dir(26, GPIO_OUT);
    gpio_put(26, 1);
    sleep_ms(100);
    gpio_put(26, 0);
    return;
}



void i2c_start(void){
        // Initialize I2C interface
    i2c_init(I2C_PORT, 400 * 1000);  // Initialize I2C1 at 400 kHz
    gpio_set_function(2, GPIO_FUNC_I2C); // Set SDA pin function
    gpio_set_function(3, GPIO_FUNC_I2C); // Set SCL pin function
    gpio_pull_up(2);
    gpio_pull_up(3);
    return;
}


void  *data_acc(uint8_t  *data1, uint8_t  *data2){

    gpio_init(MX0);
    gpio_set_dir(MX0, GPIO_OUT);
    gpio_init(MX1);
    gpio_set_dir(MX1, GPIO_OUT);
    gpio_init(MX2);
    gpio_set_dir(MX2, GPIO_OUT);    

    gpio_init(MX1OUT);
    gpio_set_dir(MX1OUT, GPIO_IN);


    int counter = 0;


    while(counter < mem_depth){
        

        bool  bitsMX1[8];
        bool  bitsMX2[8];

        for (int iMUX = 0; iMUX<8; iMUX++) {
            SetMUXCode(iMUX);
            bool read1_1 = gpio_get(MX1OUT);
            bool read1_2 = gpio_get(MX1OUT);

            bool read2_1 = gpio_get(MX2OUT);
            bool read2_2 = gpio_get(MX2OUT);

            if (read1_1 == read1_2){
                bitsMX1[iMUX] = read1_1;
            } else{
                bitsMX1[iMUX] = 255;
            }

            if(read2_1 == read2_2){
                bitsMX2[iMUX] = read2_1;
            }else{
                bitsMX2[iMUX] = 255;
            }
        
        
        }

        /* Single Read Code
        for (int iMUX = 0; iMUX<8; iMUX++) {
            SetMUXCode(iMUX);
            bitsMX1[iMUX] = gpio_get(MX1OUT);

            bitsMX2[iMUX]= gpio_get(MX2OUT);

        }
        */


        bool dataBits[2][8];
            
        // Fix the bit scrammbling done to ease trace routing
        dataBits[0][0] = bitsMX1[3];
        dataBits[0][1] = bitsMX1[0];
        dataBits[0][2] = bitsMX1[1];
        dataBits[0][3] = bitsMX1[2];
        dataBits[0][4] = bitsMX1[4];
        dataBits[0][5] = bitsMX1[6];
        dataBits[0][6] = bitsMX1[7];
        dataBits[0][7] = bitsMX1[5];

        dataBits[1][0] = bitsMX2[3];
        dataBits[1][1] = bitsMX2[0];
        dataBits[1][2] = bitsMX2[1];
        dataBits[1][3] = bitsMX2[2];
        dataBits[1][4] = bitsMX2[5];
        dataBits[1][5] = bitsMX2[7];
        dataBits[1][6] = bitsMX2[6];
        dataBits[1][7] = bitsMX2[4];

    // Extract the ADC values from the bits
        uint8_t  dataCH1 = 0;
        uint8_t  dataCH2 = 0;
        uint8_t  datascale = 1; // scale by 3300 /255 in readout
        for (int i=0; i<8; i++) {
            dataCH1 += dataBits[0][i]*datascale;
            dataCH2 += dataBits[1][i]*datascale;
            datascale *= 2;
        }
        
        data1[counter] = dataCH1;
        data2[counter] = dataCH2;
        counter++;

        SLWCK_Cycle();
    }
    printf("All bytes gotten\n");
    return 0;

}


void SetMUXCode(uint8_t  val) {
    if (val < 0 || val > 7) {
        printf("error in MUX code");
        return;
    }
    
    uint8_t  MXCODEA = 0;
    uint8_t  MXCODEB = 0;
    uint8_t  MXCODEC = 0;
    
    // Crude hack to set MUX code
    MXCODEA = val % 2;
    
    if (val > 3) {
        MXCODEB = (val - 4) / 2;
    } else {
        MXCODEB = val / 2;
    }
    
    MXCODEC = val / 4;
    
    gpio_put(MX0, MXCODEA);
    gpio_put(MX1, MXCODEB);
    gpio_put(MX2, MXCODEC);

    return;
}

void SLWCK_Cycle(void){
    SLWCK(LOW);
    SLWCK(HIGH);
    return;
}

bool SD_init(void){

        // Initialize SD card
    if (!sd_init_driver()) {
        printf("ERROR: Could not initialize SD card\r\n");
        return 0;
    }

    return 1;
}

bool SD_mount(bool value){
    
    if (value == true){
    // Mount drive
    fr = f_mount(&fs, "0:", 1);
    if (fr != FR_OK) {
        printf("ERROR: Could not mount filesystem (%d)\r\n", fr);
        return 1;
    }
    }else{
        // Unmount drive
        f_unmount("0:");
    }
    return 0;
}

void Header_setup(void){
    //Settings Header
    ret = f_printf(&fil,
        "TrgCh1 %d\n"
        "TrgCh2 %d\n"
        "sftrg %d\n"
        "extrg %d\n"
        "Prescale %d\n"
        "Position %d\n"
        "PotValCh1 %u\n"
        "PotValCh2 %u\n"
        "PotValCh3 %u\n"
        "PotValCh4 %u\n"
        "DACValCh1 %u\n"
        "DACValCh2 %u\n"
        "clckspeed %u\n"
        "TIME %u\n"
        "TEMP %u\n"
        "Trig1Cnt %u\n"
        "Trig1Cnt %u\n",
        run_setting.TrgCh1,
        run_setting.TrgCh2,
        run_setting.sfttrg,
        run_setting.extrg,
        run_setting.Prescale,
        run_setting.Position,
        run_setting.PotVal1,
        run_setting.PotVal2,
        run_setting.PotVal3,
        run_setting.PotVal4,
        run_setting.DACVal1,
        run_setting.DACVal2,
        run_setting.clckspeed,
        run_setting.time,
        run_setting.temp,
        run_setting.Trg1Cnt,
        run_setting.Trg2Cnt);
    
    if (ret < 0) {
        printf("ERROR: Could not write to file (%d)\r\n", ret);
        f_close(&fil);
        while (true); // Handle error
    }
}


void Prescale(bool state){
    if(!state){
        gpio_init(28);
        gpio_set_dir(28, GPIO_OUT);
        gpio_put(28, state);

        gpio_init(10);
        gpio_set_dir(10, GPIO_OUT);
        gpio_put(10, 1);
        printf("ISDA set high");
    }else{
        gpio_init(28);
        gpio_set_dir(28, GPIO_OUT);
        gpio_put(28, state);
    }
    
    run_setting.Prescale = state;
    return;
}

void Thr_DAQ_Set(uint8_t chan, uint16_t voltage){


    uint16_t DACval = (4095 * voltage) / 3300;
    
    if (DACval > 4095) {
        DACval = 4095;
    }

    uint8_t highByte = (voltage >> 8) & 0xFF; // Upper 8 bits
    uint8_t lowByte = voltage & 0xFF;         // Lower 8 bits

    printf("Upper 8 bits: 0x%02X\nLower 8 Bits (Hex): 0x%02X\n", highByte, lowByte);

    // Rest of your function code...
    data0[0] = highByte; // Upper 8 bits
    data0[1] = lowByte;        // Lower 8 bits

    switch (chan)
    {
    case 1:
        i2c_write_blocking(I2C_PORT, 97, data0, sizeof(data0), false);
        run_setting.DACVal1 = voltage;
        break;
    case 2:
        i2c_write_blocking(I2C_PORT, 97, data0, sizeof(data0), false);
        run_setting.DACVal2 = voltage;
        break;
    default:
        break;
    }
}

void Bias_DAC_Set(uint8_t chan, uint16_t voltage){
    uint16_t DACval = (4095 * voltage) / 3300;
    
    if (DACval > 4095) {
        DACval = 4095;
    }

    uint8_t highByte = (voltage >> 8) & 0xFF; // Upper 8 bits
    uint8_t lowByte = voltage & 0xFF;         // Lower 8 bits

    printf("Upper 8 bits: 0x%02X\nLower 8 Bits (Hex): 0x%02X\n", highByte, lowByte);

    // Rest of your function code...
    data0[0] = highByte; // Upper 8 bits
    data0[1] = lowByte;        // Lower 8 bits

    switch (chan)
    {
    case 1:
        i2c_write_blocking(I2C_PORT, 76, data0, sizeof(data0), false);
        run_setting.DACVal1 = voltage;
        break;
    case 2:
        i2c_write_blocking(I2C_PORT, 97, data0, sizeof(data0), false);
        run_setting.DACVal2 = voltage;
        break;
    default:
        break;
    }
}

void DigitPot(uint8_t chan, uint8_t N){
    uint8_t write_byte = (N & 0xFF);
    switch (chan)
    {
    case 1:
        data0[0] = 0x00;
        data0[1] = write_byte;
        i2c_write_blocking(I2C_PORT, 44, data0, sizeof(data0), false);
        run_setting.PotVal1 = N;
        break;
    case 2:
        data0[0] = 0x10;
        data0[1] = write_byte;
        i2c_write_blocking(I2C_PORT, 44, data0, sizeof(data0), false);
        run_setting.PotVal2 = N;
        break;
    case 3:
        data0[0] = 0x60;
        data0[1] = write_byte;
        i2c_write_blocking(I2C_PORT, 44, data0, sizeof(data0), false);
        run_setting.PotVal3 = N;
        break;
    case 4:
        data0[0] = 0x70;
        data0[1] = write_byte;
        i2c_write_blocking(I2C_PORT, 44, data0, sizeof(data0), false);
        run_setting.PotVal4 = N;
        break;        
    default:
        break;
    }
}


void GPIO_expader(uint8_t speed_Mhz, uint8_t Position , uint16_t local_mem_depth){

    switch (speed_Mhz)
    {
    case 20:
        run_setting.clckspeed = speed_Mhz;
        speed_Mhz = 0x02;
        break;
    case 40:
        run_setting.clckspeed = speed_Mhz;
        speed_Mhz = 0x01;
        break;
    default:
        break;
        speed_Mhz = 0x00;
    }    
    
    printf("position value = %d", Position);
    if (Position == 1){
        run_setting.Position = Position;
        Position = 0x10;
    } else if (Position == 2){
        printf("position loop = 2");
        run_setting.Position = Position;
        Position = 0x08; 
    }else{
        Position = 0x00;
    }

    if (local_mem_depth > 4096) {
        // Change Memdepth, on V2 is not possible/pico cant handle possibly.
        // mem_depth = 0x04; 
    } else {
        // local_mem_depth = 0x00;
    }

    uint8_t io_setting = speed_Mhz | Position;

    // unsigned io_setting = {
    //     speed_Mhz | 
    //     local_mem_depth  | 
    //     Position
    // };

    data0[0] = 0x03;
    data0[1] = 0x1F;
    i2c_write_blocking(I2C_PORT, 33, data0, sizeof(data0), false);
    data0[0] = 0x01;
    data0[1] = 0x00;
    i2c_write_blocking(I2C_PORT, 33, data0, sizeof(data0), false);
    sleep_us(3);
    data0[0] = 0x01;
    data0[1] = io_setting;
    i2c_write_blocking(I2C_PORT, 33, data0, sizeof(data0), false);
    data0[0] = 0x03;
    data0[1] = 0x00;
    i2c_write_blocking(I2C_PORT, 33, data0, sizeof(data0), false);
}


void Trigger_Set(bool Ch1, bool Ch2, bool extTrig){

    uint8_t val_ext = 0;
    uint8_t val_1 = 0;
    uint8_t val_2 = 0;
    run_setting.TrgCh1 = 0;
    run_setting.TrgCh2 = 0;
    run_setting.extrg = 0;

    if(Ch1 == true){
        val_1= 0x02;
        run_setting.TrgCh1 = 1;
    }

    if(Ch2 == true){
        val_2 = 0x40;
        run_setting.TrgCh2 = 1;
    }

    if(extTrig == true){
        val_ext = 0x01;
        run_setting.extrg = 1;
    }

    uint8_t iosetting = val_1 | val_2 | val_ext;


    data0[0] = 0x03;
    data0[1] = 0x1F;
    i2c_write_blocking(I2C_PORT, 32, data0, sizeof(data0), false);
    data0[0] = 0x01;
    data0[1] = 0x00;
    i2c_write_blocking(I2C_PORT, 32, data0, sizeof(data0), false);
    sleep_us(3);
    data0[0] = 0x01;
    data0[1] = iosetting;
    i2c_write_blocking(I2C_PORT, 32, data0, sizeof(data0), false);
    data0[0] = 0x03;
    data0[1] = 0x00;
    i2c_write_blocking(I2C_PORT, 32, data0, sizeof(data0), false);
}


uint32_t get_run_num(void){
    SD_mount(true);
    // Open file for reading
    fr = f_open(&fil, "run_number.txt", FA_WRITE | FA_READ);

    //If not opening correctly will create the file and then initilize it to 1
    if (fr != FR_OK) {
        
        printf("No file found, creating\n");

        // Create the file if it doesn't exist
        if (f_open(&fil, "run_number.txt", FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) {
            printf("Failed to create %s.\n",  "run_number.txt");
            f_mount(NULL, "", 0);  // Unmount the file system
            return 1;
        }
    }else{
        printf("File found, Getting value\n");
        // Read the current run number
        f_gets(buf, sizeof(buf), &fil);
        run_number = atoi(buf);
    }

    // Rewind the file pointer to the beginning of the file
    f_lseek(&fil, 0);

    // Write the incremented run number back to the file
    f_printf(&fil, "%d",run_number + 1);

    // Close the run number file
    f_close(&fil);

    // Unmount the file system
    f_mount(NULL, "", 0);

    return run_number;

}