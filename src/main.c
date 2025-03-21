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
#include <unistd.h>

#include "tamalib/tamalib.h"
#include "ws.h"
#include "cjson/cJSON.h"

#include "program.h"
#include "state.h"
#include "base64singleline.h"

#define WS_PORT 			8080
#define FRM_TXT  1
#define FRM_BIN  2
#define FRM_CLSE 8
#define FRM_FIN 128
#define FRM_MSK 128

#define BASE64_STATE_SIZE 1304
// The size of a base-64 encoded state snapshot. Calculated from the size in
// bytes (63 + INT_SLOT_NUM * 3 + MEM_RAM_SIZE + MEM_IO_SIZE = 977; see
// state.h) and the formula ceil(n_bytes / 3) * 4. This is used to validate the
// payload by the client on lod events.

#define BASE64_ROM_SIZE 16384
// The size of a base-64 encoded state snapshot. Measured. This is used to
// validate the payload by the client on rom events.

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

typedef enum {
	SPEED_UNLIMITED = 0,
	SPEED_1X = 1,
	SPEED_10X = 10,
} emulation_speed_t;

bool g_end_action = false;
bool g_sav_action = false;
bool g_lod_action = false;
char *g_load_state_save_b64;
char *g_rom_b64 = NULL;

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
	ws_sendframe_bcast(WS_PORT, msg, msg_size - 1, FRM_TXT);

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

static void update_screen(const bool skip_identical_frames)
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
	if (!skip_identical_frames | strcmp(msg, previous_msg) != 0) {
        ws_sendframe_bcast(WS_PORT, msg, msg_size - 1, FRM_TXT);
        strcpy(previous_msg, msg);
	}
}

static void hal_update_screen(void)
{
	update_screen(true);
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
		ws_sendframe_bcast(WS_PORT, msg, msg_size - 1, FRM_TXT);
	}
}

static void state_save_to_ws()
{
	size_t save_size;
	const uint8_t *save = state_save(&save_size);
	unsigned char *save_b64 = base64singleline_encode(save, save_size, NULL);
	char msg_template[] = "{\"t\":\"sav\",\"e\":{\"s\":\"%s\"}}";
	size_t msg_size = snprintf(NULL, 0, msg_template, save_b64);
	msg_size++;
	char *msg = (char *)malloc(msg_size);
	snprintf(msg, msg_size, msg_template, save_b64);
	ws_sendframe_bcast(WS_PORT, msg, msg_size - 1, FRM_TXT);
}

static void state_load_from_ws()
{
	size_t out_len;
	uint8_t *save = base64_decode(
		(unsigned char *) g_load_state_save_b64,
		strlen(g_load_state_save_b64),
		&out_len);
	state_load(save);
	free(g_load_state_save_b64);
	free(save);
}

static int hal_handler(void)
{
	tamalib_set_button(BTN_LEFT, btn_buffer[BTN_LEFT]);
	tamalib_set_button(BTN_MIDDLE, btn_buffer[BTN_MIDDLE]);
	tamalib_set_button(BTN_RIGHT, btn_buffer[BTN_RIGHT]);
	tamalib_set_button(BTN_TAP, btn_buffer[BTN_TAP]);

	if (g_sav_action == true) {
		state_save_to_ws();
        g_sav_action = false;
	}

	if (g_lod_action == true) {
		state_load_from_ws();
		g_lod_action = false;
	}

	return g_end_action;
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
	update_screen(false);
}

void onclose(ws_cli_conn_t  client)
{
	((void)client);
	printf("Disconnected!\n");
}

int handle_ws_event_rom(const cJSON *json) {
	const cJSON *r = NULL;
	int status = 0;

	r = cJSON_GetObjectItemCaseSensitive(json, "r");
	if (r == NULL) {
		fprintf(stderr, "lod event: no item \"r\"\n");
		status = 1;
		goto end;
	}
	if (!cJSON_IsString(r)) {
		fprintf(stderr, "lod event: item \"r\" has invalid type\n");
		status = 1;
		goto end;
	}
	if (r->valuestring == NULL) {
		fprintf(stderr, "lod event: item \"r\" is a null pointer\n");
		status = 1;
		goto end;
	}
	const unsigned long int len = strlen(r->valuestring);
	if (len != BASE64_ROM_SIZE) {
		fprintf(
			stderr,
			"lod event: item \"r\" is the wrong size: expected %d but got %lu\n",
			BASE64_ROM_SIZE,
			len);
		status = 1;
		goto end;
	}

	g_rom_b64 = r->valuestring;

	end:
		return status;
}

int handle_ws_event_btn(const cJSON *json) {
	const cJSON *b = NULL;
	const cJSON *s = NULL;
	int status = 0;

	// button code
	b = cJSON_GetObjectItemCaseSensitive(json, "b");
	if (b == NULL) {
		fprintf(stderr, "btn event: no item \"b\"\n");
		status = 1;
		goto end;
	}
	if (!cJSON_IsNumber(b)) {
		fprintf(stderr, "btn event: item \"b\" has invalid type\n");
    	status = 1;
    	goto end;
    }
	const int btn_code = b->valueint;
	if (!(btn_code == BTN_LEFT || btn_code == BTN_RIGHT ||
		  btn_code == BTN_MIDDLE || btn_code == BTN_TAP))
	{
		fprintf(stderr, "btn event: invalid button code \"b\": %d\n",
		        btn_code);
		status = 1;
		goto end;
	}

	// button state
    s = cJSON_GetObjectItemCaseSensitive(json, "s");
	if (s == NULL) {
		fprintf(stderr, "btn event: no item \"s\"\n");
		status = 1;
		goto end;
	}
	if (!cJSON_IsNumber(s)) {
		fprintf(stderr, "btn event: item \"s\" has invalid type\n");
		status = 1;
		goto end;
	}
	const int btn_status = s->valueint;
	if (!(btn_status == BTN_STATE_PRESSED || btn_status == BTN_STATE_RELEASED)) {
		fprintf(stderr, "btn event: invalid button status \"s\": %d\n",
		        btn_status);
		status = 1;
		goto end;
	}

	btn_buffer[btn_code] = btn_status;

	end:
		return status;
}

int handle_ws_event_mod(const cJSON *json) {
	const cJSON *m = NULL;
	int status = 0;

	m = cJSON_GetObjectItemCaseSensitive(json, "m");
	if (m == NULL) {
		fprintf(stderr, "mod event: no item \"m\"\n");
		status = 1;
		goto end;
	}
	if (!cJSON_IsNumber(m)) {
		fprintf(stderr, "mod event: item \"m\" has invalid type\n");
		status = 1;
		goto end;
	}
	const int mod_code = m->valueint;
	if (!(mod_code == EXEC_MODE_PAUSE || mod_code == EXEC_MODE_RUN ||
		  mod_code == EXEC_MODE_STEP || mod_code == EXEC_MODE_NEXT ||
		  mod_code == EXEC_MODE_TO_CALL || mod_code == EXEC_MODE_TO_RET))
	{
		fprintf(stderr, "mod event: invalid button code \"m\": %d\n",
				mod_code);
		status = 1;
		goto end;
	}

	tamalib_set_exec_mode(mod_code);

	end:
		return status;
}

int handle_ws_event_spd(const cJSON *json) {
	const cJSON *s = NULL;
	int status = 0;

	s = cJSON_GetObjectItemCaseSensitive(json, "s");
	if (s == NULL) {
		fprintf(stderr, "spd event: no item \"s\"\n");
		status = 1;
		goto end;
	}
	if (!cJSON_IsNumber(s)) {
		fprintf(stderr, "spd event: item \"s\" has invalid type\n");
		status = 1;
		goto end;
	}
	const int spd_code = s->valueint;
	if (!(spd_code == SPEED_UNLIMITED || spd_code == SPEED_1X || spd_code == SPEED_10X))
	{
		fprintf(stderr, "spd event: invalid button code \"s\": %d\n",
				spd_code);
		status = 1;
		goto end;
	}

	tamalib_set_speed(spd_code);

	end:
		return status;
}

int handle_ws_event_end() {
	g_end_action = true;
	return 0;
}

int handle_ws_event_sav(const cJSON *json) {
	g_sav_action = true;
	return 0;
}

int handle_ws_event_lod(const cJSON *json) {
	const cJSON *s = NULL;
	int status = 0;

	s = cJSON_GetObjectItemCaseSensitive(json, "s");
	if (s == NULL) {
		fprintf(stderr, "lod event: no item \"s\"\n");
		status = 1;
		goto end;
	}
	if (!cJSON_IsString(s)) {
		fprintf(stderr, "lod event: item \"s\" has invalid type\n");
		status = 1;
		goto end;
	}
	if (s->valuestring == NULL) {
		fprintf(stderr, "lod event: item \"s\" is a null pointer\n");
		status = 1;
		goto end;
	}
	const unsigned long int len = strlen(s->valuestring);
	if (len != BASE64_STATE_SIZE) {
		fprintf(
			stderr,
			"lod event: item \"s\" is the wrong size: expected %d but got %lu\n",
			BASE64_STATE_SIZE,
			len);
		status = 1;
		goto end;
	}

	g_load_state_save_b64 = (char *)malloc(len + 1);
	strcpy(g_load_state_save_b64, s->valuestring);
	g_lod_action = true;

	end:
		return status;
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
        status = 1;
        goto end;
    }

	// event type
    t = cJSON_GetObjectItemCaseSensitive(json, "t");
	if (t == NULL) {
		fprintf(stderr, "WS message: no item \"t\"\n");
		status = 1;
		goto end;
	}
    if (!(cJSON_IsString(t) && (t->valuestring != NULL))) {
		fprintf(stderr, "WS message: item \"t\" has invalid type\n");
    	status = 1;
    	goto end;
    }

	// event payload
    e = cJSON_GetObjectItemCaseSensitive(json, "e");
	if (e == NULL) {
		fprintf(stderr, "WS message: no item \"e\"\n");
		status = 1;
		goto end;
	}

	if (!strcmp(t->valuestring, "rom")) {
		handle_ws_event_rom(e);
	}
	else if (!strcmp(t->valuestring, "btn")) {
		handle_ws_event_btn(e);
	}
	else if (!strcmp(t->valuestring, "mod")) {
		handle_ws_event_mod(e);
	}
	else if (!strcmp(t->valuestring, "spd")) {
		handle_ws_event_spd(e);
	}
	else if (!strcmp(t->valuestring, "end")) {
		handle_ws_event_end();
	}
	else if (!strcmp(t->valuestring, "sav")) {
		handle_ws_event_sav(e);
	}
	else if (!strcmp(t->valuestring, "lod")) {
		handle_ws_event_lod(e);
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

	// Wait for the program to be sent through the websocket
	while (g_rom_b64 == NULL) {
		sleep(1);
	}
	g_program = program_load_b64(g_rom_b64, &g_program_size);

    tamalib_register_hal(&hal);
    tamalib_init(g_program, NULL, 1000000);
	fprintf(stderr, "Starting emulation\n");
    tamalib_mainloop();
    tamalib_release();

	return 0;
}
