#ifndef APP_INCLUDE_H
#define APP_INCLUDE_H

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "hardware/adc.h"
#include "hardware/regs/dma.h"
#include <hardware/structs/dma.h>
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include "hardware/sync.h"
#include "hardware/pwm.h"

#ifdef __cplusplus
#include <RadioLib.h>
#endif

#include "lora_radio.h"

#define CHUNK_SIZE 8
#define TX_RING_CHUNKS 8
#define RX_RING_CHUNKS 8

typedef enum
{
    STATE_RX,
    STATE_TX
} lora_state_t;

extern volatile bool tx_done;
extern volatile bool tx_needs_finish;
extern volatile bool rx_done;
extern volatile bool rx_arm_needed;
extern volatile bool rx_packet_ready;

extern uint8_t tx_ring[TX_RING_CHUNKS][CHUNK_SIZE];
extern uint8_t rx_ring[RX_RING_CHUNKS][CHUNK_SIZE];

extern volatile bool tx_chunk_ready[TX_RING_CHUNKS];
extern volatile bool rx_chunk_ready[RX_RING_CHUNKS];

extern volatile uint8_t lora_read_ind;
extern volatile uint8_t dma_write_ind;
extern volatile uint8_t rx_read_ind;
extern volatile uint8_t rx_write_ind;

extern volatile bool tx_enable;
extern volatile lora_state_t state;

#endif
