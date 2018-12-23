#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

#include "epaper.h"

const int spi_dma_channel = 1;
#define row_length (EPD_WIDTH >> 3)
#define framebuffer_length (EPD_HEIGHT * ((EPD_WIDTH + 7) / 8))
static uint8_t framebuffer[framebuffer_length];

#define order(a, b) if (a > b) { int c = a; a = b; b = c; }

void set_pixel(int x, int y, int color)
{
    if (x < 0 || x >= EPD_WIDTH || y < 0 || y >= EPD_HEIGHT)
    {
        return;
    }

    int pos = (y * EPD_WIDTH + x) / 8;
    uint8_t bitmask = 0x80 >> (x & 0x7);
    if (color)
    {
        framebuffer[pos] |= bitmask;
    }
    else
    {
        framebuffer[pos] &= ~bitmask;
    }
}

void clear(int color)
{
    memset(framebuffer, color? 0xff : 0x00, framebuffer_length);
}

void refresh()
{
    epaper_wakeup();
    epaper_display((void*)(framebuffer));
    epaper_sleep();
}

static int clip(int a, int m)
{
    return (a < 0)? 0 : (a >= m)? m - 1 : a;
}

static void hLine(int x0, int x1, int y, int color)
{
    x0 = clip(x0, EPD_WIDTH);
    x1 = clip(x1, EPD_WIDTH);
    y =  clip(y, EPD_HEIGHT);
    order(x0, x1);

    uint8_t* p = framebuffer + y * row_length;
    uint8_t* end = p + (x1 >> 3);
    p += (x0 >> 3);

    int fillStart = x0 & 0x7;
    if (fillStart)
    {
        if (color)
        {
            *p |= (0xff >> fillStart);            
        }
        else
        {
            *p &= ~(0xff >> fillStart);            
        }
        ++p;
    }
    if (x0 == x1)
    {
        return;
    }
    uint8_t fill = (color)? 0xff : 0x00;
    while(p < end)
    {
        *p = fill;
        ++p;
    }
    int fillEnd = x1 & 0x7;
    if (fillEnd) 
    {
        if (color)
        {
            *p |= ~(0xff >> fillEnd);            
        }
        else
        {
            *p &= (0xff >> fillEnd);            
        }
    }

}

static void vLine(int x, int y0, int y1, int color)
{
    x  = clip(x, EPD_WIDTH);
    y0 = clip(y0, EPD_HEIGHT);
    y1 = clip(y1, EPD_HEIGHT);
    order(y0, y1);
    uint8_t* p =   framebuffer + y0 * row_length + (x >> 3);
    uint8_t* end = framebuffer + y1 * row_length + (x >> 3);
    uint8_t fill = 0x80 >> (x & 0x7);
    if (!color)
    {
        fill = ~fill;
    }
    while(p < end)
    {
        if (color)
        {
            *p |= fill;
        }
        else
        {
            *p &= fill;
        }
        p += row_length;
    }
}

static void plotLineLow(int x0, int y0, int x1, int y1, int color)
{
  int dx = x1 - x0;
  int dy = y1 - y0;
  if (dy == 0)
  {
      hLine(x0, x1, y0, color);
      return;
  }
  int yi = 1;
  if (dy < 0)
  {
    yi = -1;
    dy = -dy;
  }
  int D = 2*dy - dx;
  int y = y0;

  for(int x = x0; x < x1; ++x)
  {
    set_pixel(x, y, color);
    if (D > 0) 
    {
       y = y + yi;
       D = D - 2*dx;
    }
    D = D + 2*dy;
  }
}

static void plotLineHigh(int x0, int y0, int x1, int y1, int color)
{
  int dx = x1 - x0;
  if (dx == 0)
  {
      vLine(x0, y0, y1, color);
      return;
  }
  int dy = y1 - y0;
  int xi = 1;
  if (dx < 0) 
  {
    xi = -1;
    dx = -dx;
  }
  
  int D = 2*dx - dy;
  int x = x0;

  for(int y = y0; y < y1; ++y)
  {
    set_pixel(x, y, color);
    if (D > 0)
    {
       x = x + xi;
       D = D - 2*dy;
    }
    D = D + 2*dx;
  }
}

static void dCircle(int xc, int yc, int x, int y, int color) 
{ 
    set_pixel(xc+x, yc+y, color); 
    set_pixel(xc-x, yc+y, color); 
    set_pixel(xc+x, yc-y, color); 
    set_pixel(xc-x, yc-y, color); 
    set_pixel(xc+y, yc+x, color); 
    set_pixel(xc-y, yc+x, color); 
    set_pixel(xc+y, yc-x, color); 
    set_pixel(xc-y, yc-x, color); 
}

static void dFilledCircle(int xc, int yc, int x, int y, int color) 
{ 
    hLine(xc-x, xc+x, yc+y, color); 
    hLine(xc-x, xc+x, yc-y, color); 
    hLine(xc-y, xc+y, yc+x, color); 
    hLine(xc-y, xc+y, yc-x, color); 
}

void draw_line(int x0, int y0, int x1, int y1, int color)
{
    if (abs(y1 - y0) < abs(x1 - x0))
    {
        if (x0 > x1)
        {
            plotLineLow(x1, y1, x0, y0, color);
        }
        else
        {
            plotLineLow(x0, y0, x1, y1, color);
        }
    }
    else
    {
        if (y0 > y1)
        {
            plotLineHigh(x1, y1, x0, y0, color);
        }
        else
        {
            plotLineHigh(x0, y0, x1, y1, color);
        }
    }
}

void draw_circle(int xc, int yc, int r, int color) 
{ 
    int x = 0, y = r; 
    int d = 3 - 2 * r; 
    dCircle(xc, yc, x, y, color); 
    while (y >= x) 
    { 
        x++; 
        if (d > 0) 
        { 
            y--;  
            d = d + 4 * (x - y) + 10; 
        } 
        else
            d = d + 4 * x + 6; 
        dCircle(xc, yc, x, y, color); 
    } 
} 

void draw_filled_circle(int xc, int yc, int r, int color) 
{ 
    int x = 0, y = r; 
    int d = 3 - 2 * r; 
    dFilledCircle(xc, yc, x, y, color); 
    while (y >= x) 
    { 
        x++; 
        if (d > 0) 
        { 
            y--;  
            d = d + 4 * (x - y) + 10; 
        } 
        else
            d = d + 4 * x + 6; 
        dFilledCircle(xc, yc, x, y, color); 
    } 
} 

void draw_filled_rect(int x0, int y0, int x1, int y1, int color)
{
    order(y0, y1);
    while(y0 < y1)
    {
        hLine(x0, x1, y0, color);
        ++y0;
    }
}

void draw_rect(int x0, int y0, int x1, int y1, int color)
{
    vLine(x0, y0, y1, color);
    vLine(x1, y0, y1, color);
    hLine(x0, x1, y0, color);
    hLine(x0, x1, y1, color);
}

static spi_device_handle_t epaper_spi;

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

void epaper_init(void)
{
    spi_bus_config_t buscfg = {
        .miso_io_num = (-1),
        .mosi_io_num = EPD_PIN_DIN,
        .sclk_io_num = EPD_PIN_CLK,
        .quadwp_io_num = (-1),
        .quadhd_io_num = (-1),
        .max_transfer_sz = framebuffer_length
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
    ret = spi_bus_add_device(VSPI_HOST, &devcfg, &epaper_spi);
    assert(ret == ESP_OK);

    gpio_set_direction(EPD_PIN_RST, GPIO_MODE_OUTPUT);
    gpio_set_direction(EPD_PIN_DC, GPIO_MODE_OUTPUT);
    gpio_set_direction(EPD_PIN_BUSY, GPIO_MODE_INPUT);
}


static void epaper_wait(void) 
{
    do 
    {
        task_delay_ms(200);
    }
    while(!gpio_get_level(EPD_PIN_BUSY));
}

static void epaper_reset()
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
        ret = spi_device_transmit(epaper_spi, &t);  // Transmit!
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
    ret = spi_device_transmit(epaper_spi, &t);  // Transmit!
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

static void epaper_refresh()
{
    send_command(DISPLAY_REFRESH);
    epaper_wait();
}

void epaper_wakeup(void)
{
    epaper_reset();

    SEND_COMMAND(POWER_SETTING, {0x03, 0x00, 0x2b, 0x2b});
    SEND_COMMAND(BOOSTER_SOFT_START, {0x17, 0x17, 0x17});
    send_command(POWER_ON);
    epaper_wait();

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

void epaper_sleep(void)
{
    send_command(POWER_OFF);
    epaper_wait();
    SEND_COMMAND(DEEP_SLEEP, {0xa5});
}

void epaper_clear(void)
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

    epaper_refresh();
}

void epaper_display(void* framebuffer) 
{
    send_command(DATA_START_TRANSMISSION_1);
    send_data(framebuffer, framebuffer_length);

    send_command(DATA_START_TRANSMISSION_2);
    send_data(framebuffer, framebuffer_length);

    epaper_refresh();
}