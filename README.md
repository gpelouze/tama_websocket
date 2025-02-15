# Tama Websocket - Tamagotchi P1 emulator websocket server

Tama Websocket is a Tamagotchi P1 emulator with a websocket interface. It leverages [TamaLib](https://github.com/jcrona/tamalib/) and [wsServer](https://github.com/Theldus/wsServer).

## Building

1. Add [libSDL2](https://wiki.libsdl.org/SDL2/Installation) and [libcJSON](https://github.com/DaveGamble/cJSON) to your system.

2. Clone and build this project:

```shell
git clone --recursive https://github.com/gpelouze/tama_websocket.git
cd tama_websocket
cmake . && make
```

## Usage

Tama Websocket requires a Tamagotchi P1 ROM named `rom.bin` to be present in the same folder as the executable. Refer to the [TamaTool documentation](https://github.com/jcrona/tamatool/blob/master/README.md#usage) for more information.

To start Tama Websocket, run:

```shell
./tama_websocket
```

The server can be accessed at <ws://localhost:8080>.

## Docker

Build

```shell
docker build . -t tama_websocket
```

Run

```shell
docker run \
  --rm --init \
  -v ./rom.bin:/app/rom.bin \
  -p "127.0.0.1:8080:8080"
  tama_websocket
```

The server can be accessed at <ws://localhost:8080>.

## Websocket API

JSON-encoded events are sent and received through the websocket. Events have exactly two attributes

- `t` (string): the type of the event
- `e` (object): the event payload

Events summary:

| Event type | Sender | Description         |
|------------|--------|---------------------|
| `scr`      | server | screen update       |
| `frq`      | server | frequency playback  |
| `log`      | server | log message         |
| `end`      | server | emulation end       |
| `btn`      | client | button press        |
| `mod`      | client | execution mode      |
| `spd`      | client | execution speed     |

### Server events

#### `scr` - screen update

Attributes:

- `m` (string): base64-encoded screen matrix (32×16 pixels, represented as a 512-bit-long list),
- `i` (string): base64-encoded icons list (8 pixels, represented as a 8-bit-long list).

Example:

```json
{
  "t": "scr",
  "e": {
    "m": "AEEMAAEqUgABIlJ3ASJSVQEiUlUBKlJVAEEMdwAAAAAPAAAAEQAZXB8AJQgRACEIAAAZSBsABQgVACUIFQAZSA==",
    "i": "AA=="
  }
}
```

Here are example functions for decoding the base-64 encoded arrays, in Python:

```Python
import base64
import numpy as np

def b64_to_bits_arr(s):
    decoded = base64.decodebytes(s.encode())
    arr = np.array(bytearray(decoded))
    return np.unpackbits(arr)

b64_to_bits_arr("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA8EAAAWkIAAH4YAABmpAAAZiUAADwYAAAAQgAAAAgA==")
```

and in JavaScript:

```javascript
function b64ToBitsArr(s) {
    const byteArr  = Uint8Array.fromBase64(s);
    const bitArr = new Uint8Array(byteArr.length * 8);
    for (let i = 0; i < s.length; i++) {
        for (let j = 0; j < 8; j++) {
            bitArr[i*8+(7-j)] = (byteArr[i] >> j) & 0x1;
        }
    }
    return bitArr;
}

b64ToBitsArr("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA8EAAAWkIAAH4YAABmpAAAZiUAADwYAAAAQgAAAAgA==")
```

#### `frq` - frequency playback

Attributes:

- `f` (number): frequency in dHz
- `p` (number): sin position
- `e` (0 or 1): enabled

Example:

```json
{
  "t": "frq",
  "e": {
    "f": 23406,
    "p": 0,
    "e": 1
  }
}
```

#### `log` - log message

Attributes:

- `l` (number): level
- `m` (string): message

Example:

```json
{
  "t": "log",
  "e": {
    "l": 0,
    "m": "a dummy message"
  }
}
```

#### `end` - emulation end

Attributes: none

Example:

```json
{
  "t": "end",
  "e": {}
}
```

### Client event

#### `btn` - button press

Attributes:

- `b` (0, 1, 2 or 3): button code
  - 0: left
  - 1: middle
  - 2: right
  - 3: tap
- `s` (0 or 1): button state
  - 0: released
  - 1: pressed

Example:
```json
{
  "t": "btn",
  "e": {
    "b": 1,
    "s": 1
  }
}
```

#### `mod` - execution mode

Attributes:

- `m`: mode
  - 0: pause
  - 1: run / resume
  - 2: enter step mode
  - 3: execute next instruction or call
  - 4: pause after next call 
  - 5: pause after next return

#### `spd` - execution speed

Attributes:

- `s`: speed
  - 0: unlimited
  - 1: 1x
  - 10: 10x

## License

Tama Websocket - Tamagotchi P1 emulator websocket server

Copyright (C) 2025 Gabriel Pelouze

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
