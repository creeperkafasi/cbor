#ifndef MEMORY_PROFILER_H
#define MEMORY_PROFILER_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#ifdef TARGET_EMBEDDED
#include "semihosting.h"
// For embedded targets, we'll use simpler memory tracking
#define MEMORY_PRINTF(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
// For native targets, we can use more advanced tracking
#include <sys/resource.h>
#include <unistd.h>
#define MEMORY_PRINTF(fmt, ...) printf(fmt, ##__VA_ARGS__)
#endif

// Memory profiling structure
typedef struct {
    size_t peak_stack_usage;
    size_t current_stack_depth;
    size_t total_cbor_structures;
    size_t total_buffer_bytes;
    size_t max_single_buffer;
    const char* current_function;
} memory_profile_t;

// Global memory profiler instance
extern memory_profile_t g_memory_profile;

// Stack tracking macros
#define MEMORY_PROFILE_FUNCTION_ENTER(func_name) \
    do { \
        g_memory_profile.current_function = func_name; \
        memory_profile_function_enter(func_name); \
    } while(0)

#define MEMORY_PROFILE_FUNCTION_EXIT() \
    do { \
        memory_profile_function_exit(); \
    } while(0)

#define MEMORY_PROFILE_BUFFER(name, size) \
    do { \
        memory_profile_buffer(name, size); \
    } while(0)

#define MEMORY_PROFILE_CBOR_STRUCTURE(type_name, instance_ptr) \
    do { \
        memory_profile_cbor_structure(type_name, instance_ptr); \
    } while(0)

#define MEMORY_PROFILE_REPORT() \
    do { \
        memory_profile_report(); \
    } while(0)

// Function declarations
void memory_profile_init(void);
void memory_profile_function_enter(const char* func_name);
void memory_profile_function_exit(void);
void memory_profile_buffer(const char* buffer_name, size_t size);
void memory_profile_cbor_structure(const char* type_name, const void* instance);
void memory_profile_report(void);
void memory_profile_print_system_info(void);

// Helper function to get approximate stack pointer (rough estimate)
uintptr_t memory_profile_get_stack_pointer(void);

#endif // MEMORY_PROFILER_H
