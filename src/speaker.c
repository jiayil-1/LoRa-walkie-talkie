

#include "include.h"
#include "speaker.h"
#define SPEAKER_PIN 28
#define PWRAP 255

// uint slice_num;
static uint chan;
static uint dma_spk_chan = 1;
static uint8_t finished_idx = 0;
static void dma_read_irq_handler()
{
    //printf("dma fired");
    dma_hw->ints1 = (1u << dma_spk_chan);
           
    spk_read_packet_ind++;

    if (spk_read_packet_ind >= CHUNK_SIZE)
    {
        finished_idx = spk_read_chunk_ind;
        rx_chunk_ready[finished_idx] = false;
        memset((void *)rx_ring[finished_idx], 0, CHUNK_SIZE);

        spk_read_packet_ind = 0;
        spk_read_chunk_ind = (spk_read_chunk_ind + 1) % RX_RING_CHUNKS;
    }

    // Rearm only when data is available; otherwise disable channel and let
    // main() restart DMA when a new RX chunk arrives.
    if (rx_chunk_ready[spk_read_chunk_ind])
    {
        dma_channel_set_read_addr(
            dma_spk_chan,
            (const volatile void *)&spk_staging[spk_read_chunk_ind][spk_read_packet_ind],
            false);
        dma_channel_set_trans_count(dma_spk_chan, 1, true);
    }
    else
    {
        dma_hw->ch[dma_spk_chan].ctrl_trig &= ~1u;
    }
}


void init_pwm()
{
    // set pin 28 to PWM function
    gpio_set_function(SPEAKER_PIN, GPIO_FUNC_PWM);

    // get the slice number for the specified GPIO pin
    slice_num = pwm_gpio_to_slice_num(SPEAKER_PIN);
    chan = pwm_gpio_to_channel(SPEAKER_PIN);

    // configure PWM
    pwm_config config = pwm_get_default_config();
    pwm_config_set_wrap(&config, PWRAP);
    // 125 Mhz / (clkdiv * 256) = 16khz
    pwm_config_set_clkdiv(&config, 1.0f); //FIX FIX

    // initialize the PWM slice with the specified configuration
    pwm_init(slice_num, &config, true);

    // set the PWM level to 0 (off)
    pwm_set_enabled(slice_num, true);
    pwm_set_chan_level(slice_num, chan, 0);
}

void init_dma_speaker()
{
    dma_timer_claim(0);
    dma_timer_set_fraction(0, 1, 9375); // FROM 9375 -> 7813

    dma_hw->ch[1].read_addr = (uintptr_t)&spk_staging[spk_read_chunk_ind];

    // make pwm cc register
    dma_hw->ch[1].write_addr = (uintptr_t)&(pwm_hw->slice[slice_num].cc);
    dma_hw->ch[1].transfer_count = (0ul << 28) | 0;
    uint32_t ctrlbits =
        (DREQ_DMA_TIMER0 << 17) | // trigger on timer0
         (0 << 12) |               // ring applies to read addr
         (0x3 << 8) |              // ring size every 8 bits
        (1ul << 4) |              // increment read
        (0x2 << 2) |              // data size = 4 bytes (word)
        1;                        // EN
    dma_hw->ch[1].ctrl_trig = ctrlbits;
    dma_hw->ch[1].ctrl_trig = ctrlbits & ~1ul;
    dma_channel_set_irq1_enabled(dma_spk_chan, true);
    irq_set_exclusive_handler(DMA_IRQ_1, dma_read_irq_handler);
    irq_set_enabled(DMA_IRQ_1, true);
}

/* ============================================================================
 * Loopback test helpers
 * ----------------------------------------------------------------------------
 * Anything below this banner is for the LOOPBACK_TEST path in main.c only:
 * mic ADC -> tx_ring -> PWM speaker on the SAME board. No LoRa.
 * Safe to delete this block (and the matching prototype in speaker.h)
 * if the loopback path is ever removed.
 * ============================================================================
 */

void spk_loopback_push(uint8_t chunk_idx)
{
    /* Stage one mic chunk into the speaker buffer, then kick the speaker
     * DMA if it has gone idle. Same arming pattern as the RX branch in main.c,
     * just sourced from tx_ring instead of rx_ring. */
    for (int i = 0; i < CHUNK_SIZE; i++)
    {
        spk_staging[chunk_idx][i] = (uint32_t)tx_ring[chunk_idx][i];
    }
    rx_chunk_ready[chunk_idx] = true;

    if (!(dma_hw->ch[dma_spk_chan].ctrl_trig & 1u))
    {
        dma_channel_set_read_addr(
            dma_spk_chan,
            &spk_staging[spk_read_chunk_ind][spk_read_packet_ind],
            false);
        dma_hw->ch[dma_spk_chan].ctrl_trig |= 1u;
        dma_channel_set_trans_count(dma_spk_chan, 1, true);
    }
}
