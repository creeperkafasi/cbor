CC=gcc

CFILES+=lib/cbor.c lib/debug.c

INCLUDES+=-I./lib

CFLAGS += -ggdb -Wall -Werror -fsanitize=address
CFLAGS += -Wextra -Wpedantic -Wshadow -Wstrict-overflow=2 -fno-strict-aliasing
CFLAGS += -Wnull-dereference -Wdouble-promotion -Wformat=2
CFLAGS += -fsanitize=undefined -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow
CFLAGS += -fstack-protector-strong -fstack-clash-protection

main:
	$(CC) $(CFILES) $(CFLAGS) $(INCLUDES) main.c