#include "include.h"

int16_t lora_init_default(void) {
	return lora_radio_begin_with_pins(17, 20, 21, 22, 915.0f);
}

int16_t lora_send_text(const char* text) {
	if(text == NULL) {
		return -1;
	}

	return lora_radio_transmit_bytes((const uint8_t*)text, strlen(text));
}
