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

#define CHUNK_SIZE 60

extern volatile bool adc_chunk_ready;
extern uint8_t adc_buf[2][CHUNK_SIZE];
extern volatile int adc_fill_idx;

uint8_t *adc_get_ready_chunk(void);


#endif