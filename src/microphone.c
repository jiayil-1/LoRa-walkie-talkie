#include "include.h"
#include "microphone.h"

uint16_t adc_fifo_out;
char buffer[10];
extern volatile lora_state_t state;


void init_adc() {
    adc_init();
    adc_gpio_init(40); // GPIO 40 is ADC0
    adc_select_input(0); // Select ADC0
    //adc_set_clkdiv(16);
}

//gp rising edge -> adc freerun 
//gp falling edge -> end adc freerun

void pb_isr_handler() {
    //handle the rising edge and turn on the freerun adc + drain the fifo
    if (gpio_get_irq_event_mask(39) & GPIO_IRQ_EDGE_RISE) {
        gpio_acknowledge_irq(39, GPIO_IRQ_EDGE_RISE);
        adc_fifo_drain();
        adc_run(true);
        state = STATE_TX;
    }

    //handle the falling edge and turn off the freerun adc + drain the fifo
    if (gpio_get_irq_event_mask(39) & GPIO_IRQ_EDGE_FALL) {
        gpio_acknowledge_irq(39, GPIO_IRQ_EDGE_FALL);
        adc_run(false);
        adc_fifo_drain();
        for(int i = 0; i < 10; i++) {
            buffer[i] = 0;
        }
        state = STATE_RX;
    }
}

void init_pb_irq() {
    //set pb_isr_handler as the interrupt handler
    gpio_init(39);
    //gpio_set_function(39)
    gpio_add_raw_irq_handler(39, pb_isr_handler);

    //enable rising and falling edge gpios
    gpio_set_irq_enabled(39, GPIO_IRQ_EDGE_RISE, true);
    gpio_set_irq_enabled(39, GPIO_IRQ_EDGE_FALL, true);

    //enable nvic interrupts
    irq_set_enabled(IO_IRQ_BANK0, true);
}

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

void init_adc_dma() {
    //initialize DMA and turn on ADC fifo
    dma_init();
    init_adc();
    adc_hw ->fcs =  ADC_FCS_EN_BITS |
                   ADC_FCS_DREQ_EN_BITS |
                   ADC_FCS_THRESH_BITS;
}


void lora_write() {
    uint16_t data = adc_fifo_out;
    //spi_write16_blocking(spi0, &data, 1);
    lora_radio_transmit_bytes((const uint8_t *)&data, sizeof(data));
}
