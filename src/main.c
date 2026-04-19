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
    tx_done = true;
    tx_needs_finish = false;
    rx_done = false;
    rx_arm_needed = true;
    rx_packet_ready = false;
    memset((void *)tx_chunk_ready, 0, sizeof(tx_chunk_ready));
    memset((void *)rx_chunk_ready, 0, sizeof(rx_chunk_ready));
    lora_read_ind = 0;
    dma_write_ind = 0;
    rx_read_ind = 0;
    rx_write_ind = 0;

    lora_init_default();

    int16_t status = rfm9x_begin_fsk(433.0, 300.0, 75.0, 250.0, 10, 16);
    if (status != 0) {
        printf("bad lora init");
        return -1;
    }

    rfm9x_set_packet_sent_action(packet_sent_isr);
    rfm9x_set_packet_received_action(packet_received_isr);

    printf("Starting");
    state = STATE_RX;

    for(;;) {
        if(state == STATE_TX) {
            if (tx_needs_finish) {
                tx_needs_finish = false;
                int16_t rc = rfm9x_finish_transmit();
                if (rc != 0) {
                    printf("TX FINISH ERR: %d\n", rc);
                }
                tx_done = true;
            }
            if (tx_done && tx_enable && tx_chunk_ready[lora_read_ind]) {
                tx_done = false;

                int16_t rc = rfm9x_start_transmit(tx_ring[lora_read_ind], CHUNK_SIZE);
                if (rc != 0) {
                    tx_done = true;   // allow retry on next chunk
                    printf("TX ERR: %d\n", rc);
                } else {
                    tx_chunk_ready[lora_read_ind] = false;
                    lora_read_ind = (lora_read_ind + 1) % TX_RING_CHUNKS;
                }
                
            }
        } else if(state == STATE_RX) {
            if (rx_arm_needed) {
                rx_arm_needed = false;
                int16_t rc = rfm9x_start_receive();
                if (rc != 0) {
                    rx_arm_needed = true;   // allow retry
                    printf("RX ERR: %d\n", rc);
                }
            }
            if (rx_packet_ready) {
                rx_packet_ready = false;
                int16_t rc = rfm9x_read_data(rx_ring[rx_write_ind], CHUNK_SIZE);
                if (rc == 0) {
                    rx_chunk_ready[rx_write_ind] = true;
                    rx_write_ind = (rx_write_ind + 1) % RX_RING_CHUNKS;
                    printf("RX chunk[%u]: %02x %02x %02x %02x ...\n",
                           (unsigned)((rx_write_ind + RX_RING_CHUNKS - 1) % RX_RING_CHUNKS),
                           rx_ring[(rx_write_ind + RX_RING_CHUNKS - 1) % RX_RING_CHUNKS][0],
                           rx_ring[(rx_write_ind + RX_RING_CHUNKS - 1) % RX_RING_CHUNKS][1],
                           rx_ring[(rx_write_ind + RX_RING_CHUNKS - 1) % RX_RING_CHUNKS][2],
                           rx_ring[(rx_write_ind + RX_RING_CHUNKS - 1) % RX_RING_CHUNKS][3]);
                } else {
                    printf("READ ERR: %d\n", rc);
                }
                rx_arm_needed = true;
            }
        }
    }

    return 0;
}



// interrupts for non-blocking LORA send
// send microphone shit and see 
// LORA receive over radio see with oscilloscope

// see digital from RP2350 output from second board
