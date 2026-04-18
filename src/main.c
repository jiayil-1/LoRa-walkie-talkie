#include "include.h"
#include "microphone.h"
#include "speaker.h"



// Set to 1 to bypass LoRa and just verify the mic -> ADC -> DMA path.
// Prints min/max/avg of each chunk plus a few raw samples over USB serial.


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


int main() {
    stdio_init_all();
    init_adc_dma();
    init_pb_irq();
    tx_done = false;
    rx_done = false;

    lora_init_default();

    int16_t status = rfm9x_begin_fsk(915.0, 300.0, 75.0, 250.0, 10, 16);
    if (status != 0) {
        printf("bad lora init");
        return -1;
    }

    rfm9x_set_packet_sent_action(packet_sent_isr);
    //rfm9x_set_packet_received_action(packet_received_isr);

    printf("Starting");
    state = STATE_TX;

    for(;;) {
        //if(state == STATE_TX) {
        if (tx_done && tx_enable) {
            tx_done = false;

            int16_t rc = rfm9x_start_transmit(&dma_buf[lora_read_ind], CHUNK_SIZE);
            //printf("dma_);
            if (rc != 0) {
                tx_done = true;   // allow retry on next chunk
                printf("TX ERR: %d\n", rc);
            }
            
        }/*
        } else if(state == STATE_RX) {
            if(rx_done) {
                rx_done = false;
                int16_t rc = rfm9x_start_receive();
                if (rc != 0) {
                    rx_done = true;   // allow retry on next chunk
                    printf("RX ERR: %d\n", rc);
                }
            }
        }*/
    }

    return 0;
}



// interrupts for non-blocking LORA send
// send microphone shit and see 
// LORA receive over radio see with oscilloscope

// see digital from RP2350 output from second board