// #ifndef SPEAKER_H
// #define SPEAKER_H

// #include <stdint.h>

// void packet_received_isr();
// void pwm_push(uint8_t);
// void init_pwm();

// #endif

#ifndef SPEAKER_H
#define SPEAKER_H

#include <stdint.h>

void packet_received_isr();
void pwm_push();
void init_pwm();
void init_speaker_timer();
void init_dma_speaker();

/* Loopback test (mic -> PWM on same board). See bottom of speaker.c. */
void spk_loopback_push(uint8_t chunk_idx);
#endif