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

#ifdef __cplusplus
#include <RadioLib.h>
#endif

#include "lora_radio.h"

//state machine for LoRa transmission
typedef enum {
    STATE_RX,  // listening
    STATE_TX   // transmitting
} lora_state_t;

