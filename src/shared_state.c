// #include "include.h"

// volatile bool tx_done = true;
// volatile bool tx_needs_finish = false;
// volatile bool rx_done = false;
// volatile bool rx_arm_needed = true;
// volatile bool rx_packet_ready = false;

// uint8_t tx_ring[TX_RING_CHUNKS][CHUNK_SIZE];
// uint8_t rx_ring[RX_RING_CHUNKS][CHUNK_SIZE];

// volatile bool tx_chunk_ready[TX_RING_CHUNKS];
// volatile bool rx_chunk_ready[RX_RING_CHUNKS];

// volatile uint8_t lora_read_ind = 0;
// volatile uint8_t dma_write_ind = 0;
// volatile uint8_t rx_read_ind = 0;
// volatile uint8_t rx_write_ind = 0;

// volatile bool tx_enable = false;
// volatile lora_state_t state = STATE_RX;

#include "include.h"

volatile bool tx_done = true;
volatile bool tx_needs_finish = false;
volatile bool rx_done = false;
volatile bool rx_arm_needed = true;
volatile bool rx_packet_ready = false;

volatile uint slice_num;

uint8_t tx_ring[TX_RING_CHUNKS][CHUNK_SIZE];
uint8_t rx_ring[RX_RING_CHUNKS][CHUNK_SIZE];

volatile bool tx_chunk_ready[TX_RING_CHUNKS];
volatile bool rx_chunk_ready[RX_RING_CHUNKS];

volatile uint8_t lora_read_ind = 0;
volatile uint8_t dma_write_ind = 0;
volatile uint8_t dma_read_ind = 0;
volatile uint8_t spk_read_chunk_ind = 0;
volatile uint8_t spk_read_packet_ind = 0;
volatile uint8_t rx_read_ind = 0;
volatile uint8_t rx_write_ind = 0;

volatile bool tx_enable = false;
volatile lora_state_t state = STATE_RX;
