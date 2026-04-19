#ifndef MICROPHONE_H
#define MICROPHONE_H

void init_adc();
void pb_isr_handler();
void init_pb_irq();
void init_adc_dma();
void packet_sent_isr();

#endif
