#ifndef TFT_H
#define TFT_H

void disp_cmd(uint8_t cmd);
void disp_data(uint8_t *buf, size_t len);
void disp_init();
void disp_fill(uint16_t color);
void disp_show_state(lora_state_t state);

#endif