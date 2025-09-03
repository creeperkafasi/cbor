#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>

// Semihosting commands
#define SYS_WRITE0  0x04  // Write null-terminated string
#define SYS_WRITEC  0x03  // Write single character
#define SYS_EXIT    0x18  // Exit

// Semihosting call
static int __semihost(int reason, const void *arg) {
    int result;
    asm volatile (
        "mov r0, %1\n"
        "mov r1, %2\n"
        "bkpt #0xAB\n"
        "mov %0, r0"
        : "=r" (result)
        : "r" (reason), "r" (arg)
        : "r0", "r1", "memory"
    );
    return result;
}

void debug_printf(const char *msg) {
    __semihost(SYS_WRITE0, msg);
}

void debug_putchar(char c) {
    __semihost(SYS_WRITEC, (const void*)(uintptr_t)c);
}

void debug_exit(int status) {
    __semihost(SYS_EXIT, (const void*)(uintptr_t)((status << 16) | 0x20026));
}

// Override standard printf to use semihosting
int printf(const char *format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    
    // Use vsnprintf to format the string
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    // Output via semihosting
    if (len > 0) {
        debug_printf(buffer);
    }
    
    return len;
}

// Override putchar too
int putchar(int c) {
    char ch = (char)c;
    debug_putchar(ch);
    return c;
}
