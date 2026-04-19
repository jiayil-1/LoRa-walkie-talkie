#include "include.h"
#include "microphone.h"

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

    // Use stable button level after debounce: high = pressed, low = released.
    if (events & GPIO_IRQ_EDGE_RISE)
    {
        gpio_acknowledge_irq(39, events);
        tx_enable = true;
        tx_done = true;
        adc_run(false);
        adc_fifo_drain();

        lora_read_ind = 0;
        dma_write_ind = 0;

        memset((void *)tx_chunk_ready, 0, sizeof(tx_chunk_ready));

        // Restart DMA stream into TX ring from the beginning.
        dma_channel_set_write_addr(dma_chan, tx_ring[0], false);
        dma_channel_set_trans_count(dma_chan, CHUNK_SIZE, true);

        adc_run(true);
        state = STATE_TX;
    }
    if (events & GPIO_IRQ_EDGE_FALL)
    {
        gpio_acknowledge_irq(39, events);
        tx_enable = false;
        rx_done = true;
        rx_arm_needed = true;
        rx_packet_ready = false;
        adc_run(false);

        dma_hw->abort = (1u << dma_chan);
        while (dma_hw->abort & (1u << dma_chan))
            ;

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

void packet_sent_isr(void)
{
    tx_needs_finish = true;
}
