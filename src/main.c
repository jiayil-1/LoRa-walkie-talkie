#include "include.h"
#include "microphone.h"
//#include "lora.h"
// #include <stdio.h>
// #include <string.h>
// #include "pico/stdlib.h"
// #include "hardware/timer.h"
// #include "hardware/irq.h"
// #include "hardware/adc.h"
// #include "hardware/regs/dma.h"
// #include <hardware/structs/dma.h>
// #include "microphone.c"
// #include "lora.c"

// adc for mic pin -> rp2350
// spi rp2350 -> lora pin

volatile lora_state_t state = STATE_RX;
#define CHUNK_SIZE 60

void init_spi() {
    //initialize gpio pins
    uint32_t mask = (1 << 17) | (1 << 18) | (1 << 19) | (1 << 20);
    gpio_init_mask(mask);

    //spi0 CSn
    gpio_set_function(17, GPIO_FUNC_SPI);
    //spi0 SCK
    gpio_set_function(18, GPIO_FUNC_SPI);
    //spi0 TX
    gpio_set_function(19, GPIO_FUNC_SPI);
    //spi0 RX
    gpio_set_function(20, GPIO_FUNC_SPI);

    //spi format
    spi_init(spi0, 1000000); //1MHz
    spi_set_format(spi0, 16, 0, 0, SPI_MSB_FIRST);
}

static volatile bool tx_done = true;

void on_packet_sent(void) {
    tx_done = true;
}


int main() {
    stdio_init_all();
    init_adc_dma();
    init_pb_irq();
    lora_init_default();

    int16_t status = rfm9x_begin_fsk(915.0, 300.0, 75.0, 250.0, 10, 16);
    if (status != 0) {
        return -1;
    }

    rfm9x_set_packet_sent_action(on_packet_sent);
    //rfm9x_start_receive();

    printf("Starting");
    
    for(;;) {
        //if(state == STATE_TX) {
            //uint8_t sample = adc_fifo_out >> 4; // convert 12-bit to 8-bit
            

        uint8_t *chunk = adc_get_ready_chunk();
        if (chunk != NULL && tx_done) {
            tx_done = false;
            int16_t rc = rfm9x_start_transmit(chunk, CHUNK_SIZE);
            if (rc != 0) {
                tx_done = true;   // allow retry on next chunk
                printf("TX ERR: %d\n", rc);
            }
            printf("TX: %02x %02x %02x %02x ...\n",
               chunk[0], chunk[1], chunk[2], chunk[3]);
        }
        
    }

    return 0;
}



// interrupts for non-blocking LORA send
// send microphone shit and see 
// LORA receive over radio see with oscilloscope

// see digital from RP2350 output from second board