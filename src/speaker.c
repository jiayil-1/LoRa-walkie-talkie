// #include "include.h"
// #include "speaker.h"
// #define SPEAKER_PIN 25
// #define PWRAP 4166

// static uint slice_num;
// static uint chan;

// void init_pwm()
// {
//     // set pin 25 to PWM function
//     gpio_set_function(SPEAKER_PIN, GPIO_FUNC_PWM);

//     // get the slice number for the specified GPIO pin
//     slice_num = pwm_gpio_to_slice_num(SPEAKER_PIN);
//     chan = pwm_gpio_to_channel(SPEAKER_PIN);

//     // configure PWM
//     pwm_config config = pwm_get_default_config();
//     pwm_config_set_wrap(&config, PWRAP);
//     pwm_config_set_clkdiv(&config, 1.0f);

//     // initialize the PWM slice with the specified configuration
//     pwm_init(slice_num, &config, true);
//     // set the PWM level to 0 (off)
//     pwm_set_chan_level(slice_num, chan, 0);
// }

// void pwm_push(uint8_t sample)
// {
//     // push the sample to the speaker
//     uint16_t level = ((uint32_t)sample * PWRAP) / 255; // Scale the sample to the PWM range
//     pwm_set_chan_level(slice_num, chan, level);
// }

#include "include.h"
#include "speaker.h"
#define SPEAKER_PIN 28
#define PWRAP 255

// uint slice_num;
static uint chan;
static uint dma_spk_chan = 1;
static uint8_t finished_idx = 0;

static int SPEAKER_DELAY_US = 63;
static void dma_read_irq_handler()
{
    printf("42\n");
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

    // Rearm for exactly one next byte only when data is available.
    if (rx_chunk_ready[spk_read_chunk_ind])
    {
        dma_channel_set_read_addr(
            dma_spk_chan,
            (const volatile void *)&rx_ring[spk_read_chunk_ind][spk_read_packet_ind],
            false);
        dma_channel_set_trans_count(dma_spk_chan, 1, false);
    }
}

void speaker_timer_isr()
{
    timer0_hw->intr = 1;
    timer0_hw->alarm[0] = timer0_hw->timerawl + SPEAKER_DELAY_US;
}

void init_speaker_timer()
{
    irq_set_exclusive_handler(TIMER0_IRQ_0, speaker_timer_isr);
    // enable alarm0 interrupt for timer1
    timer0_hw->inte = 1;

    // enable timer1 alarm0 interrupt for arm processor
    irq_set_enabled(TIMER0_IRQ_0, true);

    // set timer to go off in 63 us
    timer0_hw->alarm[0] = timer0_hw->timerawl + SPEAKER_DELAY_US;
}

void init_pwm()
{
    // set pin 25 to PWM function
    gpio_set_function(SPEAKER_PIN, GPIO_FUNC_PWM);

    // get the slice number for the specified GPIO pin
    slice_num = pwm_gpio_to_slice_num(SPEAKER_PIN);
    chan = pwm_gpio_to_channel(SPEAKER_PIN);

    // configure PWM
    pwm_config config = pwm_get_default_config();
    pwm_config_set_wrap(&config, PWRAP);
    // 125 Mhz / (clkdiv * 256) = 16khz
    pwm_config_set_clkdiv(&config, 36.6f);

    // initialize the PWM slice with the specified configuration
    pwm_init(slice_num, &config, true);

    // //sum interrupt bs
    // pwm_set_irq_enabled(slice_num, true);
    // irq_set_exclusive_handler(PWM_IRQ_WRAP_0, pwm_push);
    // irq_set_enabled(PWM_IRQ_WRAP_0, true);

    // set the PWM level to 0 (off)
    pwm_set_chan_level(slice_num, chan, 0);
}

void init_dma_speaker()
{
    dma_hw->ch[1].read_addr = (uintptr_t)&rx_ring[spk_read_chunk_ind];

    // make pwm cc register
    dma_hw->ch[1].write_addr = (uintptr_t)&(pwm_hw->slice[slice_num].cc);
    dma_hw->ch[1].transfer_count = (0ul << 28) | 1;
    uint32_t ctrlbits =
        (DREQ_DMA_TIMER0 << 17) | // trigger on timer0
        (0 << 12) |               // ring applies to read addr
        (0x3 << 8) |              // ring size every 8 bits
        (1ul << 4) |              // increment read
        (0x0 << 2) |              // data size = 1 byte
        1;                        // EN
    dma_hw->ch[1].ctrl_trig = ctrlbits;

    dma_channel_set_irq1_enabled(dma_spk_chan, true);
    irq_set_exclusive_handler(DMA_IRQ_1, dma_read_irq_handler);
    irq_set_enabled(DMA_IRQ_1, true);
}

// void pwm_push()
// {
//     pwm_clear_irq(slice_num);

//     // SET NEXT READ ADDRESS LOGIC
//     uint8_t sample = rx_ring[spk_read_chunk_ind][spk_read_packet_ind];

//     uint8_t finished_idx = spk_read_chunk_ind;
//     if(spk_read_packet_ind == 7){
//         rx_chunk_ready[finished_idx] = false;
//         memset((void *)rx_ring[finished_idx], 0, CHUNK_SIZE);
//         spk_read_chunk_ind = (spk_read_chunk_ind + 1) % RX_RING_CHUNKS;
//     }
//     //printf("RX byte: %02x\n\n", sample);
//     spk_read_packet_ind = (spk_read_packet_ind + 1) % 8;
//     // push the sample to the speaker
//     uint16_t level = ((uint32_t)sample * PWRAP) / 255; // Scale the sample to the PWM range
//     pwm_set_chan_level(slice_num, chan, level);
// }
