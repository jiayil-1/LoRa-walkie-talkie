#include "include.h"
#include "microphone.h"
#include "speaker.h"
#include "TFT.h"

#define ADC_VREF_MV    3300u
#define ADC_MAX_8BIT    255u
#define TX_LOG_EVERY_N  100u
#define RX_LOG_EVERY_N  100u

// SX127x FSK IRQ_FLAGS_2 (0x3F) bit 2 = PAYLOAD_READY
#define REG_IRQ_FLAGS_2    0x3Fu
#define FLAG_PAYLOAD_READY 0x04u

int main()
{
    uint32_t tx_log_counter = 0;
    uint32_t rx_log_counter = 0;

    stdio_init_all();
    setvbuf(stdout, NULL, _IONBF, 0);
    sleep_ms(300);
    gpio_init(16);
    gpio_set_dir(16, false);
    printf("BOOT: main entered\n");

    init_adc_dma();
    
    init_pb_irq();
    init_pwm();
    init_dma_speaker();

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
    spk_read_chunk_ind = 0;
    spk_read_packet_ind = 0;

    

    int16_t lora_status = lora_init_default();
    printf("BOOT: lora_init=%d\n", lora_status);

     disp_init();
     disp_show_state(STATE_RX);

    int16_t status = rfm9x_begin_fsk(433.0, 300.0, 75.0, 250.0, 10, 16);
    printf("BOOT: fsk_init=%d\n", status);
    if (status != 0)
    {
        printf("ERR: bad radio init, halting\n");
        while (true) { sleep_ms(1000); }
    }

    rfm9x_set_packet_sent_action(packet_rec_send_isr);
    
    printf("BOOT: ready, entering RX\n");
    state = STATE_RX;

    for (;;)
    {
        if (state == STATE_TX)
        {
            if (tx_needs_finish)
            {
                tx_needs_finish = false;
                int16_t rc = rfm9x_finish_transmit();
                if (rc != 0) { printf("TX FINISH ERR: %d\n", rc); }
                tx_done = true;
            }

            if (tx_done && tx_enable && tx_chunk_ready[lora_read_ind])
            {
                const uint8_t tx_idx = lora_read_ind;

                // Snapshot before transmit so the print is race-free with DMA.
                uint8_t snap[CHUNK_SIZE];
                memcpy(snap, (const void *)tx_ring[tx_idx], CHUNK_SIZE);

                int16_t rc = rfm9x_start_transmit(tx_ring[tx_idx], CHUNK_SIZE);
                tx_done = false;
                if (rc != 0)
                {
                    tx_done = true;
                    printf("TX ERR: %d\n", rc);
                }
                else
                {
                    tx_log_counter++;
                    if ((tx_log_counter % TX_LOG_EVERY_N) == 0u)
                    {
                        uint32_t s0 = ((uint32_t)snap[0] * ADC_VREF_MV) / ADC_MAX_8BIT;
                        uint32_t s1 = ((uint32_t)snap[1] * ADC_VREF_MV) / ADC_MAX_8BIT;
                        uint32_t s2 = ((uint32_t)snap[2] * ADC_VREF_MV) / ADC_MAX_8BIT;
                        uint32_t s3 = ((uint32_t)snap[3] * ADC_VREF_MV) / ADC_MAX_8BIT;
                        printf("TX [%lu]: %u.%03uV %u.%03uV %u.%03uV %u.%03uV\n",
                            (unsigned long)tx_log_counter,
                            s0/1000u, s0%1000u,
                            s1/1000u, s1%1000u,
                            s2/1000u, s2%1000u,
                            s3/1000u, s3%1000u);
                    }
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
                    rx_arm_needed = true;
                    printf("RX ARM ERR: %d\n", rc);
                }
            }

            if (rx_packet_ready)
            {
                rx_packet_ready = false;

                // spurious
                
                if (!(rfm9x_read_reg(REG_IRQ_FLAGS_2) & FLAG_PAYLOAD_READY))
                {
                    rx_arm_needed = true;
                    continue;
                }

                int16_t rc = rfm9x_read_data(rx_ring[rx_write_ind], CHUNK_SIZE);
                if (rc == 0)
                {
                    for (int i = 0; i < CHUNK_SIZE; i++)
                        spk_staging[rx_write_ind][i] = rx_ring[rx_write_ind][i];
                    rx_chunk_ready[rx_write_ind] = true;
                    rx_write_ind = (rx_write_ind + 1) % RX_RING_CHUNKS;
                    if (!(dma_hw->ch[1].ctrl_trig & 1u)) {
                        dma_channel_set_read_addr(1, &spk_staging[spk_read_chunk_ind][0], false);
                        dma_hw->ch[1].ctrl_trig |= 1u;  // EN=1
                        dma_channel_set_trans_count(1, 1, true);  // trigger first transfer
                    }
                    rx_log_counter++;
                    if ((rx_log_counter % RX_LOG_EVERY_N) == 0u)
                    {
                        uint8_t rx_idx = (rx_write_ind + RX_RING_CHUNKS - 1u) % RX_RING_CHUNKS;
                        uint32_t r0 = ((uint32_t)rx_ring[rx_idx][0] * ADC_VREF_MV) / ADC_MAX_8BIT;
                        uint32_t r1 = ((uint32_t)rx_ring[rx_idx][1] * ADC_VREF_MV) / ADC_MAX_8BIT;
                        uint32_t r2 = ((uint32_t)rx_ring[rx_idx][2] * ADC_VREF_MV) / ADC_MAX_8BIT;
                        uint32_t r3 = ((uint32_t)rx_ring[rx_idx][3] * ADC_VREF_MV) / ADC_MAX_8BIT;
                        printf("RX [%lu]: %u.%03uV %u.%03uV %u.%03uV %u.%03uV\n",
                            (unsigned long)rx_log_counter,
                            r0/1000u, r0%1000u,
                            r1/1000u, r1%1000u,
                            r2/1000u, r2%1000u,
                            r3/1000u, r3%1000u);
                    }
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
