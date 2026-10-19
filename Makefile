# Compilatore e flag comuni a server e client.
CC := gcc
CFLAGS := -std=c11 -D_POSIX_C_SOURCE=200112L -Wall -Wextra -Wpedantic -O2 -g -I src
LDFLAGS :=

# Directory finali e oggetti intermedi divisi per area logica.
BIN_DIR := bin
COMMON_OBJS := build/common/net.o build/common/protocol.o build/common/utils.o
SERVER_OBJS := build/server/main.o build/server/server.o build/server/game.o build/server/users.o
CLIENT_OBJS := build/client/main.o build/client/client.o build/client/ui.o
ALL_OBJS := $(COMMON_OBJS) $(SERVER_OBJS) $(CLIENT_OBJS)

.PHONY: all server client clean run-server run-client

all: server client

# Target logici: permettono `make server` o `make client`.
server: $(BIN_DIR)/server

client: $(BIN_DIR)/client

# Link dell'eseguibile server: usa codice comune + moduli server.
$(BIN_DIR)/server: $(COMMON_OBJS) $(SERVER_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Link dell'eseguibile client: usa codice comune + moduli client.
$(BIN_DIR)/client: $(COMMON_OBJS) $(CLIENT_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Regola generica di compilazione: src/foo/bar.c -> build/foo/bar.o.
build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

# Crea la directory degli eseguibili se manca.
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Comandi di comodo per demo locale.
run-server: server
	./$(BIN_DIR)/server 4242 300 5

run-client: client
	./$(BIN_DIR)/client 127.0.0.1 4242

# Rimuove prodotti di compilazione, lasciando intatti i sorgenti.
clean:
	rm -rf build $(BIN_DIR)
