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

#ifndef BASE64SINGLELINE_H
#define BASE64SINGLELINE_H

unsigned char * base64singleline_encode(const unsigned char *src, size_t len,
			      size_t *out_len);

#endif //BASE64SINGLELINE_H
