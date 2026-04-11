#include "lora_radio.h"

#if !defined(RADIOLIB_BUILD_RPI_PICO)
#define RADIOLIB_BUILD_RPI_PICO
#endif

#include <RadioLib.h>
#include "hal/RPiPico/PicoHal.h"

// Defaults from RadioLib's Pico example; override with -D in platformio.ini as needed.
#ifndef LORA_SPI_PORT
#define LORA_SPI_PORT spi0
#endif

#ifndef LORA_SPI_MISO_PIN
#define LORA_SPI_MISO_PIN 4
#endif

#ifndef LORA_SPI_MOSI_PIN
#define LORA_SPI_MOSI_PIN 3
#endif

#ifndef LORA_SPI_SCK_PIN
#define LORA_SPI_SCK_PIN 2
#endif

static PicoHal *g_hal = nullptr;
static Module *g_module = nullptr;
static SX1262 *g_radio = nullptr;

static const int16_t LORA_BRIDGE_ERR_NOT_INITIALIZED = -10000;
static const int16_t LORA_BRIDGE_ERR_INVALID_ARGUMENT = -10001;

extern "C" int16_t lora_radio_begin_with_pins(int8_t cs_pin,
                                              int8_t dio1_pin,
                                              int8_t rst_pin,
                                              int8_t busy_pin,
                                              float frequency_mhz)
{
    if (g_radio != nullptr)
    {
        delete g_radio;
        g_radio = nullptr;
    }

    if (g_module != nullptr)
    {
        delete g_module;
        g_module = nullptr;
    }

    if (g_hal != nullptr)
    {
        delete g_hal;
        g_hal = nullptr;
    }

    g_hal = new PicoHal(LORA_SPI_PORT, LORA_SPI_MISO_PIN, LORA_SPI_MOSI_PIN, LORA_SPI_SCK_PIN);

    g_module = new Module(g_hal, cs_pin, dio1_pin, rst_pin, busy_pin);
    g_radio = new SX1262(g_module);

    return g_radio->begin(frequency_mhz);
}

extern "C" int16_t lora_radio_transmit_bytes(const uint8_t *data, size_t length)
{
    if (g_radio == nullptr)
    {
        return LORA_BRIDGE_ERR_NOT_INITIALIZED;
    }

    if (data == nullptr || length == 0)
    {
        return LORA_BRIDGE_ERR_INVALID_ARGUMENT;
    }

    return g_radio->transmit(data, length);
}
