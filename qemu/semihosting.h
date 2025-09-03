#ifndef SEMIHOSTING_H
#define SEMIHOSTING_H

#include <stdio.h>

// Function prototypes
void debug_printf(const char *msg);
void debug_putchar(char c);

// Override standard IO functions
// #define printf debug_printf
// #define putchar debug_putchar
// #define puts(s) do { debug_printf(s); debug_printf("\n"); } while (0)

#endif