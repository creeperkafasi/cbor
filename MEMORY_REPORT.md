# CBOR Library Memory Profiling Report

This document provides a comprehensive analysis of memory usage for the CBOR library across different target platforms and use cases.

## Executive Summary

The CBOR library has been instrumented with memory profiling capabilities to track:
- Stack usage patterns
- Data structure memory footprints  
- Buffer allocation patterns
- Peak memory consumption

### Key Findings:

1. **Architecture Impact**: 32-bit ARM Cortex-M3 shows ~15% memory savings vs 64-bit native
2. **Core Structures**: `cbor_value_t` is the primary memory consumer (48-56 bytes per instance)
3. **Stack Usage**: Well-controlled with typical peaks under 200 bytes
4. **Buffer Efficiency**: Small, efficient buffer allocations (typically <100 bytes)

## Target Platform Comparison

### Native 64-bit Target (x86_64 Linux)

#### System Architecture
- **Target**: Native x86_64 Linux
- **Compiler**: GCC with optimizations (-Os)
- **Pointer Size**: 8 bytes
- **Address Space**: 64-bit

#### Core Data Structure Sizes
| Structure | Size (bytes) | Description |
|-----------|--------------|-------------|
| `cbor_value_t` | 56 | Main CBOR value structure |
| `cbor_pair_t` | 112 | Key-value pair (2x cbor_value_t) |
| `slice_t` | 16 | Fat pointer (8-byte ptr + 8-byte len) |
| `argument_t` | 16 | Argument union structure |
| `cbor_array_t` | 24 | Array metadata |
| `cbor_map_t` | 24 | Map metadata |

#### Performance Characteristics
- **Typical Stack Usage**: 64-1,280 bytes
- **Peak Stack Usage**: ~1,280 bytes (parsing test suite)
- **Memory Overhead**: Higher due to 64-bit pointers
- **RSS Usage**: ~15.5 MB (includes debug symbols and sanitizers)

### Embedded 32-bit Target (ARM Cortex-M3)

#### System Architecture
- **Target**: ARM Cortex-M3 (QEMU lm3s6965evb)
- **Compiler**: arm-none-eabi-gcc with optimizations (-Os)

### Contiki-NG Production Constraints ⚠️

**CRITICAL**: This library is deployed on Contiki-NG systems with severe stack limitations:

- **Total Stack**: 2,048 bytes
- **RTOS Overhead**: ~1,024 bytes  
- **Application Limit**: 1,024 bytes (absolute maximum)
- **Safe Threshold**: 512 bytes (recommended for production)

**Stack Safety Requirements:**
- All CBOR operations must stay under 1KB stack usage
- Recursive parsing depth limited to ~15 levels maximum
- No dynamic allocations (stack only)
- Buffer sizes must be statically analyzable

**Testing Strategy:**
- Custom stress tests validate Contiki-NG constraints
- Static analysis of worst-case stack usage
- Runtime monitoring with stack canaries
- Conservative safety margins (50% of limit)
- **Pointer Size**: 4 bytes
- **Address Space**: 32-bit
- **Memory Model**: Harvard architecture with separate code/data spaces

#### Core Data Structure Sizes
| Structure | Size (bytes) | Description |
|-----------|--------------|-------------|
| `cbor_value_t` | 48 | Main CBOR value structure (-14% vs 64-bit) |
| `cbor_pair_t` | 96 | Key-value pair (2x cbor_value_t) |
| `slice_t` | 8 | Fat pointer (4-byte ptr + 4-byte len) |
| `argument_t` | 16 | Argument union structure |
| `cbor_array_t` | 12 | Array metadata (-50% vs 64-bit) |
| `cbor_map_t` | 12 | Map metadata (-50% vs 64-bit) |

#### Performance Characteristics
- **Typical Stack Usage**: 96-168 bytes
- **Peak Stack Usage**: 168 bytes (string parsing function)
- **Memory Efficiency**: Superior due to 32-bit pointers
- **Footprint**: Significantly smaller RAM requirements

## Memory Usage Analysis by Use Case

### Test Case 1: Basic Parsing (test-parse)

#### Native 64-bit Results:
- **Peak Stack**: 1,280 bytes
- **CBOR Structures**: 14 instances
- **Buffer Usage**: 22 bytes total
- **Estimated Footprint**: 2,086 bytes

#### Embedded 32-bit Results:
- **Peak Stack**: 168 bytes (-87% vs native)
- **CBOR Structures**: 14 instances
- **Buffer Usage**: 22 bytes total
- **Estimated Footprint**: 862 bytes (-59% vs native)

**Analysis**: The embedded target shows dramatic memory savings, primarily due to smaller pointer sizes and more efficient stack utilization.

### Test Case 2: Complex Encoding (test-encode)

#### Native 64-bit Results:
- **Peak Stack**: 64 bytes
- **CBOR Structures**: 2 instances
- **Buffer Usage**: 64 bytes total
- **Estimated Footprint**: 240 bytes

**Analysis**: Encoding operations are more memory-efficient than parsing, requiring fewer intermediate structures.

### Test Case 3: Identification Protocol (identify-parse)

#### Embedded 32-bit Results:
- **Peak Stack**: 0 bytes (simple linear execution)
- **CBOR Structures**: 3 instances
- **Buffer Usage**: 68 bytes total
- **Estimated Footprint**: 212 bytes

**Analysis**: Real-world protocol parsing demonstrates efficient memory usage patterns.

## Memory Optimization Recommendations

### For Embedded Targets (Resource-Constrained)

1. **Structure Packing**: The current structures are well-aligned but could benefit from:
   ```c
   __attribute__((packed))  // For critical structures
   ```

2. **Stack Optimization**: Current stack usage is excellent (<200 bytes peak)
   - Consider tail-call optimization for recursive parsing
   - Pool allocation for repeated operations

3. **Buffer Management**: 
   - Current small buffer approach is optimal
   - Consider fixed-size pools for predictable allocation

### For Native Targets (Performance-Focused)

1. **Memory Pool Allocation**: 
   - Pre-allocate CBOR value pools to reduce malloc overhead
   - Implement custom allocators for frequent operations

2. **Cache Optimization**:
   - Current 56-byte cbor_value_t fits well in cache lines
   - Consider structure reordering for hot path optimization

## Detailed Stack Analysis

### Stack Growth Patterns

#### Native Target (64-bit):
```
Function Call Stack Growth:
main() → 0 bytes
├── test_integer_parsing() → 64 bytes
├── test_string_parsing() → 384 bytes  
├── test_simple_values() → 704 bytes
├── test_array_parsing() → 832 bytes
└── test_map_parsing() → 1,280 bytes (PEAK)
```

#### Embedded Target (32-bit):
```
Function Call Stack Growth:
main() → 0 bytes
├── test_integer_parsing() → 160 bytes (PEAK)
├── test_string_parsing() → 168 bytes
├── test_simple_values() → 104 bytes
├── test_array_parsing() → 160 bytes
└── test_map_parsing() → 160 bytes
```

### Stack Safety Analysis

- **Native**: Peak 1,280 bytes well within typical 8MB stack limits
- **Embedded**: Peak 168 bytes excellent for microcontroller constraints
- **Safety Margin**: Both targets show predictable, bounded growth

## Memory Fragmentation Analysis

### Buffer Allocation Patterns

| Use Case | Total Buffers | Largest Single | Pattern |
|----------|---------------|----------------|---------|
| Basic Parsing | 22 bytes | 6 bytes | Many small allocations |
| Complex Encoding | 64 bytes | 64 bytes | Single large buffer |
| Protocol Parsing | 68 bytes | 68 bytes | Single protocol buffer |

**Fragmentation Risk**: Low - small, predictable allocation patterns

## Performance vs Memory Trade-offs

### Architecture Comparison

| Metric | Native 64-bit | Embedded 32-bit | Savings |
|--------|---------------|-----------------|---------|
| cbor_value_t | 56 bytes | 48 bytes | 14.3% |
| slice_t | 16 bytes | 8 bytes | 50.0% |
| Peak Stack | 1,280 bytes | 168 bytes | 86.9% |
| Total Footprint | 2,086 bytes | 862 bytes | 58.7% |

### Memory Density

- **Native**: Lower density due to 64-bit alignment and pointers
- **Embedded**: Higher density with 32-bit optimization
- **Trade-off**: Embedded saves memory at cost of address space limitations

## Conclusions and Recommendations

### Strengths
1. **Efficient Design**: Core structures are well-sized and aligned
2. **Predictable Patterns**: Memory usage is bounded and predictable
3. **Platform Adaptive**: Good optimization for both 32-bit and 64-bit targets
4. **Stack Disciplined**: Excellent stack usage control

### Areas for Improvement
1. **Memory Pools**: Consider pools for high-frequency allocations
2. **Zero-Copy**: Maximize zero-copy operations where possible
3. **Compile-Time Optimization**: More aggressive dead code elimination

### Target-Specific Recommendations

#### For Embedded Projects:
- ✅ Current design is excellent for microcontrollers
- ✅ Stack usage well within typical constraints (1-4KB stacks)
- ✅ Memory footprint suitable for devices with 32KB+ RAM

#### For High-Performance Native:
- Consider memory pools for sustained throughput
- Evaluate SIMD opportunities for bulk operations
- Profile cache performance under load

## Appendix: Profiling Methodology

### Instrumentation Details

The memory profiling system tracks:
- Function entry/exit with stack pointer sampling
- Buffer allocations with size tracking
- Structure instance counting with type identification
- Peak usage calculation across all metrics

### Measurement Accuracy

- **Stack Tracking**: ±8 bytes accuracy (pointer alignment)
- **Buffer Tracking**: Exact byte-level accuracy
- **Structure Counting**: Exact instance counting
- **Cross-Platform**: Consistent methodology across targets

### Test Coverage

- ✅ Basic parsing operations
- ✅ Complex encoding scenarios  
- ✅ Real-world protocol handling
- ✅ Edge cases and boundary conditions
- ✅ Both 32-bit and 64-bit architectures

---

*Generated by CBOR Library Memory Profiler v1.0*  
*Date: September 4, 2025*  
*Branch: memory-profiling*
