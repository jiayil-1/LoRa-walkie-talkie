#include "include.h"
#include "microphone.h"
//#include "lora.h"
// #include <stdio.h>
// #include <string.h>
// #include "pico/stdlib.h"
// #include "hardware/timer.h"
// #include "hardware/irq.h"
// #include "hardware/adc.h"
// #include "hardware/regs/dma.h"
// #include <hardware/structs/dma.h>
// #include "microphone.c"
// #include "lora.c"

// adc for mic pin -> rp2350
// spi rp2350 -> lora pin

void init_spi() {
    //initialize gpio pins
    uint32_t mask = (1 << 17) | (1 << 18) | (1 << 19) | (1 << 20);
    gpio_init_masked(mask);

    //spi0 CSn
    gpio_set_function(17, GPIO_FUNC_SPI);
    //spi0 SCK
    gpio_set_function(18, GPIO_FUNC_SPI);
    //spi0 TX
    gpio_set_function(19, GPIO_FUNC_SPI);
    //spi0 RX
    gpio_set_function(20, GPIO_FUNC_SPI);

    //spi format
    spi_init(spi0, 1000000); //1MHz
    spi_set_format(spi0, 16, 0, 0, SPI_MSB_FIRST);
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


