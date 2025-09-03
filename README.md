# CBOR

Safe CBOR library written in C

## Features

|  | Parsing | Encoding |
|---------------|---------|----------|
| Integers      |✅Supported|✅Supported|
| Byte Strings  |✅Supported|✅Supported|
| Text Strings  |✅Supported|✅Supported|
| Arrays        |✅Supported|✅Supported|
| Maps          |✅Supported|✅Supported|
| Indefinite Length Arrays |✅Supported|✅Supported|
| Indefinite Length Maps   |✅Supported|✅Supported|
| Indefinite Length Strings|✅Supported|✅Supported|
| Simples       |✅Supported|✅Supported|
| Floats        |✅Supported|✅Supported|
| Tags          |⛔Not Supported|⛔Not Supported|

## Parsing

### Basic Parsing

You need a slice_t to work with most of the functions in the library. You can create one by specifying the length and the pointer. [Fat Pointer - Wikipedia](https://en.wikipedia.org/wiki/Fat_pointer)

```c
    slice_t cbor = {
        .len = sizeof(buf),
        .ptr = buf
    };
```

> This is to make it harder (but not impossible) to pass raw pointers around, which makes buffer overflows less common.

You may then use the `parse()` function which returns a `parse_result_t`. The structure of the parse_result_t is the following:

```c
struct {
    uint8_t is_error;
    union { 
        cbor_value_t ok;
        cbor_parser_error_t err;
    };
} 
```

The parse function can fail, and there can be multiple reasons for it. Thus, you may want to check the `is_error` value and print the `.err` field in case of an error to see if there is an error.

After checking for errors, you can access the `cbor_value_t` from the `.ok` field.

```c
struct {
    cbor_type_t type; 
    argument_t argument;
    union {
        int64_t integer;
        slice_t bytes;
        int64_t length;
        cbor_map_t map;
        cbor_array_t array;
        float floating;
        cbor_simple_t simple;
    } value;
    uint8_t* next;
};
```

The `*next` pointer points to the byte after the end of this value. This is not guaranteed to be a valid pointer. It is NULL in the cases that:

- The value is an array / map.
- The value length is indefinite.

For arrays and maps you can use the `process_array` and the `process_map` functions.

`process_array` takes a `cbor_array_t`, a `*single_processor_function` function pointer, and a `void*` argument which is passed to the processor.

`process_map` takes a `cbor_map_t`, a `*pair_processor_function` function pointer, and a `void*` argument which is passed to the processor.

If the processor function argument is empty, the function will just recurse into the container and return the pointer to the byte after the end of the struct.


### Advanced Parsing: Schema Processing

One way of parsing a pre-defined schema is to create some structs and their coresponding parser functions.

```c
typedef struct {
    slice_t f;
    slice_t sn; 
} device_info_t;

typedef struct {
    device_info_t d;
    int fn;
    slice_t rid;
} identification_request_t;

void process_device_info(const cbor_value_t *key, const cbor_value_t *value, void *arg) {
    device_info_t* device = arg;

    // Slice to cstring boilerplate, needed for strcmp
    char keystr[key->value.bytes.len + 1];
    memcpy(keystr, key->value.bytes.ptr, key->value.bytes.len);
    keystr[key->value.bytes.len] = 0;

    if (!strcmp(keystr, "f")) {
        device->f = value->value.bytes;
    }
    if (!strcmp(keystr, "sn")) {
        device->sn = value->value.bytes;
    }
}

void process_identification_request(const cbor_value_t *key, const cbor_value_t *value, void *arg) {
    identification_request_t* request = arg;

    char keystr[key->value.bytes.len + 1];
    memcpy(keystr, key->value.bytes.ptr, key->value.bytes.len);
    keystr[key->value.bytes.len] = 0;

    if (!strcmp(keystr, "d")) {
        process_map(value->value.map, process_device_info, &request->d);
    }
    if (!strcmp(keystr, "fn")) {
        request->fn = value->value.integer;
    }
    if (!strcmp(keystr, "rid")) {
        request->rid = value->value.bytes;
    }
}
```

You can then pass the parser function into process_map and the fields fill get filled in. You can call other functions, and even do recursion if you need to.

Here I used a zero-copy approach, if the initial buffer gets dropped by a return or a free, the data will be corrupted. If you want the data to persist, you would use fixed size arrays or dynamicly allocated pointers instead of slices and memcpy into these.

This example leaves out some details. What if the key is not a string? What if there is an invalid key? These are left to the user. In many cases, like fault tolerant systems, it is better to ignore these as bad user input.

### Indefinite Length Parsing

The library supports indefinite length arrays, maps, and strings. These are CBOR containers that don't specify their length upfront and are terminated by a "break" stop code (0xFF).

When parsing, indefinite length arrays, maps, and strings are marked with `length = UINT32_MAX`. The processing functions (`cbor_process_array`, `cbor_process_map`, and `cbor_process_indefinite_string`) handle this automatically:

```c
// Parse indefinite length array: [1, 2, 3]
uint8_t cbor_data[] = {0x9F, 0x01, 0x02, 0x03, 0xFF};
slice_t input = {.len = sizeof(cbor_data), .ptr = cbor_data};

cbor_parse_result_t result = cbor_parse(input);
if (!result.is_error && result.ok.type == CBOR_TYPE_ARRAY) {
    // Process elements (automatically handles indefinite length)
    cbor_process_array(result.ok.value.array, your_processor, your_arg);
}
```

```c
// Parse indefinite length text string: "hello" + "world"
uint8_t cbor_data[] = {0x7F, 0x65, 0x68, 0x65, 0x6C, 0x6C, 0x6F, 0x65, 0x77, 0x6F, 0x72, 0x6C, 0x64, 0xFF};
slice_t input = {.len = sizeof(cbor_data), .ptr = cbor_data};

cbor_parse_result_t result = cbor_parse(input);
if (!result.is_error && result.ok.type == CBOR_TYPE_TEXT_STRING && result.ok.argument.tag == ARGUMENT_NONE) {
    // Process string chunks (automatically handles indefinite length)
    cbor_process_indefinite_string(result.ok.value.array, CBOR_TYPE_TEXT_STRING, your_processor, your_arg);
}
```

## Encoding

The library provides comprehensive encoding support for all CBOR types including indefinite length containers.

### Basic Encoding

All encoding operations use the `cbor_encode()` function which takes a `cbor_value_t` and a target buffer slice:

```c
cbor_encode_result_t cbor_encode(cbor_value_t value, slice_t target);
```

The result structure follows the same pattern as parsing:
```c
struct {
    uint8_t is_error;
    union { 
        slice_t ok;          // Points to remaining buffer after encoding
        cbor_encoder_error_t err;
    };
} 
```

### Encoding Integers

```c
// Positive integer
cbor_value_t value = {
    .type = CBOR_TYPE_INTEGER,
    .value.integer = 42
};

uint8_t buffer[64];
slice_t target = {.len = sizeof(buffer), .ptr = buffer};
cbor_encode_result_t result = cbor_encode(value, target);

if (!result.is_error) {
    size_t encoded_length = sizeof(buffer) - result.ok.len;
    // Use buffer[0] to buffer[encoded_length-1]
}
```

```c
// Negative integer  
cbor_value_t value = {
    .type = CBOR_TYPE_INTEGER,
    .value.integer = -100
};
```

### Encoding Strings

```c
// Text string
cbor_value_t text = {
    .type = CBOR_TYPE_TEXT_STRING,
    .value.bytes = STR2SLICE("Hello, World!")
};

// Byte string
uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
cbor_value_t bytes = {
    .type = CBOR_TYPE_BYTE_STRING,
    .value.bytes = {.len = sizeof(data), .ptr = data}
};
```

### Encoding Floats

```c
// Float32
cbor_value_t float_val = {
    .type = CBOR_TYPE_FLOAT,
    .argument.tag = ARGUMENT_FLOAT32,
    .value.floating = 3.14159f
};

// Float64  
cbor_value_t double_val = {
    .type = CBOR_TYPE_FLOAT,
    .argument.tag = ARGUMENT_FLOAT64,
    .value.floating = 2.718281828459045
};
```

### Encoding Simple Values

```c
// Boolean true
cbor_value_t bool_true = {
    .type = CBOR_TYPE_SIMPLE,
    .value.simple = CBOR_SIMPLE_TRUE
};

// Boolean false
cbor_value_t bool_false = {
    .type = CBOR_TYPE_SIMPLE,
    .value.simple = CBOR_SIMPLE_FALSE
};

// Null
cbor_value_t null_val = {
    .type = CBOR_TYPE_SIMPLE,
    .value.simple = CBOR_SIMPLE_NULL
};

// Undefined
cbor_value_t undef_val = {
    .type = CBOR_TYPE_SIMPLE,
    .value.simple = CBOR_SIMPLE_UNDEFINED
};
```

### Encoding Arrays

```c
// Create array elements
cbor_value_t elements[] = {
    {.type = CBOR_TYPE_INTEGER, .value.integer = 1},
    {.type = CBOR_TYPE_INTEGER, .value.integer = 2},
    {.type = CBOR_TYPE_TEXT_STRING, .value.bytes = STR2SLICE("three")}
};

// Create definite length array for encoding
cbor_value_t array = {
    .type = CBOR_ENCODE_TYPE_VALUES,
    .value.values = {
        .len = 3,
        .ptr = elements
    }
};

// Encode the array
uint8_t buffer[64];
slice_t target = {.len = sizeof(buffer), .ptr = buffer};
cbor_encode_result_t result = cbor_encode(array, target);
```

### Encoding Maps

```c
// Create key-value pairs
cbor_value_t key1 = {.type = CBOR_TYPE_TEXT_STRING, .value.bytes = STR2SLICE("name")};
cbor_value_t val1 = {.type = CBOR_TYPE_TEXT_STRING, .value.bytes = STR2SLICE("Alice")};

cbor_value_t key2 = {.type = CBOR_TYPE_TEXT_STRING, .value.bytes = STR2SLICE("age")};
cbor_value_t val2 = {.type = CBOR_TYPE_INTEGER, .value.integer = 30};

cbor_pair_t pairs[] = {
    {.first = key1, .second = val1},
    {.first = key2, .second = val2}
};

// Create definite length map for encoding
cbor_value_t map = {
    .type = CBOR_ENCODE_TYPE_PAIRS,
    .value.pairs = {
        .len = 2,
        .ptr = pairs
    }
};

// Encode the map
uint8_t buffer[64];
slice_t target = {.len = sizeof(buffer), .ptr = buffer};
cbor_encode_result_t result = cbor_encode(map, target);
```

### Encoding Indefinite Length Containers

The library provides convenient macros for creating indefinite length containers:

#### Indefinite Length Arrays

```c
// Create indefinite length array using macro
cbor_value_t elements[] = {
    {.type = CBOR_TYPE_INTEGER, .value.integer = 1},
    {.type = CBOR_TYPE_INTEGER, .value.integer = 2},
    {.type = CBOR_TYPE_INTEGER, .value.integer = 3}
};
cbor_value_t indefinite_array = CBOR_INDEFINITE_ARRAY(elements);
// This creates: {.type = CBOR_ENCODE_TYPE_VALUES_INDEFINITE, .value.values = {.len = 3, .ptr = elements}}

// Encode - produces [_ 1, 2, 3] which encodes as 0x9F 0x01 0x02 0x03 0xFF
uint8_t buffer[64];
slice_t target = {.len = sizeof(buffer), .ptr = buffer};
cbor_encode_result_t result = cbor_encode(indefinite_array, target);
```

#### Indefinite Length Maps

```c
// Create indefinite length map using macro
cbor_value_t key1 = {.type = CBOR_TYPE_TEXT_STRING, .value.bytes = STR2SLICE("a")};
cbor_value_t val1 = {.type = CBOR_TYPE_INTEGER, .value.integer = 1};
cbor_value_t key2 = {.type = CBOR_TYPE_TEXT_STRING, .value.bytes = STR2SLICE("b")};
cbor_value_t val2 = {.type = CBOR_TYPE_INTEGER, .value.integer = 2};

cbor_pair_t pairs[] = {
    {.first = key1, .second = val1},
    {.first = key2, .second = val2}
};
cbor_value_t indefinite_map = CBOR_INDEFINITE_MAP(pairs);
// This creates: {.type = CBOR_ENCODE_TYPE_PAIRS_INDEFINITE, .value.pairs = {.len = 2, .ptr = pairs}}

// Encode - produces {_ "a": 1, "b": 2} which encodes as 0xBF 0x61 0x61 0x01 0x61 0x62 0x02 0xFF
cbor_encode_result_t result = cbor_encode(indefinite_map, target);
```

#### Indefinite Length Strings

```c
// Create indefinite length text string from chunks
cbor_value_t chunks[] = {
    {.type = CBOR_TYPE_TEXT_STRING, .value.bytes = STR2SLICE("hello")},
    {.type = CBOR_TYPE_TEXT_STRING, .value.bytes = STR2SLICE("world")}
};
cbor_value_t indefinite_string = CBOR_INDEFINITE_TEXT_STRING(chunks);
// This creates: {.type = CBOR_ENCODE_TYPE_TEXT_STRING_INDEFINITE, .value.values = {.len = 2, .ptr = chunks}}

// Encode - produces (_ "hello", "world") which encodes as 0x7F 0x65... 0x65... 0xFF
cbor_encode_result_t result = cbor_encode(indefinite_string, target);
```

```c
// Create indefinite length byte string from chunks
uint8_t chunk1[] = {0x01, 0x02};
uint8_t chunk2[] = {0x03, 0x04};
cbor_value_t byte_chunks[] = {
    {.type = CBOR_TYPE_BYTE_STRING, .value.bytes = {.len = sizeof(chunk1), .ptr = chunk1}},
    {.type = CBOR_TYPE_BYTE_STRING, .value.bytes = {.len = sizeof(chunk2), .ptr = chunk2}}
};
cbor_value_t indefinite_bytes = CBOR_INDEFINITE_BYTE_STRING(byte_chunks);
// This creates: {.type = CBOR_ENCODE_TYPE_BYTE_STRING_INDEFINITE, .value.values = {.len = 2, .ptr = byte_chunks}}

// Encode - produces (_ h'0102', h'0304') which encodes as 0x5F 0x42 0x01 0x02 0x42 0x03 0x04 0xFF
cbor_encode_result_t result = cbor_encode(indefinite_bytes, target);
```

**Important**: Indefinite length strings are composed of chunks of definite length strings of the same type (all text strings or all byte strings). Mixed types within an indefinite string are not allowed per CBOR specification.

### Error Handling

Always check the encoding result for errors:

```c
cbor_encode_result_t result = cbor_encode(value, target);
if (result.is_error) {
    switch (result.err) {
        case CBOR_ENCODER_ERROR_BUFFER_TOO_SMALL:
            printf("Buffer too small for encoding\n");
            break;
        case CBOR_ENCODER_ERROR_INVALID_TYPE:
            printf("Invalid CBOR type\n");
            break;
        // Handle other error cases...
    }
} else {
    size_t encoded_length = target.len - result.ok.len;
    printf("Successfully encoded %zu bytes\n", encoded_length);
}
```

## Result Types

<details>
<summary>More about Result types</summary>

-----

This idea is directly borrowed from Rust.

This is a type alias for the type `result_cbor_value_t_cbor_parser_error_t_t`, which is defined by the following macros:


```c
DEFINE_RESULT_TYPE(cbor_value_t, cbor_parser_error_t);
FN_RESULT (
    cbor_value_t, cbor_parser_error_t,
    parse, slice_t buf
);
```

The first one, `DEFINE_RESULT_TYPE`, takes an `oktype` and an `errtype` and creates a struct which is a tagged union containing an `is_error` value and a union of `oktype ok` and `errtype err`. The name of this type is intentionally long and verbose, to avoid name collisions.

The second one similarly takes the oktype and the errtype and in addition takes a function name and variable length arguments.

This function typedefs the `result_oktype_errtype_t` into `function_name_result_t`. This new name is often more convenient to use and likely avoid any name collisions by assuming that function names do not collide.

The macros `OK()` and `ERR()` take the name of the function and a value of `oktype` or `errortype` respectively and generate the result structure.

In terms of performance, i thought about it. There will be a performance cost to this, but none of it is something that the compiler can't optimize. *In GCC we trust.*

</details>

# Building

Run `make` or `make TARGET=native` to build everything for linux.

Run `make TARGET=embedded` to build for Arm Cortex M3

Run `make help` for help

Run `make clean` to clean all the build artifacts

Build artifacts are generated in the `build/TARGET`

## Running the arm build on QEMU

You can use this command to run `build/embedded/main.elf` in QEMU

```sh
qemu-system-arm -M lm3s6965evb -cpu cortex-m3 -nographic -semihosting -kernel build/embedded/main.elf
```

> Note: According to Deepseek, the lm3s6965evb is the most similar QEMU machine to a CC2538. That's why I used it.