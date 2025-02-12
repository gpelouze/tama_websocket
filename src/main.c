/*
 * Tama Websocket - Tamagotchi P1 emulator websocket server
 *
 * Copyright (C) 2025 Gabriel Pelouze <gabriel@pelouze.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <base64.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "tamalib/tamalib.h"
#include "ws.h"
#include "cjson/cJSON.h"

// FIXME program.h requires SDL2 just to load rom.bin.
// It should be replaced with something lighter.
#include "program.h"
#include "base64singleline.h"

#define WS_PORT 			8080
#define FRM_TXT  1
#define FRM_BIN  2
#define FRM_CLSE 8
#define FRM_FIN 128
#define FRM_MSK 128

#define ROM_PATH			"rom.bin"

static u12_t *g_program = NULL;		// The actual program that is executed
static uint32_t g_program_size = 0;

static u32_t current_freq = 0; // in dHz
static unsigned int sin_pos = 0;
static bool_t is_audio_playing = 0;

static bool_t matrix_buffer[LCD_HEIGHT][LCD_WIDTH] = {{0}};
static bool_t icon_buffer[ICON_NUM] = {0};
static bool_t btn_buffer[4] = {0};

static u8_t log_levels = LOG_ERROR | LOG_INFO;

static void hal_halt(void)
{
	char msg[] = "{\"t\":\"end\",\"e\":{}}";
	size_t size = 18;
	ws_sendframe_bcast(WS_PORT, msg, size, FRM_TXT);
	exit(EXIT_SUCCESS);
}

static bool_t hal_is_log_enabled(log_level_t level)
{
	return !!(log_levels & level);
}

static void hal_log(log_level_t level, char *buff, ...)
{
	va_list arglist;

	if (!(log_levels & level)) {
		return;
	}

	va_start(arglist, buff);

	vfprintf((level == LOG_ERROR) ? stderr : stdout, buff, arglist);

	char msg_template[] = "{\"t\":\"log\",\"e\":{\"l\":\"%d\",\"m\":\"%s\"}}";
	size_t msg_size = snprintf(NULL, 0, msg_template, level, buff);
	msg_size++;
	char *msg = (char *)malloc(msg_size);
	snprintf(msg, msg_size, msg_template, level, buff);
	ws_sendframe_bcast(WS_PORT, msg, msg_size, FRM_TXT);

	va_end(arglist);
}

static timestamp_t hal_get_timestamp(void)
{
	struct timespec time;

	clock_gettime(CLOCK_REALTIME, &time);
	return (time.tv_sec * 1000000 + time.tv_nsec/1000);
}

static void hal_sleep_until(timestamp_t ts)
{
#ifndef NO_SLEEP
	struct timespec t;
	int32_t remaining = (int32_t) (ts - hal_get_timestamp());

	/* Sleep for a bit more than what is needed */
	if (remaining > 0) {
		t.tv_sec = remaining / 1000000;
		t.tv_nsec = (remaining % 1000000) * 1000;
		nanosleep(&t, NULL);
	}
#else
	/* Wait instead of sleeping to get the highest possible accuracy
	 * NOTE: the accuracy still depends on the timestamp_t resolution.
	 */
	while ((int32_t) (ts - hal_get_timestamp()) > 0);
#endif
}

/**
 * @brief Encode a bool_t array to base64
 *
 * @param src array to encode
 * @param len src length
 * @param out_len output length
 * @return base64-encoded src
 *
 * @note src is an array of bytes used to represent bits, and is therefore
 * expected to only contain values 0x0 and 0x1.
 */
unsigned char * bool_t_to_base64(const bool_t *src, const size_t len,
	size_t *out_len)
{
	if (len % 8) {
		printf("bool_t_to_base64: cannot convert array of size %lu\n", len);
		return NULL;
	}
	size_t new_len = len / 8;
	unsigned char * new_arr = malloc(new_len);
	for (int i = 0; i < new_len; i++) {
		new_arr[i] = 0;
		for (int j = 0; j < 8; j++) {
            new_arr[i] |= src[i*8+(7-j)] << j;
		}
	}
	return base64singleline_encode(new_arr, new_len, out_len);
}

static void hal_update_screen(void)
{
	/* This uses a const msg_size allow for a static previous_msg.
	 * The message size is calculated as follows:
	 * - 88 chars for the matrix (512 bits base64-encoded)
	 * - 4 chars for the icons (8 bits base64-encoded)
	 * - 31 chars json overhead (length of msg_template, minus the two '%s')
	 * - 1 string terminator
	 *
	 * The number of chars for base64-encoded arrays is given by:
	 * ceil(n_bits / 8 / 3) * 4
	 */
	const size_t msg_size = 124;
	char msg[124] = {};
	static char previous_msg[124] = {};
	char msg_template[] = "{\"t\":\"scr\",\"e\":{\"m\":\"%s\",\"i\":\"%s\"}}";
	unsigned char *matrix_b64 = bool_t_to_base64((bool_t *)matrix_buffer, LCD_HEIGHT * LCD_WIDTH, NULL);
	unsigned char *icon_b64 = bool_t_to_base64(icon_buffer, ICON_NUM, NULL);
	snprintf(msg, msg_size, msg_template, matrix_b64, icon_b64);
	if (strcmp(msg, previous_msg)) {
        ws_sendframe_bcast(WS_PORT, msg, msg_size, FRM_TXT);
        strcpy(previous_msg, msg);
	}
}

static void hal_set_lcd_matrix(u8_t x, u8_t y, bool_t val)
{
	matrix_buffer[y][x] = val;
}

static void hal_set_lcd_icon(u8_t icon, bool_t val)
{
	icon_buffer[icon] = val;
}

static void hal_set_frequency(u32_t freq)
{
	if (current_freq != freq) {
		current_freq = freq;
		sin_pos = 0;
	}
}

static void hal_play_frequency(bool_t en)
{
	if (is_audio_playing != en) {
		is_audio_playing = en;
		char msg_template[] = "{\"t\":\"frq\",\"e\":{\"f\":%u,\"p\":%u,\"e\":%d}}";
		size_t msg_size = snprintf(NULL, 0, msg_template, current_freq, sin_pos, is_audio_playing);
		msg_size++;
		char *msg = (char *)malloc(msg_size);
		snprintf(msg, msg_size, msg_template, current_freq, sin_pos, is_audio_playing);
		ws_sendframe_bcast(WS_PORT, msg, msg_size, FRM_TXT);
	}
}

static int hal_handler(void)
{
	tamalib_set_button(BTN_LEFT, btn_buffer[BTN_LEFT]);
	tamalib_set_button(BTN_MIDDLE, btn_buffer[BTN_MIDDLE]);
	tamalib_set_button(BTN_RIGHT, btn_buffer[BTN_RIGHT]);
	tamalib_set_button(BTN_TAP, btn_buffer[BTN_TAP]);
	return 0;
}

static hal_t hal = {
    .halt = &hal_halt,
    .is_log_enabled = &hal_is_log_enabled,
    .log = &hal_log,
    .sleep_until = &hal_sleep_until,
    .get_timestamp = &hal_get_timestamp,
    .update_screen = &hal_update_screen,
    .set_lcd_matrix = &hal_set_lcd_matrix,
    .set_lcd_icon = &hal_set_lcd_icon,
    .set_frequency = &hal_set_frequency,
    .play_frequency = &hal_play_frequency,
    .handler = &hal_handler,
};

void onopen(ws_cli_conn_t client)
{
	((void)client);
	printf("Connected!\n");
}

void onclose(ws_cli_conn_t  client)
{
	((void)client);
	printf("Disconnected!\n");
}

int handle_ws_event_btn(const cJSON *json) {
	const cJSON *b = NULL;
	const cJSON *s = NULL;
	int status = 0;

	// button code
	b = cJSON_GetObjectItemCaseSensitive(json, "b");
	if (b == NULL) {
		fprintf(stderr, "btn event: no item \"b\"\n");
		status = 0;
		goto end;
	}
	if (!cJSON_IsNumber(b)) {
		fprintf(stderr, "btn event: item \"b\" has invalid type\n");
    	status = 0;
    	goto end;
    }
	const int btn_code = b->valueint;
	if (!(btn_code == BTN_LEFT || btn_code == BTN_RIGHT ||
		  btn_code == BTN_MIDDLE || btn_code == BTN_TAP))
	{
		fprintf(stderr, "btn event: invalid button code \"b\": %d\n",
		        btn_code);
    	status = 0;
    	goto end;
	}

	// button state
    s = cJSON_GetObjectItemCaseSensitive(json, "s");
	if (s == NULL) {
		fprintf(stderr, "btn event: no item \"s\"\n");
		status = 0;
		goto end;
	}
	if (!cJSON_IsNumber(s)) {
		fprintf(stderr, "btn event: item \"s\" has invalid type\n");
    	status = 0;
    	goto end;
    }
	const int btn_status = s->valueint;
	if (!(btn_status == BTN_STATE_PRESSED || btn_status == BTN_STATE_RELEASED)) {
		fprintf(stderr, "btn event: invalid button status \"s\": %d\n",
		        btn_status);
    	status = 0;
    	goto end;
	}

	btn_buffer[btn_code] = btn_status;

	end:
		return 0;
}

int handle_ws_message(const unsigned char *msg)
{
	const cJSON *t = NULL;
	const cJSON *e = NULL;
	int status = 0;
	cJSON *json = cJSON_Parse(msg);
	if (json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "WS message: JSON error before: %s\n", error_ptr);
        }
        status = 0;
        goto end;
    }

	// event type
    t = cJSON_GetObjectItemCaseSensitive(json, "t");
	if (t == NULL) {
		fprintf(stderr, "WS message: no item \"t\"\n");
		status = 0;
		goto end;
	}
    if (!(cJSON_IsString(t) && (t->valuestring != NULL))) {
		fprintf(stderr, "WS message: item \"t\" has invalid type\n");
    	status = 0;
    	goto end;
    }

	// event payload
    e = cJSON_GetObjectItemCaseSensitive(json, "e");
	if (e == NULL) {
		fprintf(stderr, "WS message: no item \"e\"\n");
		status = 0;
		goto end;
	}

	if (!strcmp(t->valuestring, "btn")) {
		handle_ws_event_btn(e);
	}
	else {
		fprintf(stderr, "WS message: unknown event type \"%s\"\n", t->valuestring);
	}

	end:
		cJSON_Delete(json);
		return status;
}

void onmessage(ws_cli_conn_t client,
	const unsigned char *msg, uint64_t size, int type)
{
	((void)client);
	((void)msg);
	((void)size);
	((void)type);
	char *client_address = ws_getaddress(client);
	printf("[%s] %s\n", client_address, msg);
	handle_ws_message(msg);
}

int main (int argc, const char * argv[]) {

	const char *WS_HOST = getenv("TAMA_WS_HOST");
	WS_HOST = (WS_HOST != NULL) ? WS_HOST: "127.0.0.1";

	struct ws_server ws = {
		.host = WS_HOST,
		.port = WS_PORT,
		.thread_loop   = 1,
		.timeout_ms    = 1000,
		.evs.onopen    = &onopen,
		.evs.onclose   = &onclose,
		.evs.onmessage = &onmessage
	};

	ws_socket(&ws);

	g_program = program_load(ROM_PATH, &g_program_size);

    tamalib_register_hal(&hal);
    tamalib_init(g_program, NULL, 1000000);
	fprintf(stderr, "Starting emulation\n");
    tamalib_mainloop();
    tamalib_release();

	return 0;
}
