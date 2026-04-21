#include "include.h"
#include "microphone.h"
#include "speaker.h"
int ptt = 21;

void init_spi()
{
    // initialize gpio pins
    uint32_t mask = (1 << 17) | (1 << 18) | (1 << 19) | (1 << 20);
    gpio_init_mask(mask);

    // spi0 CSn
    gpio_set_function(17, GPIO_FUNC_SPI);
    // spi0 SCK
    gpio_set_function(18, GPIO_FUNC_SPI);
    // spi0 TX
    gpio_set_function(19, GPIO_FUNC_SPI);
    // spi0 RX
    gpio_set_function(20, GPIO_FUNC_SPI);

    // spi format
    spi_init(spi0, 1000000); // 1MHz
    spi_set_format(spi0, 16, 0, 0, SPI_MSB_FIRST);
}

int main()
{
    stdio_init_all();
    setvbuf(stdout, NULL, _IONBF, 0);
    sleep_ms(300);
    gpio_init(16);
    gpio_set_dir(16, false);
    printf("BOOT: main entered\n");

    init_adc_dma();
    printf("BOOT: init_adc_dma done\n");

    init_pb_irq();
    printf("BOOT: init_pb_irq done\n");

    // CHANGES HERE
    init_pwm();
    init_dma_speaker();
    init_speaker_timer();
    // CHANGES END

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

    int16_t lora_status = lora_init_default();
    printf("BOOT: lora_init_default=%d\n", lora_status);

    int16_t ver_before = rfm9x_get_version();
    printf("BOOT: version_reg_before=0x%02x\n", (unsigned)(ver_before & 0xFF));

    int16_t status = rfm9x_begin_fsk(433.0, 300.0, 75.0, 250.0, 10, 16);
    printf("BOOT: rfm9x_begin_fsk=%d\n", status);

    if (status != 0)
    {
        int16_t ver_after = rfm9x_get_version();
        printf("ERR: version_reg_after=0x%02x\n", (unsigned)(ver_after & 0xFF));
        printf("ERR: bad lora init\n");
        while (true)
        {
            sleep_ms(1000);
        }
    }
    printf("BOOT: radio init complete\n");

    rfm9x_set_packet_sent_action(packet_rec_send_isr);
    printf("BOOT: packet sent ISR set\n");

    printf("BOOT: packet received ISR set\n");

    printf("BOOT: entering state RX\n");
    state = STATE_RX;

    for (;;)
    {
        if (state == STATE_TX)
        {
            if (tx_needs_finish)
            {
                tx_needs_finish = false;
                int16_t rc = rfm9x_finish_transmit();
                if (rc != 0)
                {
                    printf("TX FINISH ERR: %d\n", rc);
                }
                tx_done = true;
            }
            /*
            printf("TX gate: idx=%u ready=%u tx_done=%u tx_enable=%u\n",
            (unsigned)lora_read_ind,
            (unsigned)tx_chunk_ready[lora_read_ind],
            (unsigned)tx_done,
            (unsigned)tx_enable);
            */
            if (tx_done && tx_enable && tx_chunk_ready[lora_read_ind])
            {
                // printf("Starting transmit");
                int16_t rc = rfm9x_start_transmit(tx_ring[lora_read_ind], CHUNK_SIZE);
                tx_done = false;
                if (rc != 0)
                {
                    tx_done = true; // allow retry on next chunk
                    printf("TX ERR: %d\n", rc);
                }
                else
                {
                    tx_chunk_ready[lora_read_ind] = false;
                    lora_read_ind = (lora_read_ind + 1) % TX_RING_CHUNKS;
                }
            }
        }
        else if (state == STATE_RX)
        {

            if (rx_arm_needed)
            {
                rx_arm_needed = false;
                int16_t rc = rfm9x_start_receive();
                if (rc != 0)
                {
                    rx_arm_needed = true; // allow retry
                    printf("RX ERR: %d\n", rc);
                }
            }
            if (rx_packet_ready)
            {
                rx_packet_ready = false;
                int16_t rc = rfm9x_read_data(rx_ring[rx_write_ind], CHUNK_SIZE);
                if (rc == 0)
                {
                    rx_chunk_ready[rx_write_ind] = true;
                    rx_write_ind = (rx_write_ind + 1) % RX_RING_CHUNKS;
                    printf("RX chunk[%u]: %02x %02x %02x %02x ...\n",
                           (unsigned)((rx_write_ind + RX_RING_CHUNKS - 1) % RX_RING_CHUNKS),
                           rx_ring[(rx_write_ind + RX_RING_CHUNKS - 1) % RX_RING_CHUNKS][0],
                           rx_ring[(rx_write_ind + RX_RING_CHUNKS - 1) % RX_RING_CHUNKS][1],
                           rx_ring[(rx_write_ind + RX_RING_CHUNKS - 1) % RX_RING_CHUNKS][2],
                           rx_ring[(rx_write_ind + RX_RING_CHUNKS - 1) % RX_RING_CHUNKS][3]);
                }
                else
                {
                    printf("READ ERR: %d\n", rc);
                }
                rx_arm_needed = true;
            }
        }
    }

    return 0;
}
