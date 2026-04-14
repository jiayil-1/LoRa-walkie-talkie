#include "include.h"
#include "microphone.h"

uint8_t adc_buf[2][CHUNK_SIZE];
volatile int adc_fill_idx = 0;
volatile bool adc_chunk_ready = false;
static int dma_chan;

extern volatile lora_state_t state;

void init_adc() {
    adc_init();
    adc_gpio_init(40); // GPIO 40 is ADC0
    adc_select_input(0); // Select ADC0
    adc_set_clkdiv(5999); //48M / 6000 = 8K samples/s
}


static void dma_irq_handler() {
    dma_hw->ints0 = (1u << dma_chan); 

    adc_chunk_ready = true;

    adc_fill_idx ^= 1;
    dma_channel_set_write_addr(dma_chan, adc_buf[adc_fill_idx], false);
    dma_channel_set_trans_count(dma_chan, CHUNK_SIZE, true);  /* true = trigger */
}

//gp rising edge -> adc freerun 
//gp falling edge -> end adc freerun

void pb_isr_handler() {
    //handle the rising edge and turn on the freerun adc + drain the fifo
    if (gpio_get_irq_event_mask(39) & GPIO_IRQ_EDGE_RISE) {
        gpio_acknowledge_irq(39, GPIO_IRQ_EDGE_RISE);
        adc_fifo_drain();
        adc_fill_idx = 0;
        adc_chunk_ready = false;
        dma_channel_set_write_addr(dma_chan, adc_buf[0], false);
        dma_channel_set_trans_count(dma_chan, CHUNK_SIZE, true);
        adc_run(true);
        state = STATE_TX;
    }

    //handle the falling edge and turn off the freerun adc + drain the fifo
    if (gpio_get_irq_event_mask(39) & GPIO_IRQ_EDGE_FALL) {
        gpio_acknowledge_irq(39, GPIO_IRQ_EDGE_FALL);
        adc_run(false);
        dma_channel_abort(dma_chan);
        adc_fifo_drain();
        adc_chunk_ready = false;
        state = STATE_RX;
    }
}

void init_pb_irq() {
    //set pb_isr_handler as the interrupt handler
    gpio_init(39);
    gpio_set_dir(39, GPIO_IN);
    //gpio_set_function(39)
    gpio_add_raw_irq_handler(39, pb_isr_handler);

    //enable rising and falling edge gpios
    gpio_set_irq_enabled(39, GPIO_IRQ_EDGE_RISE, true);
    gpio_set_irq_enabled(39, GPIO_IRQ_EDGE_FALL, true);

    //enable nvic interrupts
    irq_set_enabled(IO_IRQ_BANK0, true);
}
/*
void dma_init() {
    //set read/write addresses for DMA
    dma_hw->ch[0].read_addr = (uintptr_t)&adc_hw->fifo;
    dma_hw->ch[0].write_addr = (uintptr_t)&adc_fifo_out;

    //configure DMA
    dma_hw->ch[0].transfer_count = (1u << 28) | 1u; //change to size

    uint32_t temp = 0;
    temp |= (1u << DMA_CH0_CTRL_TRIG_DATA_SIZE_LSB);
    temp |= (48u << DMA_CH0_CTRL_TRIG_TREQ_SEL_LSB);
    temp |= DMA_CH0_CTRL_TRIG_EN_BITS;

    dma_hw->ch[0].ctrl_trig = temp;
}
*/
void init_adc_dma() {
    //initialize DMA and turn on ADC fifo
    //dma_init();
    init_adc();

    adc_fifo_setup(true, true, 1, false, true);

    dma_chan = dma_claim_unused_channel(true);

    dma_channel_config cfg = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_8);
    channel_config_set_read_increment(&cfg, false);            /* always read from FIFO */
    channel_config_set_write_increment(&cfg, true);            /* write across buffer */
    channel_config_set_dreq(&cfg, DREQ_ADC);                   /* paced by ADC */

    dma_channel_configure(dma_chan, &cfg, adc_buf[0], &adc_hw->fifo, CHUNK_SIZE,false);
    dma_channel_set_irq0_enabled(dma_chan, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_irq_handler);
    irq_set_enabled(DMA_IRQ_0, true);
}

/*
void lora_write() {
    uint8_t data = adc_fifo_out >> 4; // Convert 12-bit ADC value to 8-bit by shifting right
    //spi_write16_blocking(spi0, &data, 1);
    lora_radio_transmit_bytes((const uint8_t *)&data, sizeof(data));
}*/


uint8_t *adc_get_ready_chunk(void) {
    if (!adc_chunk_ready) return NULL;
      uint32_t save = save_and_disable_interrupts();
      adc_chunk_ready = false;
      int idx = adc_fill_idx ^ 1;
      restore_interrupts(save);
      return adc_buf[idx];
}