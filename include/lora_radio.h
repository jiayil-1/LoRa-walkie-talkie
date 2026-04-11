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
    int16_t lora_send_text(const char *text);

#ifdef __cplusplus
}
#endif

#endif
