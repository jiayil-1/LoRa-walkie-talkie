#include "include.h"
#include "microphone.h"

static int dma_chan = 0;

void init_adc() {
    adc_init();
    adc_gpio_init(40); // GPIO 40 is ADC0
    adc_select_input(0); // Select ADC0
    adc_set_clkdiv(5999); //48M / 6000 = 8K samples/s
}

static void dma_irq_handler() {
    dma_hw->ints0 = (1u << dma_chan);    
    dma_channel_set_write_addr(dma_chan, &dma_buf[dma_write_ind], true);
}


//gp rising edge -> adc freerun 
//gp falling edge -> end adc freerun

void pb_isr_handler() {
    //handle the rising edge and turn on the freerun adc + drain the fifo
    if (gpio_get_irq_event_mask(39) & GPIO_IRQ_EDGE_RISE) {
        //printf("Pushbutton is PUSHED\n");
        tx_enable = true;
        tx_done = true;
        gpio_acknowledge_irq(39, GPIO_IRQ_EDGE_RISE);
        adc_fifo_drain();

        //bring dma back to life
        dma_hw->ch[0].read_addr = (uintptr_t)&adc_hw->fifo;
        dma_hw->ch[0].write_addr = (uintptr_t)&dma_buf[dma_write_ind];
        dma_hw->ch[0].transfer_count = (1ul << 28) | 1ul;
        uint32_t config = (DREQ_ADC << 17) | (0x0 << 2) | 1;
        dma_hw->ch[0].ctrl_trig = config;

        //start adc freerun until button released
        adc_run(true);
        state = STATE_TX;
    }

    //handle the falling edge and turn off the freerun adc + drain the fifo
    if (gpio_get_irq_event_mask(39) & GPIO_IRQ_EDGE_FALL) {
        //printf("Pushbutton RELEASED\n");
        tx_enable = false;
        rx_done = true;
        gpio_acknowledge_irq(39, GPIO_IRQ_EDGE_FALL);
        adc_run(false);
        //safer dma abort to avoid hanging
        dma_hw->abort = (1u << dma_chan);
        while (dma_hw->abort & (1u << dma_chan));
        adc_fifo_drain();
        state = STATE_RX;
        memset(dma_buf, 0, CHUNK_SIZE);
    }
}

void init_pb_irq() {
    //set pb_isr_handler as the interrupt handler
    gpio_init(39);
    gpio_set_dir(39, GPIO_IN);
    gpio_disable_pulls(39);
    //gpio_set_function(39)
    gpio_add_raw_irq_handler(39, pb_isr_handler);

    //enable rising and falling edge gpios
    gpio_set_irq_enabled(39, GPIO_IRQ_EDGE_RISE, true);
    gpio_set_irq_enabled(39, GPIO_IRQ_EDGE_FALL, true);

    //enable nvic interrupts
    irq_set_enabled(IO_IRQ_BANK0, true);
}

void init_adc_dma() {
    //initialize DMA and turn on ADC fifo
    init_adc();
    dma_write_ind = 0;
    lora_read_ind = 4;
    adc_fifo_setup(true, true, 1, false, true);

    
    dma_hw->ch[0].read_addr = (uintptr_t)&adc_hw->fifo;
    dma_hw->ch[0].write_addr = (uintptr_t)&dma_buf[dma_write_ind];
    dma_hw->ch[0].transfer_count = (0x1 << 28) | 1ul;
    uint32_t config = (DREQ_ADC << 17) | (0x0 << 2) | 1;
    dma_hw->ch[0].ctrl_trig = config;

    // dma_channel_config cfg = dma_channel_get_default_config(dma_chan);
    // channel_config_set_transfer_data_size(&cfg, DMA_SIZE_8);
    // channel_config_set_read_increment(&cfg, false);            /* always read from FIFO */
    // channel_config_set_write_increment(&cfg, false);            /* don't increment write address */
    // channel_config_set_dreq(&cfg, DREQ_ADC);                   /* paced by ADC */

    // dma_channel_configure(dma_chan, &cfg, dma_buf[dma_write_ind], &adc_hw->fifo, CHUNK_SIZE,false);
    dma_channel_set_irq0_enabled(dma_chan, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_irq_handler);
    irq_set_enabled(DMA_IRQ_0, true);
    
}


void packet_sent_isr(void) { // lora interrupt
    
    
    /*for (int i = 0; i < CHUNK_SIZE; i++) {
                    printf(" %02x", dma_buf[i]);
                }
                printf("\n");*/
    //printf("I just sent a packet!!!\n");
    
    tx_done = true;

    dma_write_ind = (dma_write_ind + 1) % 8;
    lora_read_ind = (lora_read_ind + 1) % 8;
}