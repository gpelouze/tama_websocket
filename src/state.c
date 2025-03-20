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
 *
 *
 * This file was derived from the file state.c of the TamaTool project, which
 * is distributed with the following copyright notice:
 *
 * TamaTool - A cross-platform Tamagotchi P1 explorer
 *
 * Copyright (C) 2021 Jean-Christophe Rona <jc@rona.fr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "tamalib/tamalib.h"

#define STATE_FILE_MAGIC				"TLST"
#define STATE_FILE_VERSION				3


unsigned char * state_save(size_t *out_len)
{
	state_t *state;
	uint32_t num = 0;
	uint32_t i;
	size_t save_size = 63 + INT_SLOT_NUM * 3 + MEM_RAM_SIZE + MEM_IO_SIZE;
	uint8_t * save = (uint8_t*)malloc(save_size);

	state = tamalib_get_state();

	/* First the magic, then the version, and finally the fields of
	 * the state_t struct written as u8, u16 little-endian or u32
	 * little-endian following the struct order
	 */
	save[0] = (uint8_t) STATE_FILE_MAGIC[0];
	save[1] = (uint8_t) STATE_FILE_MAGIC[1];
	save[2] = (uint8_t) STATE_FILE_MAGIC[2];
	save[3] = (uint8_t) STATE_FILE_MAGIC[3];
	num += 4;

	save[num] = STATE_FILE_VERSION & 0xFF;
	num += 1;

	save[num] = *(state->pc) & 0xFF;
	save[num+1] = (*(state->pc) >> 8) & 0x1F;
	num += 2;

	save[num] = *(state->x) & 0xFF;
	save[num+1] = (*(state->x) >> 8) & 0xF;
	num += 2;

	save[num] = *(state->y) & 0xFF;
	save[num+1] = (*(state->y) >> 8) & 0xF;
	num += 2;

	save[num] = *(state->a) & 0xF;
	num += 1;

	save[num] = *(state->b) & 0xF;
	num += 1;

	save[num] = *(state->np) & 0x1F;
	num += 1;

	save[num] = *(state->sp) & 0xFF;
	num += 1;

	save[num] = *(state->flags) & 0xF;
	num += 1;

	save[num] = *(state->tick_counter) & 0xFF;
	save[num+1] = (*(state->tick_counter) >> 8) & 0xFF;
	save[num+2] = (*(state->tick_counter) >> 16) & 0xFF;
	save[num+3] = (*(state->tick_counter) >> 24) & 0xFF;
	num += 4;

	save[num] = *(state->clk_timer_2hz_timestamp) & 0xFF;
	save[num+1] = (*(state->clk_timer_2hz_timestamp) >> 8) & 0xFF;
	save[num+2] = (*(state->clk_timer_2hz_timestamp) >> 16) & 0xFF;
	save[num+3] = (*(state->clk_timer_2hz_timestamp) >> 24) & 0xFF;
	num += 4;

	save[num] = *(state->clk_timer_4hz_timestamp) & 0xFF;
	save[num+1] = (*(state->clk_timer_4hz_timestamp) >> 8) & 0xFF;
	save[num+2] = (*(state->clk_timer_4hz_timestamp) >> 16) & 0xFF;
	save[num+3] = (*(state->clk_timer_4hz_timestamp) >> 24) & 0xFF;
	num += 4;

	save[num] = *(state->clk_timer_8hz_timestamp) & 0xFF;
	save[num+1] = (*(state->clk_timer_8hz_timestamp) >> 8) & 0xFF;
	save[num+2] = (*(state->clk_timer_8hz_timestamp) >> 16) & 0xFF;
	save[num+3] = (*(state->clk_timer_8hz_timestamp) >> 24) & 0xFF;
	num += 4;

	save[num] = *(state->clk_timer_16hz_timestamp) & 0xFF;
	save[num+1] = (*(state->clk_timer_16hz_timestamp) >> 8) & 0xFF;
	save[num+2] = (*(state->clk_timer_16hz_timestamp) >> 16) & 0xFF;
	save[num+3] = (*(state->clk_timer_16hz_timestamp) >> 24) & 0xFF;
	num += 4;

	save[num] = *(state->clk_timer_32hz_timestamp) & 0xFF;
	save[num+1] = (*(state->clk_timer_32hz_timestamp) >> 8) & 0xFF;
	save[num+2] = (*(state->clk_timer_32hz_timestamp) >> 16) & 0xFF;
	save[num+3] = (*(state->clk_timer_32hz_timestamp) >> 24) & 0xFF;
	num += 4;

	save[num] = *(state->clk_timer_64hz_timestamp) & 0xFF;
	save[num+1] = (*(state->clk_timer_64hz_timestamp) >> 8) & 0xFF;
	save[num+2] = (*(state->clk_timer_64hz_timestamp) >> 16) & 0xFF;
	save[num+3] = (*(state->clk_timer_64hz_timestamp) >> 24) & 0xFF;
	num += 4;

	save[num] = *(state->clk_timer_128hz_timestamp) & 0xFF;
	save[num+1] = (*(state->clk_timer_128hz_timestamp) >> 8) & 0xFF;
	save[num+2] = (*(state->clk_timer_128hz_timestamp) >> 16) & 0xFF;
	save[num+3] = (*(state->clk_timer_128hz_timestamp) >> 24) & 0xFF;
	num += 4;

	save[num] = *(state->clk_timer_256hz_timestamp) & 0xFF;
	save[num+1] = (*(state->clk_timer_256hz_timestamp) >> 8) & 0xFF;
	save[num+2] = (*(state->clk_timer_256hz_timestamp) >> 16) & 0xFF;
	save[num+3] = (*(state->clk_timer_256hz_timestamp) >> 24) & 0xFF;
	num += 4;

	save[num] = *(state->prog_timer_timestamp) & 0xFF;
	save[num+1] = (*(state->prog_timer_timestamp) >> 8) & 0xFF;
	save[num+2] = (*(state->prog_timer_timestamp) >> 16) & 0xFF;
	save[num+3] = (*(state->prog_timer_timestamp) >> 24) & 0xFF;
	num += 4;

	save[num] = *(state->prog_timer_enabled) & 0x1;
	num += 1;

	save[num] = *(state->prog_timer_data) & 0xFF;
	num += 1;

	save[num] = *(state->prog_timer_rld) & 0xFF;
	num += 1;

	save[num] = *(state->call_depth) & 0xFF;
	save[num+1] = (*(state->call_depth) >> 8) & 0xFF;
	save[num+2] = (*(state->call_depth) >> 16) & 0xFF;
	save[num+3] = (*(state->call_depth) >> 24) & 0xFF;
	num += 4;

	for (i = 0; i < INT_SLOT_NUM; i++) {
		save[num] = state->interrupts[i].factor_flag_reg & 0xF;
		num += 1;

		save[num] = state->interrupts[i].mask_reg & 0xF;
		num += 1;

		save[num] = state->interrupts[i].triggered & 0x1;
		num += 1;
	}

	/* First 640 half bytes correspond to the RAM */
	for (i = 0; i < MEM_RAM_SIZE; i++) {
		save[num] = GET_RAM_MEMORY(state->memory, i + MEM_RAM_ADDR) & 0xF;
		num += 1;
	}

	/* I/Os are from 0xF00 to 0xF7F */
	for (i = 0; i < MEM_IO_SIZE; i++) {
		save[num] = GET_IO_MEMORY(state->memory, i + MEM_IO_ADDR) & 0xF;
		num += 1;
	}

	*out_len = save_size;
	return save;
}

void state_load(uint8_t *save)
{
	state_t *state;
	uint32_t num = 0;
	uint32_t i;

	state = tamalib_get_state();

	/* First the magic, then the version, and finally the fields of
	 * the state_t struct written as u8, u16 little-endian or u32
	 * little-endian following the struct order
	 */
	if (save[0] != (uint8_t) STATE_FILE_MAGIC[0] || save[1] != (uint8_t) STATE_FILE_MAGIC[1] ||
		save[2] != (uint8_t) STATE_FILE_MAGIC[2] || save[3] != (uint8_t) STATE_FILE_MAGIC[3]) {
		fprintf(stderr, "FATAL: Wrong state save magic!\n");
		return;
	}
	num += 4;

	if (save[num] != STATE_FILE_VERSION) {
		fprintf(stderr, "FATAL: Unsupported version %u (expected %u) in state save!\n", save[num], STATE_FILE_VERSION);
		/* TODO: Handle migration at a point */
		return;
	}
	num += 1;

	*(state->pc) = save[num] | ((save[num+1] & 0x1F) << 8);
	num += 2;

	*(state->x) = save[num] | ((save[num+1] & 0xF) << 8);
	num += 2;

	*(state->y) = save[num] | ((save[num+1] & 0xF) << 8);
	num += 2;

	*(state->a) = save[num] & 0xF;
	num += 1;

	*(state->b) = save[num] & 0xF;
	num += 1;

	*(state->np) = save[num] & 0x1F;
	num += 1;

	*(state->sp) = save[num];
	num += 1;

	*(state->flags) = save[num] & 0xF;
	num += 1;

	*(state->tick_counter) = save[num] | (save[num+1] << 8) | (save[num+2] << 16) | (save[num+3] << 24);
	num += 4;

	*(state->clk_timer_2hz_timestamp) = save[num] | (save[num+1] << 8) | (save[num+2] << 16) | (save[num+3] << 24);
	num += 4;

	*(state->clk_timer_4hz_timestamp) = save[num] | (save[num+1] << 8) | (save[num+2] << 16) | (save[num+3] << 24);
	num += 4;

	*(state->clk_timer_8hz_timestamp) = save[num] | (save[num+1] << 8) | (save[num+2] << 16) | (save[num+3] << 24);
	num += 4;

	*(state->clk_timer_16hz_timestamp) = save[num] | (save[num+1] << 8) | (save[num+2] << 16) | (save[num+3] << 24);
	num += 4;

	*(state->clk_timer_32hz_timestamp) = save[num] | (save[num+1] << 8) | (save[num+2] << 16) | (save[num+3] << 24);
	num += 4;

	*(state->clk_timer_64hz_timestamp) = save[num] | (save[num+1] << 8) | (save[num+2] << 16) | (save[num+3] << 24);
	num += 4;

	*(state->clk_timer_128hz_timestamp) = save[num] | (save[num+1] << 8) | (save[num+2] << 16) | (save[num+3] << 24);
	num += 4;

	*(state->clk_timer_256hz_timestamp) = save[num] | (save[num+1] << 8) | (save[num+2] << 16) | (save[num+3] << 24);
	num += 4;

	*(state->prog_timer_timestamp) = save[num] | (save[num+1] << 8) | (save[num+2] << 16) | (save[num+3] << 24);
	num += 4;

	*(state->prog_timer_enabled) = save[num] & 0x1;
	num += 1;

	*(state->prog_timer_data) = save[num];
	num += 1;

	*(state->prog_timer_rld) = save[num];
	num += 1;

	*(state->call_depth) = save[num] | (save[num+1] << 8) | (save[num+2] << 16) | (save[num+3] << 24);
	num += 4;

	for (i = 0; i < INT_SLOT_NUM; i++) {
		state->interrupts[i].factor_flag_reg = save[num] & 0xF;
		num += 1;

		state->interrupts[i].mask_reg = save[num] & 0xF;
		num += 1;

		state->interrupts[i].triggered = save[num] & 0x1;
		num += 1;
	}

	/* First 640 half bytes correspond to the RAM */
	for (i = 0; i < MEM_RAM_SIZE; i++) {
		SET_RAM_MEMORY(state->memory, i + MEM_RAM_ADDR, save[num] & 0xF);
		num += 1;
	}

	/* I/Os are from 0xF00 to 0xF7F */
	for (i = 0; i < MEM_IO_SIZE; i++) {
		SET_IO_MEMORY(state->memory, i + MEM_IO_ADDR, save[num] & 0xF);
		num += 1;
	}

	if (num != (63 + INT_SLOT_NUM * 3 + MEM_RAM_SIZE + MEM_IO_SIZE)) {
		fprintf(stderr, "FATAL: Failed to load state save!\n");
	}

	tamalib_refresh_hw();
}
