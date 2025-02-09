FROM alpine:latest AS build

WORKDIR /build

RUN apk add --no-cache cmake make clang sdl2-dev cjson cjson-dev

COPY CMakeLists.txt .
COPY src/ src/

RUN cmake .
RUN make


FROM alpine:latest AS runtime

LABEL org.opencontainers.image.authors="Gabriel Pelouze <gabriel@pelouze.net>"
LABEL org.opencontainers.image.url="https://github.com/gpelouze/tama_websocket"
LABEL org.opencontainers.image.documentation="https://github.com/gpelouze/tama_websocket"
LABEL org.opencontainers.image.source="https://github.com/gpelouze/tama_websocket"
LABEL org.opencontainers.image.licenses="GPL-3.0-or-later"
LABEL org.opencontainers.image.title="Tama Websocket"
LABEL org.opencontainers.image.description="Tamagotchi P1 emulator websocket server"

EXPOSE 8080
ENV TAMA_WS_HOST=0.0.0.0

RUN apk add --no-cache sdl2 cjson
USER 1000

WORKDIR /app

COPY --from=build /build/tama_websocket .

ENTRYPOINT ["./tama_websocket"]