#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "hardware/adc.h"
#include "hardware/regs/dma.h"
#include <hardware/structs/dma.h>

// adc for mic pin -> rp2350
// spi rp2350 -> lora pin

uint32_t adc_fifo_out;
char buffer[10];



void init_adc() {
    adc_init();
    adc_gpio_init(40); // GPIO 40 is ADC0
    adc_select_input(0); // Select ADC0
}

//gp rising edge -> adc freerun 
//gp falling edge -> end adc freerun

void pb_isr_handler() {
    //handle the rising edge and turn on the freerun adc + drain the fifo
    if (gpio_get_irq_event_mask(39) & GPIO_IRQ_EDGE_RISE) {
        gpio_acknowledge_irq(39, GPIO_IRQ_EDGE_RISE);
        adc_fifo_drain();
        adc_run(true);
    }

    //handle the falling edge and turn off the freerun adc + drain the fifo
    if (gpio_get_irq_event_mask(39) & GPIO_IRQ_EDGE_FALL) {
        gpio_acknowledge_irq(39, GPIO_IRQ_EDGE_FALL);
        adc_run(false);
        adc_fifo_drain();
        for(int i = 0; i < 10; i++) {
            buffer[i] = 0;
        }
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


int main() {
    stdio_init_all();
    init_adc_dma();
    init_pb_irq();
    

    // for(;;) {
    //     printf("ADC Result: %ld     \r", adc_hw->result);
    //     fflush(stdout);
    //     sleep_ms(250);
    // }
    printf("Starting");
    for(;;) {
        float f = (adc_fifo_out * 3.3) / 4095.0;
        snprintf(buffer, sizeof(buffer), "%1.7f", f);
    
        printf("%s \n", buffer);
        sleep_ms(50);
    }

    return 0;
}


