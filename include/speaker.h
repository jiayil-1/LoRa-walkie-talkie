#ifndef SPEAKER_H
#define SPEAKER_H

#include <stdint.h>

void packet_received_isr();
void pwm_push(uint8_t);
void init_pwm();

#endif