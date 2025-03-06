#pragma once

/**--------------------------------------------------------------------------------------
 * ESP32S3 T-Display-S3
 *-------------------------------------------------------------------------------------*/

// Button pins
#define PIN_BUTTON_1 0
#define PIN_BUTTON_2 14

// Battery voltage pin
#define PIN_BAT_VOLT 4

// External expansion sd card pins
#define PIN_SD_CMD 13
#define PIN_SD_CLK 11
#define PIN_SD_D0 12

// Touch pad pins
#define PIN_IIC_SCL 32
#define PIN_IIC_SDA 33
#define PIN_TOUCH_INT 21
#define PIN_TOUCH_RES 25

// Display pins
#define PIN_LCD_POWER_ON -1
#define ST7789_DRIVER
#define TFT_RGB_ORDER TFT_BGR
#define TFT_WIDTH 240
#define TFT_HEIGHT 320
#define TFT_INVERSION_OFF
#define ESP32_DMA
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS 15
#define TFT_DC 2
#define TFT_RST -1
#define TFT_BL 27
#define TOUCH_CS 33