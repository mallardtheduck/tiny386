#define BUILD_ESP32
#define STATIC_ALLOC

#define IRAM_ATTR_CPU_EXEC1 IRAM_ATTR

#define PSRAM_ALLOC_LEN (7 * 1024 * 1024)

#define BPP 16
//#define SCALE_3_2
//#define FULL_UPDATE
//#define SWAPXY
#define SWAP_BYTEORDER_BPP16
#define USE_LCD_LCD43
#define LCD_WIDTH 800
#define LCD_HEIGHT 480

#define SD_CLK 12
#define SD_CMD 11
#define SD_D0 13

// #define I2S_MCLK I2S_GPIO_UNUSED
// #define I2S_BCLK 42
// #define I2S_WS   2
// #define I2S_DOUT 41

void task_yield();