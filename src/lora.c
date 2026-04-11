#include "lora.h"
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "hardware/adc.h"
#include "hardware/regs/dma.h"
#include <hardware/structs/dma.h>




void lora_write() {
    uint16_t adc_fifo_out 
    spi_write16_blocking(spi0, &packet, 1);
}

