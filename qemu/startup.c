#include <stdint.h>

// Add semihosting prototypes
void debug_printf(const char *msg);
void debug_putchar(char c);
void debug_exit(int status);

// Cortex-M3 core registers
#define SCB_VTOR (*(volatile uint32_t*)0xE000ED08)

// Reset handler - your main entry point
void Reset_Handler(void);
void main(void);

// External symbols defined in linker script
extern uint32_t _sidata;  // Start of initialized data in FLASH
extern uint32_t _sdata;   // Start of data section in RAM
extern uint32_t _edata;   // End of data section in RAM
extern uint32_t _sbss;    // Start of bss section in RAM
extern uint32_t _ebss;    // End of bss section in RAM

// Minimal vector table
__attribute__((section(".isr_vector")))
void (* const g_pfnVectors[])(void) = {
    (void (*)(void))((uint32_t)0x20000000 + 64 * 1024), // Stack pointer
    Reset_Handler,                                      // Reset handler
};

// Reset handler
void Reset_Handler(void) {
    debug_printf("=== Starting Reset_Handler ===\r\n");
    
    // Initialize .data section
    uint32_t *pSrc = &_sidata;
    uint32_t *pDest = &_sdata;
    debug_printf("Initializing .data section\r\n");
    while (pDest < &_edata) {
        *pDest++ = *pSrc++;
    }
    
    // Zero .bss section
    debug_printf("Zeroing .bss section\r\n");
    pDest = &_sbss;
    while (pDest < &_ebss) {
        *pDest++ = 0;
    }
    
    // Set vector table offset
    SCB_VTOR = (uint32_t)g_pfnVectors;
    debug_printf("Vector table set\r\n");
    
    // Call main
    debug_printf("Calling main()...\r\n");
    main();
    
    // If main returns
    debug_printf("main() returned! Exiting.\r\n");
    debug_exit(0);
    
    while (1);
}

// Provide minimal _exit for nosys
void _exit(__attribute__((unused)) int status) {
    while (1);
}

// Minimal _sbrk for heap (if needed)
void *_sbrk(int incr) {
    extern char end; // Defined in linker script
    static char *heap_end = 0;
    char *prev_heap_end;

    if (heap_end == 0) {
        heap_end = &end;
    }
    prev_heap_end = heap_end;
    heap_end += incr;
    return (void*)prev_heap_end;
}