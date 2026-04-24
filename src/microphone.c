#include "include.h"
#include "microphone.h"
#include "TFT.h"

static int dma_chan = 0;
static volatile uint64_t last_button_irq_us = 0;

void init_adc()
{
    adc_init();
    adc_gpio_init(40);    // GPIO 40 is ADC0
    adc_select_input(0);  // Select ADC0
    adc_set_clkdiv(2999); // 48M / 3000 = 16K samples/s
}

static void dma_irq_handler()
{
    dma_hw->ints0 = (1u << dma_chan);

    tx_chunk_ready[dma_write_ind] = true;

    // one chunk = good when samples filled > chunk_size, move onto next index
    dma_write_ind = (dma_write_ind + 1) % TX_RING_CHUNKS;
    dma_channel_set_write_addr(dma_chan, tx_ring[dma_write_ind], false); // increment write addr
    dma_channel_set_trans_count(dma_chan, CHUNK_SIZE, true);             // transfer?
}

void pb_isr_handler()
{

    uint32_t events = gpio_get_irq_event_mask(39);
    if (!events)
    {
        return;
    }

    // debounce
    const uint64_t debounce_us = 50000;
    uint64_t now_us = timer_hw->timerawl;
    if ((now_us - last_button_irq_us) < debounce_us)
    {
        return;
    }
    last_button_irq_us = now_us;

    // Use stable button level after debounce: high = pressed, low = released.
    if (events & GPIO_IRQ_EDGE_RISE)
    {
        disp_show_state(STATE_TX);
        gpio_acknowledge_irq(39, events);
        tx_enable = true;
        tx_done = true;
        adc_run(false);
        adc_fifo_drain();

        lora_read_ind = 0;
        dma_write_ind = 0;

        memset((void *)tx_chunk_ready, 0, sizeof(tx_chunk_ready));
        dma_hw->abort = (1u << 1);
        while (dma_hw->abort & (1u << 1))
            ;
        // Restart DMA stream into TX ring from the beginning.
        dma_channel_set_write_addr(dma_chan, tx_ring[0], false);
        dma_channel_set_trans_count(dma_chan, CHUNK_SIZE, true);

        adc_run(true);
        state = STATE_TX;
        
    }
    if (events & GPIO_IRQ_EDGE_FALL)
    {
        disp_show_state(STATE_RX);
        gpio_acknowledge_irq(39, events);
        tx_enable = false;
        rx_done = true;
        rx_arm_needed = true;
        rx_packet_ready = false;
        adc_run(false);

        dma_hw->abort = (1u << dma_chan);
        while (dma_hw->abort & (1u << dma_chan))
            ;

        dma_hw->ch[1].read_addr = (uintptr_t)&rx_ring[spk_read_chunk_ind];

    // make pwm cc register
        dma_hw->ch[1].write_addr = (uintptr_t)&(pwm_hw->slice[slice_num].cc);
        dma_hw->ch[1].transfer_count = (0ul << 28) | 0;
        uint32_t ctrlbits =
        (DREQ_DMA_TIMER0 << 17) | // trigger on timer0
        // (0 << 12) |               // ring applies to read addr
        // (0x3 << 8) |              // ring size every 8 bits
        //(1ul << 4) |              // increment read
        (0x0 << 2) |              // data size = 1 byte
        1;                        // EN
        dma_hw->ch[1].ctrl_trig = ctrlbits;
        dma_hw->ch[1].ctrl_trig = ctrlbits & ~1ul;

        adc_fifo_drain();
        state = STATE_RX;
        
    }
}

void init_pb_irq()
{

    gpio_init(39);
    gpio_set_dir(39, GPIO_IN);
    // Keep a defined default low level (safe even with external pulldown).
    // gpio_pull_down(39);
    gpio_add_raw_irq_handler(39, pb_isr_handler);
    gpio_set_irq_enabled(39, GPIO_IRQ_EDGE_RISE, true);
    gpio_set_irq_enabled(39, GPIO_IRQ_EDGE_FALL, true);

    // gpio_set_irq_enabled(16, GPIO_IRQ_EDGE_RISE, true);

    irq_set_enabled(IO_IRQ_BANK0, true);
}

void init_adc_dma()
{
    // initialize DMA and turn on ADC fifo
    init_adc();
    dma_write_ind = 0;
    lora_read_ind = 0;
    adc_fifo_setup(true, true, 1, false, true);

    dma_channel_config cfg = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_8);
    channel_config_set_read_increment(&cfg, false); // read from same
    channel_config_set_write_increment(&cfg, true); // walk the chunk
    channel_config_set_dreq(&cfg, DREQ_ADC);        // based on adc

    dma_channel_configure(
        dma_chan,
        &cfg,
        tx_ring[0], // first chunk
        &adc_hw->fifo,
        CHUNK_SIZE,
        false // button based
    );

    dma_channel_set_irq0_enabled(dma_chan, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_irq_handler);
    irq_set_enabled(DMA_IRQ_0, true);
}

void packet_rec_send_isr(void)
{
    if (state == STATE_RX)
    {
        rx_packet_ready = true;
    }
    else if (state == STATE_TX)
    {
        tx_needs_finish = true;
    }
}
