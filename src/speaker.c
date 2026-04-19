#include "include.h"
#include "speaker.h"
#define SPEAKER_PIN 25
#define PWRAP 4166


static uint slice_num;
static uint chan;

void init_pwm() {
    //set pin 25 to PWM function
    gpio_set_function(SPEAKER_PIN, GPIO_FUNC_PWM);

    //get the slice number for the specified GPIO pin
    slice_num = pwm_gpio_to_slice_num(SPEAKER_PIN);
    chan = pwm_gpio_to_channel(SPEAKER_PIN);
    
    //configure PWM
    pwm_config config = pwm_get_default_config();
    pwm_config_set_wrap(&config, PWRAP);
    pwm_config_set_clkdiv(&config, 1.0f);

    //initialize the PWM slice with the specified configuration
    pwm_init(slice_num, &config, true);
    //set the PWM level to 0 (off)
    pwm_set_chan_level(slice_num, chan, 0);
}

void pwm_push(uint8_t sample) {
    //push the sample to the speaker
    uint16_t level = ((uint32_t) sample * PWRAP) / 255; // Scale the sample to the PWM range
    pwm_set_chan_level(slice_num, chan, level);
}

void packet_received_isr() {
    rx_packet_ready = true;
}
