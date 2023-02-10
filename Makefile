CFLAGS = -Wall -Wextra -pedantic -lm

CFILES = $(shell find src/ -name "*.c")
CC = gcc

bin/ircy: $(CFILES)
	mkdir -p $(shell dirname $@)
	$(CC) $(CFLAGS) $^ -o $@
