/*
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

#include "SDL2/SDL.h"

#include "program.h"

typedef struct map {
	uint32_t ref;
	uint32_t width;
	uint32_t height;
} map_t;

static map_t g_map[MAX_SPRITES];


u12_t * program_load(char *path, uint32_t *size)
{
	SDL_RWops *f;
	uint32_t i;
	uint8_t buf[2];
	u12_t *program;

	f = SDL_RWFromFile(path, "r");
	if (f == NULL) {
		fprintf(stderr, "FATAL: Cannot open ROM \"%s\" !\n", path);
		return NULL;
	}

	*size = SDL_RWsize(f)/2;

	//fprintf(stdout, "ROM size is %u * 12bits\n", *size);

	program = (u12_t *) SDL_malloc(*size * sizeof(u12_t));
	if (program == NULL) {
		fprintf(stderr, "FATAL: Cannot allocate ROM memory !\n");
		SDL_RWclose(f);
		return NULL;
	}

	for (i = 0; i < *size; i++) {
		if (SDL_RWread(f, buf, 2, 1) != 1) {
			fprintf(stderr, "FATAL: Cannot read program from ROM !\n");
			SDL_free(program);
			SDL_RWclose(f);
			return NULL;
		}

		program[i] = buf[1] | ((buf[0] & 0xF) << 8);
	}

	SDL_RWclose(f);
	return program;
}

void program_save(char *path, u12_t *program, uint32_t size)
{
	SDL_RWops *f;
	uint32_t i;
	uint8_t buf[2];

	f = SDL_RWFromFile(path, "w");
	if (f == NULL) {
		fprintf(stderr, "FATAL: Cannot open ROM \"%s\" !\n", path);
		return;
	}

	for (i = 0; i < size; i++) {
		buf[0] = (program[i] >> 8) & 0xF;
		buf[1] = program[i] & 0xFF;

		if (SDL_RWwrite(f, buf, 2, 1) != 1) {
			fprintf(stderr, "FATAL: Cannot write program from ROM !\n");
			SDL_RWclose(f);
			return;
		}
	}

	SDL_RWclose(f);
}

void program_to_header(u12_t *program, uint32_t size)
{
	uint32_t i;

	fprintf(stdout, "static const u12_t g_program[] = {");

	for (i = 0; i < size; i++) {
		if (!(i % 16)) {
			fprintf(stdout, "\n\t");
		} else {
			fprintf(stdout, " ");
		}

		fprintf(stdout, "0x%03X,", program[i]);
	}

	fprintf(stdout, "\n};\n");
}

static uint32_t generate_data_map(map_t *map, u12_t *program, uint32_t size, uint32_t *max_width)
{
	uint32_t i;
	uint32_t count = 0;
	uint32_t width = 0;

	*max_width = 0;

	/* Parse the program to get a map */
	for (i = 0; i < size; i++) {
		if ((program[i] >> 8) == 0x9) {
			/* LBPX */
			if (width == 0) {
			}

			width++;
		} else {
			/* RETD */
			if ((program[i] >> 8) == 0x1 && width != 0) {
				map[count].ref = i - width;
				map[count].width = width + 1;
				map[count].height = 8;

				if (map[count].width > *max_width) {
					*max_width = map[count].width;
				}

				count++;
			}

			width = 0;
		}
	}

	return count;
}
