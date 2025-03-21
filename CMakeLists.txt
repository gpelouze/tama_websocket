# Tama Websocket - Tamagotchi P1 emulator websocket server
#
# Copyright (C) 2025 Gabriel Pelouze <gabriel@pelouze.net>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.31)
project(tama_websocket C)

set(CMAKE_C_STANDARD 11)

find_package(cJSON REQUIRED)

include_directories(src)
include_directories(src/tamalib)
include_directories(src/wsServer/include)
include_directories(src/wsServer/src)

add_executable(tama_websocket
    src/tamalib/cpu.c
    src/tamalib/cpu.h
    src/tamalib/hal.h
    src/tamalib/hw.c
    src/tamalib/hw.h
    src/tamalib/tamalib.c
    src/tamalib/tamalib.h
    src/wsServer/src/ws.c
    src/wsServer/src/base64.c
    src/wsServer/src/sha1.c
    src/wsServer/src/handshake.c
    src/wsServer/src/utf8.c
    src/base64singleline.c
    src/base64singleline.h
    src/hal_types.h
    src/main.c
    src/program.c
    src/program.h
    src/state.c
    src/state.h)

target_link_libraries(tama_websocket cjson)
