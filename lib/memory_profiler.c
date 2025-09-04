#include "memory_profiler.h"
#include "cbor.h"
#include <string.h>

// Global memory profiler instance
memory_profile_t g_memory_profile = {0};

// Stack start pointer (approximate)
static uintptr_t stack_start = 0;
static size_t max_stack_depth = 0;

void memory_profile_init(void) {
    memset(&g_memory_profile, 0, sizeof(g_memory_profile));
    stack_start = memory_profile_get_stack_pointer();
    
    MEMORY_PRINTF("\n=== MEMORY PROFILING INITIALIZED ===\n");
    MEMORY_PRINTF("Stack start pointer: 0x%lx\n", (unsigned long)stack_start);
    
    memory_profile_print_system_info();
}

void memory_profile_function_enter(const char* func_name) {
    uintptr_t current_sp = memory_profile_get_stack_pointer();
    size_t current_depth;
    
    // Calculate stack depth more reliably
    if (stack_start == 0) {
        stack_start = current_sp;  // Initialize if not set
        current_depth = 0;
    } else {
        // Stack typically grows downward on most architectures
        if (stack_start >= current_sp) {
            current_depth = stack_start - current_sp;
        } else {
            current_depth = current_sp - stack_start;
        }
    }
    
    g_memory_profile.current_stack_depth = current_depth;
    
    if (current_depth > g_memory_profile.peak_stack_usage) {
        g_memory_profile.peak_stack_usage = current_depth;
    }
    
    if (current_depth > max_stack_depth) {
        max_stack_depth = current_depth;
        MEMORY_PRINTF("📊 NEW PEAK STACK: %s() - %zu bytes\n", func_name, current_depth);
    }
    
    MEMORY_PRINTF("📥 ENTER %s() - Stack: %zu bytes\n", func_name, current_depth);
}

void memory_profile_function_exit(void) {
    uintptr_t current_sp = memory_profile_get_stack_pointer();
    size_t current_depth;
    
    if (stack_start == 0) {
        current_depth = 0;
    } else {
        // Stack typically grows downward on most architectures
        if (stack_start >= current_sp) {
            current_depth = stack_start - current_sp;
        } else {
            current_depth = current_sp - stack_start;
        }
    }
    
    MEMORY_PRINTF("📤 EXIT %s() - Stack: %zu bytes\n", 
                  g_memory_profile.current_function ? g_memory_profile.current_function : "unknown", 
                  current_depth);
}

void memory_profile_buffer(const char* buffer_name, size_t size) {
    g_memory_profile.total_buffer_bytes += size;
    
    if (size > g_memory_profile.max_single_buffer) {
        g_memory_profile.max_single_buffer = size;
    }
    
    MEMORY_PRINTF("🗂️  BUFFER %s: %zu bytes (Total buffers: %zu bytes)\n", 
                  buffer_name, size, g_memory_profile.total_buffer_bytes);
}

void memory_profile_cbor_structure(const char* type_name, const void* instance) {
    g_memory_profile.total_cbor_structures++;
    
    // Calculate size based on type
    size_t structure_size = 0;
    
    if (strstr(type_name, "cbor_value_t")) {
        structure_size = sizeof(cbor_value_t);
        const cbor_value_t* value = (const cbor_value_t*)instance;
        MEMORY_PRINTF("🏗️  CBOR %s: %zu bytes (type=%d, total_structs=%zu)\n", 
                      type_name, structure_size, (int)value->type, g_memory_profile.total_cbor_structures);
    } else if (strstr(type_name, "cbor_pair_t")) {
        structure_size = sizeof(cbor_pair_t);
        MEMORY_PRINTF("🏗️  CBOR %s: %zu bytes (total_structs=%zu)\n", 
                      type_name, structure_size, g_memory_profile.total_cbor_structures);
    } else if (strstr(type_name, "slice_t")) {
        structure_size = sizeof(slice_t);
        const slice_t* slice = (const slice_t*)instance;
        MEMORY_PRINTF("🏗️  CBOR %s: %zu bytes (len=%zu, total_structs=%zu)\n", 
                      type_name, structure_size, slice->len, g_memory_profile.total_cbor_structures);
    } else {
        structure_size = 32; // rough estimate for unknown types
        MEMORY_PRINTF("🏗️  CBOR %s: ~%zu bytes (estimated, total_structs=%zu)\n", 
                      type_name, structure_size, g_memory_profile.total_cbor_structures);
    }
}

void memory_profile_report(void) {
    MEMORY_PRINTF("\n=== MEMORY PROFILING REPORT ===\n");
    MEMORY_PRINTF("📈 Peak stack usage: %zu bytes\n", g_memory_profile.peak_stack_usage);
    MEMORY_PRINTF("📚 Total CBOR structures created: %zu\n", g_memory_profile.total_cbor_structures);
    MEMORY_PRINTF("💾 Total buffer memory used: %zu bytes\n", g_memory_profile.total_buffer_bytes);
    MEMORY_PRINTF("📦 Largest single buffer: %zu bytes\n", g_memory_profile.max_single_buffer);
    
    // Calculate estimated total memory footprint
    size_t estimated_total = g_memory_profile.peak_stack_usage + 
                           g_memory_profile.total_buffer_bytes + 
                           (g_memory_profile.total_cbor_structures * sizeof(cbor_value_t));
    
    MEMORY_PRINTF("🧮 Estimated total memory footprint: %zu bytes\n", estimated_total);
    MEMORY_PRINTF("========================================\n\n");
}

void memory_profile_print_system_info(void) {
    MEMORY_PRINTF("💻 System Information:\n");
    MEMORY_PRINTF("   - sizeof(cbor_value_t): %zu bytes\n", sizeof(cbor_value_t));
    MEMORY_PRINTF("   - sizeof(cbor_pair_t): %zu bytes\n", sizeof(cbor_pair_t));
    MEMORY_PRINTF("   - sizeof(slice_t): %zu bytes\n", sizeof(slice_t));
    MEMORY_PRINTF("   - sizeof(argument_t): %zu bytes\n", sizeof(argument_t));
    MEMORY_PRINTF("   - sizeof(cbor_array_t): %zu bytes\n", sizeof(cbor_array_t));
    MEMORY_PRINTF("   - sizeof(cbor_map_t): %zu bytes\n", sizeof(cbor_map_t));
    
#ifdef TARGET_EMBEDDED
    MEMORY_PRINTF("   - Target: ARM Cortex-M3 (32-bit)\n");
    MEMORY_PRINTF("   - sizeof(void*): %zu bytes\n", sizeof(void*));
    MEMORY_PRINTF("   - sizeof(size_t): %zu bytes\n", sizeof(size_t));
    MEMORY_PRINTF("   - sizeof(uintptr_t): %zu bytes\n", sizeof(uintptr_t));
#else
    MEMORY_PRINTF("   - Target: Native (64-bit)\n");
    MEMORY_PRINTF("   - sizeof(void*): %zu bytes\n", sizeof(void*));
    MEMORY_PRINTF("   - sizeof(size_t): %zu bytes\n", sizeof(size_t));
    MEMORY_PRINTF("   - sizeof(uintptr_t): %zu bytes\n", sizeof(uintptr_t));
    
    // For native target, try to get more system info
    #ifndef __APPLE__ // rusage might not be available on all systems
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        MEMORY_PRINTF("   - Current RSS: %ld KB\n", usage.ru_maxrss);
    }
    #endif
#endif
    MEMORY_PRINTF("\n");
}

// Platform-specific stack pointer retrieval
uintptr_t memory_profile_get_stack_pointer(void) {
    volatile uintptr_t stack_var;
    return (uintptr_t)&stack_var;
}
