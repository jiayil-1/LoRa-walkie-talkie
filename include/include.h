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

//state machine for LoRa transmission
typedef enum {
    STATE_RX,  // listening
    STATE_TX   // transmitting
} lora_state_t;

volatile bool tx_done;
volatile bool rx_done;
uint8_t dma_buf[CHUNK_SIZE];
uint8_t rec_buf;
 volatile int lora_read_ind;
volatile int dma_write_ind;
volatile bool tx_enable;
 volatile lora_state_t state;






