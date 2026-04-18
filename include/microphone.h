#ifndef MICROPHONE_H
#define MICROPHONE_H

#include <stdint.h>

void init_adc();
void pb_isr_handler();
void init_pb_irq();
void dma_init();
void init_adc_dma();
void packet_sent_isr();

extern volatile bool adc_chunk_ready;
extern uint8_t dma_buf[CHUNK_SIZE];
extern volatile int adc_fill_idx;


#endif