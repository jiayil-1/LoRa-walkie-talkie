#ifndef MICROPHONE_H
#define MICROPHONE_H

void init_adc();
void pb_isr_handler();
void init_pb_irq();
void init_adc_dma();
void packet_rec_send_isr();

/* Loopback test (mic -> PWM on same board). See bottom of microphone.c. */
void mic_loopback_start(void);

#endif
