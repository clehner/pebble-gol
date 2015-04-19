NAME = pebble-gol

BIN = build/$(NAME).pbw

all: install

install: build
	pebble install

build: $(BIN)

$(BIN): src/pebble-gol.c
	pebble build
