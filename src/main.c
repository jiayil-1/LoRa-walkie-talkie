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


