#ifndef LORA_RADIO_H
#define LORA_RADIO_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    int16_t lora_radio_begin_with_pins(int8_t cs_pin,
                                       int8_t dio1_pin,
                                       int8_t rst_pin,
                                       int8_t busy_pin,
                                       float frequency_mhz);

    int16_t lora_radio_transmit_bytes(const uint8_t *data, size_t length);

    int16_t lora_init_default(void);
    int16_t rfm9x_begin_fsk(float freq, float br, float freqDev,
                         float rxBw, int8_t power,
                         uint16_t preambleLength);
    int16_t rfm9x_start_receive(void);
    int16_t rfm9x_read_data(uint8_t *buf, size_t len);
    size_t rfm9x_get_packet_length(void);
    int16_t rfm9x_start_transmit(const uint8_t *data, size_t len);
    int16_t rfm9x_finish_transmit(void);
    void rfm9x_set_packet_sent_action(void (*callback)(void));
    void rfm9x_set_packet_received_action(void (*isr)(void));
    void rfm9x_clear_packet_sent_action(void);

#ifdef __cplusplus
}
#endif

#endif
