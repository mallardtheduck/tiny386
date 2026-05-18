#include <unistd.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "common.h"
#include "i2c.h"

#include "../../i8042.h"

#define KEYBOARD_I2C_ADDRESS 0x5F
#define SECONADRY_I2C_ADDRESS 0x6F

#define KF_NORMAL 0
#define KF_SHIFT 1
#define KF_PREFIX_E0 2
#define KF_ALT 4
#define KF_CTRL 8

#define ALT_TOGGLE_KEY 0xA6
#define CTRL_TOGGLE_KEY 0xA7
#define SHIFT_TOGGLE_KEY 0xA8

#define KEY_ALT_SCANCODE 0x38
#define KEY_CTRL_SCANCODE 0x1D
#define KEY_SHIFT_SCANCODE 0x2A

#define MOUSE_BIT_UP 	BIT(1)
#define MOUSE_BIT_DOWN 	BIT(0)
#define MOUSE_BIT_LEFT	BIT(3)
#define MOUSE_BIT_RIGHT	BIT(2)
#define MOUSE_BIT_BTN1	BIT(6)
#define MOUSE_BIT_BTN2	BIT(5)
#define MOUSE_BIT_CLICK	BIT(4)

#define MIN_MOUSE_SPEED 2
#define MAX_MOUSE_SPEED 20
#define MOUSE_ACCEL_RATE 2

#define BIT2VALUE(bt, by, va) (((bt) & (by)) ? (va) : 0)

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  ((byte) & 0x80 ? '1' : '0'), \
  ((byte) & 0x40 ? '1' : '0'), \
  ((byte) & 0x20 ? '1' : '0'), \
  ((byte) & 0x10 ? '1' : '0'), \
  ((byte) & 0x08 ? '1' : '0'), \
  ((byte) & 0x04 ? '1' : '0'), \
  ((byte) & 0x02 ? '1' : '0'), \
  ((byte) & 0x01 ? '1' : '0') 

typedef struct{
	uint8_t key;
	uint8_t scancode;
	uint8_t flags;
} key_conv;

static key_conv keys[] = {
	{0x1B, 0x01, KF_NORMAL}, //ESC
	{0x0D, 0x1C, KF_NORMAL}, //Enter
	{0x20, 0x39, KF_NORMAL}, //Space
	{0x08, 0x0E, KF_NORMAL}, //Backspace
	{0x09, 0x0F, KF_NORMAL}, //Tab

	{0x31, 0x02, KF_NORMAL}, //1
	{0x32, 0x03, KF_NORMAL}, //2
	{0x33, 0x04, KF_NORMAL}, //3
	{0x34, 0x05, KF_NORMAL}, //4
	{0x35, 0x06, KF_NORMAL}, //5
	{0x36, 0x07, KF_NORMAL}, //6
	{0x37, 0x08, KF_NORMAL}, //7
	{0x38, 0x09, KF_NORMAL}, //8
	{0x39, 0x0A, KF_NORMAL}, //9
	{0x30, 0x0B, KF_NORMAL}, //0

	{0x71, 0x10, KF_NORMAL}, //q
	{0x77, 0x11, KF_NORMAL}, //w
	{0x65, 0x12, KF_NORMAL}, //e
	{0x72, 0x13, KF_NORMAL}, //r
	{0x74, 0x14, KF_NORMAL}, //t
	{0x79, 0x15, KF_NORMAL}, //y
	{0x75, 0x16, KF_NORMAL}, //u
	{0x69, 0x17, KF_NORMAL}, //i
	{0x6F, 0x18, KF_NORMAL}, //o
	{0x70, 0x19, KF_NORMAL}, //p

	{0x61, 0x1E, KF_NORMAL}, //a
	{0x73, 0x1F, KF_NORMAL}, //s
	{0x64, 0x20, KF_NORMAL}, //d
	{0x66, 0x21, KF_NORMAL}, //f
	{0x67, 0x22, KF_NORMAL}, //g
	{0x68, 0x23, KF_NORMAL}, //h
	{0x6A, 0x24, KF_NORMAL}, //j
	{0x6B, 0x25, KF_NORMAL}, //k
	{0x6C, 0x26, KF_NORMAL}, //l

	{0x7A, 0x2C, KF_NORMAL}, //z
	{0x78, 0x2D, KF_NORMAL}, //x
	{0x63, 0x2E, KF_NORMAL}, //c
	{0x76, 0x2F, KF_NORMAL}, //v
	{0x62, 0x30, KF_NORMAL}, //b
	{0x6E, 0x31, KF_NORMAL}, //n
	{0x6D, 0x32, KF_NORMAL}, //m

	{0x2C, 0x33, KF_NORMAL}, //,
	{0x2E, 0x34, KF_NORMAL}, //.

	{0x51, 0x10, KF_SHIFT}, //Q
	{0x57, 0x11, KF_SHIFT}, //W
	{0x45, 0x12, KF_SHIFT}, //E
	{0x52, 0x13, KF_SHIFT}, //R
	{0x54, 0x14, KF_SHIFT}, //T
	{0x59, 0x15, KF_SHIFT}, //Y
	{0x55, 0x16, KF_SHIFT}, //U
	{0x49, 0x17, KF_SHIFT}, //I
	{0x4F, 0x18, KF_SHIFT}, //O
	{0x50, 0x19, KF_SHIFT}, //P

	{0x41, 0x1E, KF_SHIFT}, //A
	{0x53, 0x1F, KF_SHIFT}, //S
	{0x44, 0x20, KF_SHIFT}, //D
	{0x46, 0x21, KF_SHIFT}, //F
	{0x47, 0x22, KF_SHIFT}, //G
	{0x48, 0x23, KF_SHIFT}, //H
	{0x4A, 0x24, KF_SHIFT}, //J
	{0x4B, 0x25, KF_SHIFT}, //K
	{0x4C, 0x26, KF_SHIFT}, //L

	{0x5A, 0x2C, KF_SHIFT}, //Z
	{0x58, 0x2D, KF_SHIFT}, //X
	{0x43, 0x2E, KF_SHIFT}, //C
	{0x56, 0x2F, KF_SHIFT}, //V
	{0x42, 0x30, KF_SHIFT}, //B
	{0x4E, 0x31, KF_SHIFT}, //N
	{0x4D, 0x32, KF_SHIFT}, //M

	{0x21, 0x02, KF_SHIFT}, //!
	{0x40, 0x03, KF_SHIFT}, //@
	{0x23, 0x04, KF_SHIFT}, //#
	{0x24, 0x05, KF_SHIFT}, //$
	{0x25, 0x06, KF_SHIFT}, //%
	{0x5E, 0x07, KF_SHIFT}, //^
	{0x26, 0x08, KF_SHIFT}, //&
	{0x2A, 0x09, KF_SHIFT}, //*
	{0x28, 0x0A, KF_SHIFT}, //(
	{0x29, 0x0B, KF_SHIFT}, //)

	{0x7B, 0x1A, KF_SHIFT}, //{
	{0x7D, 0x1B, KF_SHIFT}, //}
	{0x5B, 0x1A, KF_NORMAL}, //[
	{0x5D, 0x1B, KF_NORMAL}, //]
	{0x2F, 0x35, KF_NORMAL}, ///
	{0x5C, 0x2B, KF_NORMAL}, //\.
	{0x7C, 0x2B, KF_SHIFT}, //|
	{0x7E, 0x29, KF_SHIFT}, //~
	{0x27, 0x28, KF_NORMAL}, //'
	{0x22, 0x28, KF_SHIFT}, //"

	{0x3B, 0x27, KF_NORMAL}, //;
	{0x3A, 0x27, KF_SHIFT}, //:
	{0x60, 0x29, KF_NORMAL}, //`
	{0x2B, 0x0D, KF_SHIFT}, //+
	{0x2D, 0x0C, KF_NORMAL}, //-
	{0x5F, 0x0C, KF_SHIFT}, //_
	{0x3D, 0x0D, KF_NORMAL}, //=
	{0x3F, 0x35, KF_SHIFT}, //?

	{0x3C, 0x33, KF_SHIFT}, //<
	{0x3E, 0x34, KF_SHIFT}, //>

	{0x81, 0x3B, KF_NORMAL}, //F1
	{0x82, 0x3C, KF_NORMAL}, //F2
	{0x83, 0x3D, KF_NORMAL}, //F3
	{0x84, 0x3E, KF_NORMAL}, //F4
	{0x85, 0x3F, KF_NORMAL}, //F5
	{0x86, 0x40, KF_NORMAL}, //F6
	{0x87, 0x41, KF_NORMAL}, //F7
	{0x88, 0x42, KF_NORMAL}, //F8
	{0x89, 0x43, KF_NORMAL}, //F9
	{0x8A, 0x44, KF_NORMAL}, //F10

	{0xB5, 0x48, KF_PREFIX_E0}, //Up
	{0xB6, 0x50, KF_PREFIX_E0}, //Down
	{0xB4, 0x4B, KF_PREFIX_E0}, //Left
	{0xB7, 0x4D, KF_PREFIX_E0}, //Right

	{0x99, 0x49, KF_PREFIX_E0}, //Page up
	{0xA4, 0x51, KF_PREFIX_E0}, //Page down
	{0x98, 0x47, KF_PREFIX_E0}, //Home
	{0xA5, 0x4F, KF_PREFIX_E0}, //End

	{0x8B, 0x53, KF_PREFIX_E0}, //Del

	{0x8D, 0x3E, KF_ALT}, // FN+Q = Alt+F4
	{0x8E, 0x3E, KF_CTRL}, // FN+W = Ctrl+F4

	{0, 0, 0} //END
};

static bool alt_toggle = false;
static bool ctrl_toggle = false;
static bool shift_toggle = false;

static key_conv *cur_key = NULL;

static key_conv* get_conv(uint8_t key){
	for(key_conv* ckey = keys; ckey->key != 0; ++ckey){
		if(ckey->key == key) return ckey;
	}
	return NULL;
}

static void send_key(key_conv *key, int is_down){
	if(key->flags & KF_SHIFT){
		ps2_put_keycode(globals.kbd, is_down, KEY_SHIFT_SCANCODE);
	}
	if(key->flags & KF_PREFIX_E0){
		ps2_put_keycode(globals.kbd, is_down, 0xE0);
	}
	if(key->flags & KF_ALT){
		ps2_put_keycode(globals.kbd, is_down, KEY_ALT_SCANCODE);
	}
	if(key->flags & KF_CTRL){
		ps2_put_keycode(globals.kbd, is_down, KEY_CTRL_SCANCODE);
	}
	ps2_put_keycode(globals.kbd, is_down, key->scancode);
}

static void i2c_keyboard(){
	if(cur_key){
		send_key(cur_key, 0);
		cur_key = NULL;
		if(alt_toggle){
			alt_toggle = !alt_toggle;
			ps2_put_keycode(globals.kbd, alt_toggle, KEY_ALT_SCANCODE);
		}
		if(ctrl_toggle){
			ctrl_toggle = !ctrl_toggle;
			ps2_put_keycode(globals.kbd, ctrl_toggle, KEY_CTRL_SCANCODE);
		}
		if(shift_toggle){
			shift_toggle = !shift_toggle;
			ps2_put_keycode(globals.kbd, shift_toggle, KEY_SHIFT_SCANCODE);
		}
	}
	uint8_t data_rd = 0;
	i2c_master_read_from_device(I2C_MASTER_NUM, KEYBOARD_I2C_ADDRESS, &data_rd, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
	if(data_rd){
		if(data_rd == ALT_TOGGLE_KEY){
			alt_toggle = !alt_toggle;
			ps2_put_keycode(globals.kbd, alt_toggle, KEY_ALT_SCANCODE);
		} else if(data_rd == CTRL_TOGGLE_KEY){
			ctrl_toggle = !ctrl_toggle;
			ps2_put_keycode(globals.kbd, ctrl_toggle, KEY_CTRL_SCANCODE);
		}else if(data_rd == SHIFT_TOGGLE_KEY){
			shift_toggle = !shift_toggle;
			ps2_put_keycode(globals.kbd, shift_toggle, KEY_SHIFT_SCANCODE);
		}else{
			key_conv *new_key = get_conv(data_rd);
			if(new_key){
				send_key(new_key, 1);
				cur_key = new_key;
			}
		}
	}
}

static void i2c_mouse(){
	static int last_raw_dx = 0;
	static int last_raw_dy = 0;
	static int last_btn = 0;
	static int cur_speed = MIN_MOUSE_SPEED;
	static int move_counter = 0;
	uint8_t data_rd;
	esp_err_t res1 = i2c_master_read_from_device(I2C_MASTER_NUM, SECONADRY_I2C_ADDRESS, &data_rd, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
	//printf("%x Mouse byte: " BYTE_TO_BINARY_PATTERN "\n", res1, BYTE_TO_BINARY(data_rd));
	int raw_dx = BIT2VALUE(MOUSE_BIT_RIGHT, data_rd, 1) + BIT2VALUE(MOUSE_BIT_LEFT, data_rd, -1);
	int raw_dy = BIT2VALUE(MOUSE_BIT_DOWN, data_rd, 1) + BIT2VALUE(MOUSE_BIT_UP, data_rd, -1);
	bool leftBtn = (data_rd & MOUSE_BIT_BTN1) || (data_rd & MOUSE_BIT_CLICK);
	bool rightBtn = (data_rd & MOUSE_BIT_BTN2);
	int btn = (leftBtn ? 1 : 0) | (rightBtn ? 2 : 0);
		if(raw_dx == last_raw_dx && raw_dy == last_raw_dy){
		++move_counter;
		if(move_counter > MOUSE_ACCEL_RATE && cur_speed < MAX_MOUSE_SPEED) ++cur_speed;
	}else{
		move_counter = 0;
		cur_speed = MIN_MOUSE_SPEED;
	}
	int dx = raw_dx * cur_speed;
	int dy = raw_dy * cur_speed;
	if(dx || dy || btn != last_btn){
		//printf("Mouse data: %d %d %d\n", dx, dy, btn);
		ps2_mouse_event(globals.mouse, dx, dy, 0, btn);
	}
	last_raw_dx = raw_dx;
	last_raw_dy = raw_dy;
	last_btn = btn;
}

void probe(){
	uint8_t data_rd = 0;
	printf("I2C PROBE START\n");
	for(uint8_t i = 0; i < 255; ++i){
		esp_err_t res = i2c_master_read_from_device(I2C_MASTER_NUM, i, &data_rd, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
		if(res == ESP_OK){
			printf("I2C: Device found at: %x (data:%x)\n", i, data_rd);
		}
	}
	printf("I2C PROBE END\n");
}

static void i2c_task(void *arg){
	while(!i2c_ok) taskYIELD();
	//probe();
	while(true){
		i2c_keyboard();
		i2c_mouse();
		taskYIELD();
		//vTaskDelay(30 / portTICK_PERIOD_MS);
	}
}

void i2c_main(){
	xTaskCreatePinnedToCore(i2c_task, "i2c_task", 4096, NULL, 0, NULL, 0);
}