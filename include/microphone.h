#ifndef MICROPHONE_H
#define MICROPHONE_H

#include <stdint.h>

extern uint16_t adc_fifo_out;
extern char buffer[10];

void init_adc();
void pb_isr_handler();
void init_pb_irq();
void dma_init();
void init_adc_dma();

#endif