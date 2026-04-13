#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
SOCKET fd;
#else
#include <arpa/inet.h>
int fd;
#endif

void ps2_put_keycode(void *s, int is_down, int keycode)
{
	uint8_t data[1];
	data[0] = keycode & 0x7f;
	if (!is_down)
		data[0] |= 0x80;
	printf("put %x\n", data[0]);
	send(fd, data, 1, 0);
}

static uint8_t t(int x)
{
	uint8_t res = x;
	if (x > 127)
		res = 127;
	if (x < -127)
		res = -127;
	return res;
}

void ps2_mouse_event(void *s,
		     int dx, int dy, int dz, int buttons_state)
{
	uint8_t data[5];
	data[0] = 0;
	data[1] = t(dx);
	data[2] = t(dy);
	data[3] = t(dz);
	data[4] = buttons_state;
	int ret = send(fd, data, 5, 0);
	assert(ret == 5);
}

#include "SDL.h"
typedef struct {
	void *kbd;
	void *mouse;
} PC;

typedef struct {
	int width, height;
	SDL_Surface *screen;
	PC pc0, *pc;
} Console;
#define BPP 32

Console *console_init(int width, int height)
{
	Console *s = malloc(sizeof(Console));
	s->width = width;
	s->height = height;
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	s->screen = SDL_SetVideoMode(s->width, s->height, BPP, 0);
	s->pc = &(s->pc0);
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY*5,
			    SDL_DEFAULT_REPEAT_INTERVAL*5);
	SDL_WM_SetCaption("tiny386 - use ctrl + ] to grab/ungrab", NULL);
	return s;
}

/* we assume Xorg is used with a PC keyboard. Return 0 if no keycode found. */
// static int sdl_get_keycode(const SDL_KeyboardEvent *ev)
// {
// 	int keycode;
// 	keycode = ev->keysym.scancode;
// 	if (keycode == 0) {
// 		int sym = ev->keysym.sym;
// 		switch (sym) {
// 		case SDLK_UP: return 0x67;
// 		case SDLK_DOWN: return 0x6c;
// 		case SDLK_LEFT: return 0x69;
// 		case SDLK_RIGHT: return 0x6a;
// 		case SDLK_HOME: return 0x66;
// 		case SDLK_END: return 0x6b;
// 		case SDLK_PAGEUP: return 0x68;
// 		case SDLK_PAGEDOWN: return 0x6d;
// 		case SDLK_INSERT: return 0x6e;
// 		case SDLK_DELETE: return 0x6f;
// 		case SDLK_KP_DIVIDE: return 0x62;
// 		case SDLK_KP_ENTER: return 0x60;
// 		case SDLK_RCTRL: return 0x61;
// 		case SDLK_PAUSE: return 0x77;
// 		case SDLK_PRINT: return 0x63;
// 		case SDLK_RALT: return 0x64;
// 		default: printf("unknown %x %d\n", sym, sym); return 0;
// 		}
// 	}
// #ifndef _WIN32
// 	if (keycode < 9) {
// 		keycode = 0;
// 	} else if (keycode < 127 + 9) {
// 		keycode -= 8;
// 	} else {
// 		keycode = 0;
// 	}
// #endif
// 	return keycode;
// }

static int sdl_get_keycode(const SDL_KeyboardEvent *ev)
{
    switch (ev->keysym.sym) {
        case SDLK_ESCAPE: return 1;
        case SDLK_1: return 2;
        case SDLK_2: return 3;
        case SDLK_3: return 4;
        case SDLK_4: return 5;
        case SDLK_5: return 6;
        case SDLK_6: return 7;
        case SDLK_7: return 8;
        case SDLK_8: return 9;
        case SDLK_9: return 10;
        case SDLK_0: return 11;
        case SDLK_MINUS: return 12;
        case SDLK_EQUALS: return 13;
        case SDLK_BACKSPACE: return 14;
        case SDLK_TAB: return 15;
        case SDLK_q: return 16;
        case SDLK_w: return 17;
        case SDLK_e: return 18;
        case SDLK_r: return 19;
        case SDLK_t: return 20;
        case SDLK_y: return 21;
        case SDLK_u: return 22;
        case SDLK_i: return 23;
        case SDLK_o: return 24;
        case SDLK_p: return 25;
        case SDLK_LEFTBRACKET: return 26;
        case SDLK_RIGHTBRACKET: return 27;
        case SDLK_RETURN: return 28;
        case SDLK_LCTRL: return 29;
        case SDLK_a: return 30;
        case SDLK_s: return 31;
        case SDLK_d: return 32;
        case SDLK_f: return 33;
        case SDLK_g: return 34;
        case SDLK_h: return 35;
        case SDLK_j: return 36;
        case SDLK_k: return 37;
        case SDLK_l: return 38;
        case SDLK_SEMICOLON: return 39;
        case SDLK_QUOTE: return 40;
        case SDLK_BACKQUOTE: return 41;
        case SDLK_LSHIFT: return 42;
        case SDLK_BACKSLASH: return 43;
        case SDLK_z: return 44;
        case SDLK_x: return 45;
        case SDLK_c: return 46;
        case SDLK_v: return 47;
        case SDLK_b: return 48;
        case SDLK_n: return 49;
        case SDLK_m: return 50;
        case SDLK_COMMA: return 51;
        case SDLK_PERIOD: return 52;
        case SDLK_SLASH: return 53;
        case SDLK_RSHIFT: return 54;
        case SDLK_KP_MULTIPLY: return 55;
        case SDLK_LALT: return 56;
        case SDLK_SPACE: return 57;
        case SDLK_CAPSLOCK: return 58;
        case SDLK_F1: return 59;
        case SDLK_F2: return 60;
        case SDLK_F3: return 61;
        case SDLK_F4: return 62;
        case SDLK_F5: return 63;
        case SDLK_F6: return 64;
        case SDLK_F7: return 65;
        case SDLK_F8: return 66;
        case SDLK_F9: return 67;
        case SDLK_F10: return 68;
        case SDLK_NUMLOCK: return 69;
        case SDLK_SCROLLOCK: return 70;
        case SDLK_KP7: return 71;
        case SDLK_KP8: return 72;
        case SDLK_KP9: return 73;
        case SDLK_KP_MINUS: return 74;
        case SDLK_KP4: return 75;
        case SDLK_KP5: return 76;
        case SDLK_KP6: return 77;
        case SDLK_KP_PLUS: return 78;
        case SDLK_KP1: return 79;
        case SDLK_KP2: return 80;
        case SDLK_KP3: return 81;
        case SDLK_KP0: return 82;
        case SDLK_KP_PERIOD: return 83;
        case SDLK_F11: return 87;
        case SDLK_F12: return 88;
        case SDLK_KP_ENTER: return 96;
        case SDLK_RCTRL: return 97;
        case SDLK_KP_DIVIDE: return 98;
        case SDLK_SYSREQ: return 99;
        case SDLK_RALT: return 100;
        case SDLK_HOME: return 102;
        case SDLK_UP: return 103;
        case SDLK_PAGEUP: return 104;
        case SDLK_LEFT: return 105;
        case SDLK_RIGHT: return 106;
        case SDLK_END: return 107;
        case SDLK_DOWN: return 108;
        case SDLK_PAGEDOWN: return 109;
        case SDLK_INSERT: return 110;
        case SDLK_DELETE: return 111;
        case SDLK_KP_EQUALS: return 117;
        //case SDLK_KP_PLUSMINUS: return 118;
        case SDLK_PAUSE: return 119;
        //case SDLK_KP_COMMA: return 121;
        // case SDLK_LGUI: return 125;
        // case SDLK_RGUI: return 126;
        case SDLK_HELP: return 138;
        case SDLK_MENU: return 139;
        case SDLK_F13: return 183;
        case SDLK_F14: return 184;
        case SDLK_F15: return 185;
        default: return 0;
    }
}

/* release all pressed keys */
#define KEYCODE_MAX 127
static uint8_t key_pressed[KEYCODE_MAX + 1];

static void sdl_reset_keys(PC *pc)
{
	int i;
	for(i = 1; i <= KEYCODE_MAX; i++) {
		if (key_pressed[i]) {
			ps2_put_keycode(pc->kbd, 0, i);
			key_pressed[i] = 0;
		}
	}
}

static void sdl_handle_key_event(const SDL_KeyboardEvent *ev, PC *pc)
{
	int keycode, keypress;

	keycode = sdl_get_keycode(ev);
	if (keycode) {
#if SDL_PATCHLEVEL < 50 /* not sdl12-compat */
		if (keycode == 0x3a || keycode ==0x45) {
			/* SDL does not generate key up for numlock & caps lock */
			ps2_put_keycode(pc->kbd, 1, keycode);
			usleep(100);
			ps2_put_keycode(pc->kbd, 0, keycode);
		} else
#endif
		{
			keypress = (ev->type == SDL_KEYDOWN);
			if (keycode == 0x1b && keypress &&
			    (key_pressed[0x1d])) {
				static int en;
				en ^= 1;
				printf("en=%d\n", en);
				SDL_ShowCursor(en ? SDL_DISABLE : SDL_ENABLE);
				SDL_WM_GrabInput(en ? SDL_GRAB_ON : SDL_GRAB_OFF);
				return;
			}
			if (keycode <= KEYCODE_MAX)
				key_pressed[keycode] = keypress;
			ps2_put_keycode(pc->kbd, keypress, keycode);
		}
	} else if (ev->type == SDL_KEYUP) {
		/* workaround to reset the keyboard state (used when changing
		   desktop with ctrl-alt-x on Linux) */
		sdl_reset_keys(pc);
	}
}

static void sdl_send_mouse_event(PC *pc, int x1, int y1,
				 int dz, int state, bool is_absolute)
{
	int buttons, x, y;

	buttons = 0;
	if (state & SDL_BUTTON(SDL_BUTTON_LEFT))
		buttons |= (1 << 0);
	if (state & SDL_BUTTON(SDL_BUTTON_RIGHT))
		buttons |= (1 << 1);
	if (state & SDL_BUTTON(SDL_BUTTON_MIDDLE))
		buttons |= (1 << 2);
	if (is_absolute) {
		x = 0;//(x1 * 32768) / screen_width;
		y = 0;//(y1 * 32768) / screen_height;
	} else {
		x = x1;
		y = y1;
	}
	ps2_mouse_event(pc->mouse, x, y, dz, buttons);
}

#include <time.h>
static uint32_t get_uticks()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ((uint32_t) ts.tv_sec * 1000000 +
		(uint32_t) ts.tv_nsec / 1000);
}

static int after_eq(uint32_t a, uint32_t b)
{
	return (a - b) < (1u << 31);
}

static void sdl_handle_mouse_motion_event(const SDL_Event *ev, PC *pc)
{
	bool is_absolute = 0; //vm_mouse_is_absolute(m);
	int x, y;
	if (is_absolute) {
		x = ev->motion.x;
		y = ev->motion.y;
	} else {
		x = ev->motion.xrel;
		y = ev->motion.yrel;
	}
	static int x0, y0;
	x0 += x;
	y0 += y;

	static uint32_t last = 0;
	uint32_t now = get_uticks();
	if (last == 0 || after_eq(now, last)) {
		last = now + 24000;
		sdl_send_mouse_event(pc, x0, y0, 0, ev->motion.state, is_absolute);
		x0 = 0;
		y0 = 0;
	}
}

static void sdl_handle_mouse_button_event(const SDL_Event *ev, PC *pc)
{
	bool is_absolute = 0; //vm_mouse_is_absolute(m);
	int state, dz;

	dz = 0;
	if (ev->type == SDL_MOUSEBUTTONDOWN) {
		if (ev->button.button == SDL_BUTTON_WHEELUP) {
			dz = -1;
		} else if (ev->button.button == SDL_BUTTON_WHEELDOWN) {
			dz = 1;
		}
	}

	state = SDL_GetMouseState(NULL, NULL);
	/* just in case */
	if (ev->type == SDL_MOUSEBUTTONDOWN)
		state |= SDL_BUTTON(ev->button.button);
	else
		state &= ~SDL_BUTTON(ev->button.button);

	if (is_absolute) {
		sdl_send_mouse_event(pc, ev->button.x, ev->button.y,
				     dz, state, is_absolute);
	} else {
		sdl_send_mouse_event(pc, 0, 0, dz, state, is_absolute);
	}
}

static void poll(void *opaque)
{
	Console *s = opaque;
	SDL_Event ev;

	while (SDL_PollEvent(&ev)) {
		switch (ev.type) {
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			sdl_handle_key_event(&(ev.key), s->pc);
			break;
		case SDL_MOUSEMOTION:
			sdl_handle_mouse_motion_event(&ev, s->pc);
			break;
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			sdl_handle_mouse_button_event(&ev, s->pc);
			break;
		case SDL_QUIT:
			exit(0);
		}
	}
}

int main(int argc, char *argv[])
{
#ifdef _WIN32
	WSADATA Data;
	WSAStartup(MAKEWORD(2, 0), &Data);
#endif
	if (argc != 3)
		return 1;
	int ret;
	struct sockaddr_in addr;
	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		return -1;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(argv[2]));
	if (inet_pton(AF_INET, argv[1], &addr.sin_addr) <= 0) {
		return -2;
	}

	if ((ret = connect(fd, (struct sockaddr*)&addr, sizeof(addr))) < 0) {
		return -1;
	}

	Console *console = console_init(640, 480);
	for (;;) {
		poll(console);
		usleep(100);
	}
	return 0;
}
