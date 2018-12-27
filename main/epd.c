#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

#include "epd.h"

const int spi_dma_channel = 1;

static spi_device_handle_t epd_spi;

DRAM_ATTR static uint8_t lut_vcom0[] =
{
    0x00, 0x17, 0x00, 0x00, 0x00, 0x02,        
    0x00, 0x17, 0x17, 0x00, 0x00, 0x02,        
    0x00, 0x0A, 0x01, 0x00, 0x00, 0x01,        
    0x00, 0x0E, 0x0E, 0x00, 0x00, 0x02,        
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,        
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,        
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

DRAM_ATTR static uint8_t lut_ww[] ={
    0x40, 0x17, 0x00, 0x00, 0x00, 0x02,
    0x90, 0x17, 0x17, 0x00, 0x00, 0x02,
    0x40, 0x0A, 0x01, 0x00, 0x00, 0x01,
    0xA0, 0x0E, 0x0E, 0x00, 0x00, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

DRAM_ATTR static uint8_t lut_bw[] ={
    0x40, 0x17, 0x00, 0x00, 0x00, 0x02,
    0x90, 0x17, 0x17, 0x00, 0x00, 0x02,
    0x40, 0x0A, 0x01, 0x00, 0x00, 0x01,
    0xA0, 0x0E, 0x0E, 0x00, 0x00, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

DRAM_ATTR static uint8_t lut_wb[] ={
    0x80, 0x17, 0x00, 0x00, 0x00, 0x02,
    0x90, 0x17, 0x17, 0x00, 0x00, 0x02,
    0x80, 0x0A, 0x01, 0x00, 0x00, 0x01,
    0x50, 0x0E, 0x0E, 0x00, 0x00, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

DRAM_ATTR static uint8_t lut_bb[] ={
    0x80, 0x17, 0x00, 0x00, 0x00, 0x02,
    0x90, 0x17, 0x17, 0x00, 0x00, 0x02,
    0x80, 0x0A, 0x01, 0x00, 0x00, 0x01,
    0x50, 0x0E, 0x0E, 0x00, 0x00, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static void spi_pre_transfer_callback(spi_transaction_t *trans)
{
    gpio_set_level(EPD_PIN_DC, trans->user? 1 : 0);
}

static void spi_post_transfer_callback(spi_transaction_t *trans)
{
}

static void task_delay_ms(int ms)
{
    vTaskDelay(ms / portTICK_PERIOD_MS);    
}

void epd_init(void)
{
    spi_bus_config_t buscfg = {
        .miso_io_num = (-1),
        .mosi_io_num = EPD_PIN_DIN,
        .sclk_io_num = EPD_PIN_CLK,
        .quadwp_io_num = (-1),
        .quadhd_io_num = (-1),
        .max_transfer_sz = EPD_BYTES
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 4000000,              // Clock out at 4 MHz
        .mode = 0,                              // SPI mode 0
        .spics_io_num = EPD_PIN_CS,             // CS pin
        .queue_size = 7,                        // We want to be able to queue 7 transactions at a time
        .flags = (SPI_DEVICE_HALFDUPLEX|SPI_DEVICE_3WIRE),
        .pre_cb = spi_pre_transfer_callback,    // Specify pre-transfer callback to handle D/C line
        .post_cb = spi_post_transfer_callback,  // Specify post-transfer callback
    };

    esp_err_t ret;
    // Initialize the SPI bus
    ret = spi_bus_initialize(VSPI_HOST, &buscfg, spi_dma_channel);
    assert(ret == ESP_OK);
    // Attach the EPD to the SPI bus
    ret = spi_bus_add_device(VSPI_HOST, &devcfg, &epd_spi);
    assert(ret == ESP_OK);

    gpio_set_direction(EPD_PIN_RST, GPIO_MODE_OUTPUT);
    gpio_set_direction(EPD_PIN_DC, GPIO_MODE_OUTPUT);
    gpio_set_direction(EPD_PIN_BUSY, GPIO_MODE_INPUT);
}

static void epd_wait(void) 
{
    do 
    {
        task_delay_ms(200);
    }
    while(!gpio_get_level(EPD_PIN_BUSY));
}

static void epd_reset()
{
    task_delay_ms(200);
    gpio_set_level(EPD_PIN_RST, 0);
    task_delay_ms(200);
    gpio_set_level(EPD_PIN_RST, 1);
    task_delay_ms(200);
}

static void send_data(const void* bytes, uint32_t length)
{
    esp_err_t ret;
    spi_transaction_t t;

    if (length > 0)
    {
        memset(&t, 0, sizeof(t));                   // Zero out the transaction
        t.length = length << 3;                     // Len is in bytes, transaction length is in bits.
        t.tx_buffer = bytes;                        // Data
        t.user = (void*)1;                          // D/C needs to be set to 1
        ret = spi_device_transmit(epd_spi, &t);  // Transmit!
        assert(ret == ESP_OK);                      // Should have had no issues.
    }
}

static void send_command(uint8_t cmd) 
{
    esp_err_t ret;
    spi_transaction_t t;

    memset(&t, 0, sizeof(t));                   // Zero out the transaction
    t.length = 8;                               // Command is 8 bits
    t.tx_buffer = &cmd;                         // The data is the cmd itself
    t.user = (void*)0;                          // D/C needs to be set to 0
    ret = spi_device_transmit(epd_spi, &t);  // Transmit!
    assert(ret == ESP_OK);                      // Should have had no issues.
}

#define SEND_COMMAND(cmd, ...) \
{ \
    DRAM_ATTR static uint8_t _data_bytes[] = __VA_ARGS__; \
    send_command(cmd);  \
    send_data(_data_bytes, sizeof(_data_bytes)/sizeof(uint8_t)); \
}

#define SEND_LUT(cmd, lut) \
{ \
    send_command(cmd);  \
    send_data(lut, sizeof(lut)/sizeof(uint8_t)); \
}

static void epd_refresh()
{
    send_command(DISPLAY_REFRESH);
    epd_wait();
}

void epd_wakeup(void)
{
    epd_reset();

    SEND_COMMAND(POWER_SETTING, {0x03, 0x00, 0x2b, 0x2b});
    SEND_COMMAND(BOOSTER_SOFT_START, {0x17, 0x17, 0x17});
    send_command(POWER_ON);
    epd_wait();

    SEND_COMMAND(PANEL_SETTING, {0xbf, 0x0d});
    SEND_COMMAND(PLL_CONTROL, {0x3c});
    SEND_COMMAND(RESOLUTION_SETTING, {0x01, 0x90, 0x01, 0x2c});
    SEND_COMMAND(VCM_DC_SETTING, {0x28});
    SEND_COMMAND(VCOM_AND_DATA_INTERVAL_SETTING, {0x97});

    SEND_LUT(LUT_FOR_VCOM, lut_vcom0);
    SEND_LUT(LUT_WHITE_TO_WHITE, lut_ww);
    SEND_LUT(LUT_BLACK_TO_WHITE, lut_bw);
    SEND_LUT(LUT_WHITE_TO_BLACK, lut_wb);
    SEND_LUT(LUT_BLACK_TO_BLACK, lut_bb);
}

void epd_sleep(void)
{
    send_command(POWER_OFF);
    epd_wait();
    SEND_COMMAND(DEEP_SLEEP, {0xa5});
}

void epd_clear(void)
{
    const int width = (EPD_WIDTH + 7) / 8;
    static uint8_t white = 0xff;
    send_command(DATA_START_TRANSMISSION_1);
    for(int row = 0; row < EPD_HEIGHT; ++row)
    {
        for(int col = 0; col < width; ++col)
        {
            send_data(&white, 1);
        }
    }

    send_command(DATA_START_TRANSMISSION_2);
    for(int row = 0; row < EPD_HEIGHT; ++row)
    {
        for(int col = 0; col < width; ++col)
        {
            send_data(&white, 1);
        }
    }

    epd_refresh();
}

void epd_display(void* framebuffer) 
{
    send_command(DATA_START_TRANSMISSION_1);
    send_data(framebuffer, EPD_BYTES);

    send_command(DATA_START_TRANSMISSION_2);
    send_data(framebuffer, EPD_BYTES);

    epd_refresh();
}
