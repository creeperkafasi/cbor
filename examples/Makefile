CBOR_LIB=../lib

CC=gcc

CFILES+=$(CBOR_LIB)/cbor.c $(CBOR_LIB)/debug.c

INCLUDES+=-I$(CBOR_LIB)

CFLAGS += -ggdb -Wall -Werror -fsanitize=address
CFLAGS += -Wextra -Wpedantic -Wshadow -Wstrict-overflow=2 -fno-strict-aliasing
CFLAGS += -Wnull-dereference -Wdouble-promotion -Wformat=2
CFLAGS += -fsanitize=undefined -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow
CFLAGS += -fstack-protector-strong -fstack-clash-protection

parse-identify:
	$(CC) $(CFILES) $(CFLAGS) $(INCLUDES) parse-identify.c -o parse-identify.out

