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
#include <string.h>

#include <base64.h>

#include "program.h"

u12_t * program_load_b64(char *rom_b64, uint32_t *size)
{
	uint32_t i;
	size_t rom_len;
	uint8_t *rom;
	u12_t *program;

	if (rom_b64 == NULL) {
		fprintf(stderr, "FATAL: Cannot load ROM!\n");
		return NULL;
	}

	rom = base64_decode(
		(unsigned char *) rom_b64,
		strlen(rom_b64),
		&rom_len);

	*size = rom_len / 2;

	program = (u12_t *) malloc(*size * sizeof(u12_t));
	if (program == NULL) {
		fprintf(stderr, "FATAL: Cannot allocate ROM memory!\n");
		return NULL;
	}

	for (i = 0; i < *size; i++) {
		program[i] = rom[2*i+1] | ((rom[2*i] & 0xF) << 8);
	}

	return program;
}
