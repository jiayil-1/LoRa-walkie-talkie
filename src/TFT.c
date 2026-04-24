#include "include.h"
#include "TFT.h"

// pins for SPI1
#define DISP_SPI    spi1 
#define DISP_SCK    14
#define DISP_MOSI   15
#define DISP_CS     13
#define DISP_DC     11
#define DISP_RST    12

// ILI9341 commands
#define ILI9341_SWRESET 0x01
#define ILI9341_SLPOUT  0x11
#define ILI9341_DISPON  0x29
#define ILI9341_COLMOD  0x3A
#define ILI9341_MADCTL  0x36

// colors (RGB565)
#define COLOR_GREEN  0x07E0
#define COLOR_RED   0x001F 

void disp_cmd(uint8_t cmd) {
    //put screen into command mode
    gpio_put(DISP_DC, 0);
    gpio_put(DISP_CS, 0);

    //write command
    spi_write_blocking(DISP_SPI, &cmd, 1);
    gpio_put(DISP_CS, 1);
}

void disp_data(uint8_t *buf, size_t len) {
    //put screen into data mode
    gpio_put(DISP_DC, 1);
    gpio_put(DISP_CS, 0);

    //write data
    spi_write_blocking(DISP_SPI, buf, len);
    gpio_put(DISP_CS, 1);
}

void disp_init() {
    //init SPI1
    spi_init(DISP_SPI, 10000000);
    gpio_set_function(DISP_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(DISP_MOSI, GPIO_FUNC_SPI);

    //init control pins
    gpio_init(DISP_CS);  
    gpio_init(DISP_DC);  
    gpio_init(DISP_RST); 
    gpio_set_dir(DISP_CS,  true); 
    gpio_set_dir(DISP_DC,  true); 
    gpio_set_dir(DISP_RST, true);

    gpio_put(DISP_CS,  1);  //start off
    gpio_put(DISP_DC,  1);  //data mode

    //reset
    gpio_put(DISP_RST, 0); 
    sleep_ms(10);
    gpio_put(DISP_RST, 1); 
    sleep_ms(120);

    //init sequence
    disp_cmd(ILI9341_SWRESET); 
    sleep_ms(150);
    disp_cmd(ILI9341_SLPOUT);  
    sleep_ms(150);

    //16-bit color
    disp_cmd(ILI9341_COLMOD);
    uint8_t colmod = 0x55;
    disp_data(&colmod, 1);

    disp_cmd(ILI9341_DISPON);
}

void disp_fill(uint16_t color) {
    //set col address 0-239
    disp_cmd(0x2A);
    uint8_t col[] = {0x00, 0x00, 0x00, 0xEF};
    disp_data(col, 4);

    //set row address 0-319
    disp_cmd(0x2B);
    uint8_t row[] = {0x00, 0x00, 0x01, 0x3F};
    disp_data(row, 4);

    //write pixels
    disp_cmd(0x2C);
    gpio_put(DISP_DC, 1);
    gpio_put(DISP_CS, 0);
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    for (int i = 0; i < 240 * 320; i++) {
        spi_write_blocking(DISP_SPI, &hi, 1);
        spi_write_blocking(DISP_SPI, &lo, 1);
    }
    gpio_put(DISP_CS, 1);
}

void disp_show_state(lora_state_t state) {
    if (state == STATE_TX) {
        disp_fill(COLOR_GREEN);    //green = transmit
    } 
    else {
        disp_fill(COLOR_RED);  //red = receive
    }
}