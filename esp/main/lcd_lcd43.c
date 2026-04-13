#ifdef USE_LCD_LCD43

/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_touch_gt911.h"

#include "common.h"

#define CONFIG_LCD43_LCD_TOUCH_CONTROLLER_GT911 0 // 1 initiates the touch, 0 closes the touch.

#define I2C_MASTER_SCL_IO           9       /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           8       /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              0       /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          400000                     /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000

#define GPIO_INPUT_IO_4    4
#define GPIO_INPUT_PIN_SEL  1ULL<<GPIO_INPUT_IO_4
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define LCD43_LCD_H_RES               (LCD_WIDTH)
#define LCD43_LCD_V_RES               (LCD_HEIGHT)
#define LCD43_LCD_PIXEL_CLOCK_HZ      (16 * 1000 * 1000)
#define LCD43_LCD_BIT_PER_PIXEL       (16)
#define LCD43_RGB_BIT_PER_PIXEL       (16)
#define LCD43_RGB_DATA_WIDTH          (16)
#define LCD43_RGB_BOUNCE_BUFFER_SIZE  (LCD43_LCD_H_RES * 0)
#define LCD43_LCD_IO_RGB_DISP         (-1)             // -1 if not used
#define LCD43_LCD_IO_RGB_VSYNC        (GPIO_NUM_3)
#define LCD43_LCD_IO_RGB_HSYNC        (GPIO_NUM_46)
#define LCD43_LCD_IO_RGB_DE           (GPIO_NUM_5)
#define LCD43_LCD_IO_RGB_PCLK         (GPIO_NUM_7)
#define LCD43_LCD_IO_RGB_DATA0        (GPIO_NUM_14)
#define LCD43_LCD_IO_RGB_DATA1        (GPIO_NUM_38)
#define LCD43_LCD_IO_RGB_DATA2        (GPIO_NUM_18)
#define LCD43_LCD_IO_RGB_DATA3        (GPIO_NUM_17)
#define LCD43_LCD_IO_RGB_DATA4        (GPIO_NUM_10)
#define LCD43_LCD_IO_RGB_DATA5        (GPIO_NUM_39)
#define LCD43_LCD_IO_RGB_DATA6        (GPIO_NUM_0)
#define LCD43_LCD_IO_RGB_DATA7        (GPIO_NUM_45)
#define LCD43_LCD_IO_RGB_DATA8        (GPIO_NUM_48)
#define LCD43_LCD_IO_RGB_DATA9        (GPIO_NUM_47)
#define LCD43_LCD_IO_RGB_DATA10       (GPIO_NUM_21)
#define LCD43_LCD_IO_RGB_DATA11       (GPIO_NUM_1)
#define LCD43_LCD_IO_RGB_DATA12       (GPIO_NUM_2)
#define LCD43_LCD_IO_RGB_DATA13       (GPIO_NUM_42)
#define LCD43_LCD_IO_RGB_DATA14       (GPIO_NUM_41)
#define LCD43_LCD_IO_RGB_DATA15       (GPIO_NUM_40)

#define LCD43_LCD_IO_RST              (-1)             // -1 if not used
#define LCD43_PIN_NUM_BK_LIGHT        (-1)    // -1 if not used
#define LCD43_LCD_BK_LIGHT_ON_LEVEL   (1)
#define LCD43_LCD_BK_LIGHT_OFF_LEVEL  !LCD43_LCD_BK_LIGHT_ON_LEVEL

#define LCD43_PIN_NUM_TOUCH_RST       (-1)            // -1 if not used
#define LCD43_PIN_NUM_TOUCH_INT       (-1)            // -1 if not used

static const char *TAG = "ESP LCD43";

esp_err_t waveshare_esp32_s3_rgb_lcd_init();

esp_err_t wavesahre_rgb_lcd_bl_on();
esp_err_t wavesahre_rgb_lcd_bl_off();

static esp_lcd_panel_handle_t the_panel_handle;

// VSYNC event callback function
IRAM_ATTR static bool rgb_lcd_on_vsync_event(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx)
{
    return 1;
}

#if CONFIG_LCD43_LCD_TOUCH_CONTROLLER_GT911
/**
 * @brief I2C master initialization
 */
static esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;

    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    // Configure I2C parameters
    i2c_param_config(i2c_master_port, &i2c_conf);

    // Install I2C driver
    return i2c_driver_install(i2c_master_port, i2c_conf.mode, 0, 0, 0);
}

// GPIO initialization
void gpio_init(void)
{
    // Zero-initialize the config structure
    gpio_config_t io_conf = {};
    // Disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    // Bit mask of the pins, use GPIO4 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    // Set as input mode
    io_conf.mode = GPIO_MODE_OUTPUT;

    gpio_config(&io_conf);
}

// Reset the touch screen
void waveshare_esp32_s3_touch_reset()
{
    uint8_t write_buf = 0x01;
    i2c_master_write_to_device(I2C_MASTER_NUM, 0x24, &write_buf, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);

    // Reset the touch screen. It is recommended to reset the touch screen before using it.
    write_buf = 0x2C;
    i2c_master_write_to_device(I2C_MASTER_NUM, 0x38, &write_buf, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    esp_rom_delay_us(100 * 1000);
    gpio_set_level(GPIO_INPUT_IO_4, 0);
    esp_rom_delay_us(100 * 1000);
    write_buf = 0x2E;
    i2c_master_write_to_device(I2C_MASTER_NUM, 0x38, &write_buf, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    esp_rom_delay_us(200 * 1000);
}

#endif

// Initialize RGB LCD
esp_err_t waveshare_esp32_s3_rgb_lcd_init()
{
    ESP_LOGI(TAG, "Install RGB LCD panel driver"); // Log the start of the RGB LCD panel driver installation
    esp_lcd_panel_handle_t panel_handle = NULL; // Declare a handle for the LCD panel
    esp_lcd_rgb_panel_config_t panel_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT, // Set the clock source for the panel
        .timings =  {
            .pclk_hz = LCD43_LCD_PIXEL_CLOCK_HZ, // Pixel clock frequency
            .h_res = LCD43_LCD_H_RES, // Horizontal resolution
            .v_res = LCD43_LCD_V_RES, // Vertical resolution
            .hsync_pulse_width = 4, // Horizontal sync pulse width
            .hsync_back_porch = 8, // Horizontal back porch
            .hsync_front_porch = 8, // Horizontal front porch
            .vsync_pulse_width = 4, // Vertical sync pulse width
            .vsync_back_porch = 8, // Vertical back porch
            .vsync_front_porch = 8, // Vertical front porch
            .flags = {
                .pclk_active_neg = 1, // Active low pixel clock
            },
        },
        .data_width = LCD43_RGB_DATA_WIDTH, // Data width for RGB
        .bits_per_pixel = LCD43_RGB_BIT_PER_PIXEL, // Bits per pixel
        .num_fbs = 1, // Number of frame buffers
        .bounce_buffer_size_px = LCD43_RGB_BOUNCE_BUFFER_SIZE, // Bounce buffer size in pixels
        .sram_trans_align = 4, // SRAM transaction alignment
        .psram_trans_align = 64, // PSRAM transaction alignment
        .hsync_gpio_num = LCD43_LCD_IO_RGB_HSYNC, // GPIO number for horizontal sync
        .vsync_gpio_num = LCD43_LCD_IO_RGB_VSYNC, // GPIO number for vertical sync
        .de_gpio_num = LCD43_LCD_IO_RGB_DE, // GPIO number for data enable
        .pclk_gpio_num = LCD43_LCD_IO_RGB_PCLK, // GPIO number for pixel clock
        .disp_gpio_num = LCD43_LCD_IO_RGB_DISP, // GPIO number for display
        .data_gpio_nums = {
            LCD43_LCD_IO_RGB_DATA0,
            LCD43_LCD_IO_RGB_DATA1,
            LCD43_LCD_IO_RGB_DATA2,
            LCD43_LCD_IO_RGB_DATA3,
            LCD43_LCD_IO_RGB_DATA4,
            LCD43_LCD_IO_RGB_DATA5,
            LCD43_LCD_IO_RGB_DATA6,
            LCD43_LCD_IO_RGB_DATA7,
            LCD43_LCD_IO_RGB_DATA8,
            LCD43_LCD_IO_RGB_DATA9,
            LCD43_LCD_IO_RGB_DATA10,
            LCD43_LCD_IO_RGB_DATA11,
            LCD43_LCD_IO_RGB_DATA12,
            LCD43_LCD_IO_RGB_DATA13,
            LCD43_LCD_IO_RGB_DATA14,
            LCD43_LCD_IO_RGB_DATA15,
        },
        .flags = {
            .fb_in_psram = 1, // Use PSRAM for framebuffer
        },
    };

    // Create a new RGB panel with the specified configuration
    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &panel_handle));

    ESP_LOGI(TAG, "Initialize RGB LCD panel"); // Log the initialization of the RGB LCD panel
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle)); // Initialize the LCD panel

    esp_lcd_touch_handle_t tp_handle = NULL; // Declare a handle for the touch panel
#if CONFIG_LCD43_LCD_TOUCH_CONTROLLER_GT911
    ESP_LOGI(TAG, "Initialize I2C bus"); // Log the initialization of the I2C bus
    i2c_master_init(); // Initialize the I2C master
    ESP_LOGI(TAG, "Initialize GPIO"); // Log GPIO initialization
    gpio_init(); // Initialize GPIO pins
    ESP_LOGI(TAG, "Initialize Touch LCD"); // Log touch LCD initialization
    waveshare_esp32_s3_touch_reset(); // Reset the touch panel

    esp_lcd_panel_io_handle_t tp_io_handle = NULL; // Declare a handle for touch panel I/O
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG(); // Configure I2C for GT911 touch controller
	tp_io_config.scl_speed_hz = 0;

    ESP_LOGI(TAG, "Initialize I2C panel IO"); // Log I2C panel I/O initialization
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)I2C_MASTER_NUM, &tp_io_config, &tp_io_handle)); // Create new I2C panel I/O

    ESP_LOGI(TAG, "Initialize touch controller GT911"); // Log touch controller initialization
    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = LCD43_LCD_H_RES, // Set maximum X coordinate
        .y_max = LCD43_LCD_V_RES, // Set maximum Y coordinate
        .rst_gpio_num = LCD43_PIN_NUM_TOUCH_RST, // GPIO number for reset
        .int_gpio_num = LCD43_PIN_NUM_TOUCH_INT, // GPIO number for interrupt
        .levels = {
            .reset = 0, // Reset level
            .interrupt = 0, // Interrupt level
        },
        .flags = {
            .swap_xy = 0, // No swap of X and Y
            .mirror_x = 0, // No mirroring of X
            .mirror_y = 0, // No mirroring of Y
        },
    };
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &tp_handle)); // Create new I2C GT911 touch controller
#endif // CONFIG_LCD43_LCD_TOUCH_CONTROLLER_GT911

    // Register callbacks for RGB panel events
    esp_lcd_rgb_panel_event_callbacks_t cbs = {
#if LCD43_RGB_BOUNCE_BUFFER_SIZE > 0
        .on_bounce_frame_finish = rgb_lcd_on_vsync_event, // Callback for bounce frame finish
#else
        .on_vsync = rgb_lcd_on_vsync_event, // Callback for vertical sync
#endif
    };
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(panel_handle, &cbs, NULL)); // Register event callbacks

	the_panel_handle = panel_handle;

    return ESP_OK; // Return success 
}

/******************************* Turn on the screen backlight **************************************/
esp_err_t wavesahre_rgb_lcd_bl_on()
{
    //Configure CH422G to output mode 
    uint8_t write_buf = 0x01;
    i2c_master_write_to_device(I2C_MASTER_NUM, 0x24, &write_buf, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);

    //Pull the backlight pin high to light the screen backlight 
    write_buf = 0x1E;
    i2c_master_write_to_device(I2C_MASTER_NUM, 0x38, &write_buf, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    return ESP_OK;
}

/******************************* Turn off the screen backlight **************************************/
esp_err_t wavesahre_rgb_lcd_bl_off()
{
    //Configure CH422G to output mode 
    uint8_t write_buf = 0x01;
    i2c_master_write_to_device(I2C_MASTER_NUM, 0x24, &write_buf, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);

    //Turn off the screen backlight by pulling the backlight pin low 
    write_buf = 0x1A;
    i2c_master_write_to_device(I2C_MASTER_NUM, 0x38, &write_buf, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    return ESP_OK;
}

/******************************* VGA Task **************************************/

void pc_vga_step(void *o);

void vga_task(void *arg)
{
	int core_id = esp_cpu_get_core_id();
	fprintf(stderr, "lcd43 vga runs on core %d\n", core_id);

	ESP_LOGI(TAG, "Init display");

	waveshare_esp32_s3_rgb_lcd_init();

	globals.panel = the_panel_handle;
	/* Signal i386_task that the LCD panel is ready */
	xEventGroupSetBits(global_event_group, BIT1);
	xEventGroupWaitBits(global_event_group,
			    BIT0,
			    pdFALSE,
			    pdFALSE,
			    portMAX_DELAY);

	while (1) {
		pc_vga_step(globals.pc);
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}

void lcd_draw(int x_start, int y_start, int x_end, int y_end, void *src)
{
	//ESP_LOGI(TAG, "lcd_draw(%d, %d, %d, %d, %p)\n", x_start, y_start, x_end, y_end, src);
	if (globals.panel) {
		ESP_ERROR_CHECK(
			esp_lcd_panel_draw_bitmap(
				globals.panel,
				x_start, y_start,
				x_end, y_end,
				src));
	}
}

#endif //USE_LCD_LCD43