#ifndef __EPD_H__
#define __EPD_H__

#define EPD_WIDTH  400
#define EPD_HEIGHT 300
#define EPD_BYTES_PER_ROW ((EPD_WIDTH+7)/8)
#define EPD_BYTES EPD_HEIGHT*EPD_BYTES_PER_ROW

// EPD4IN2 commands
#define PANEL_SETTING                               0x00
#define POWER_SETTING                               0x01
#define POWER_OFF                                   0x02
#define POWER_OFF_SEQUENCE_SETTING                  0x03
#define POWER_ON                                    0x04
#define POWER_ON_MEASURE                            0x05
#define BOOSTER_SOFT_START                          0x06
#define DEEP_SLEEP                                  0x07
#define DATA_START_TRANSMISSION_1                   0x10
#define DATA_STOP                                   0x11
#define DISPLAY_REFRESH                             0x12
#define DATA_START_TRANSMISSION_2                   0x13
#define LUT_FOR_VCOM                                0x20
#define LUT_WHITE_TO_WHITE                          0x21
#define LUT_BLACK_TO_WHITE                          0x22
#define LUT_WHITE_TO_BLACK                          0x23
#define LUT_BLACK_TO_BLACK                          0x24
#define PLL_CONTROL                                 0x30
#define TEMPERATURE_SENSOR_COMMAND                  0x40
#define TEMPERATURE_SENSOR_SELECTION                0x41
#define TEMPERATURE_SENSOR_WRITE                    0x42
#define TEMPERATURE_SENSOR_READ                     0x43
#define VCOM_AND_DATA_INTERVAL_SETTING              0x50
#define LOW_POWER_DETECTION                         0x51
#define TCON_SETTING                                0x60
#define RESOLUTION_SETTING                          0x61
#define GSST_SETTING                                0x65
#define GET_STATUS                                  0x71
#define AUTO_MEASUREMENT_VCOM                       0x80
#define READ_VCOM_VALUE                             0x81
#define VCM_DC_SETTING                              0x82
#define PARTIAL_WINDOW                              0x90
#define PARTIAL_IN                                  0x91
#define PARTIAL_OUT                                 0x92
#define PROGRAM_MODE                                0xA0
#define ACTIVE_PROGRAMMING                          0xA1
#define READ_OTP                                    0xA2
#define POWER_SAVING                                0xE3

#define EPD_PIN_BUSY 32
#define EPD_PIN_RST  23
#define EPD_PIN_DC   16
#define EPD_PIN_CS   17
#define EPD_PIN_CLK   5
#define EPD_PIN_DIN  18

#ifdef __cplusplus
extern "C" {
#endif

extern void epd_init(void);
extern void epd_uninit(void);

extern void epd_wakeup(void);
extern void epd_clear(void);
extern void epd_display(void* image);
extern void epd_sleep(void);

#ifdef __cplusplus
}
#endif

#endif /* __EPD_H__ */