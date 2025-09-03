# CBOR

Safe CBOR library written in C

## Parsing

|  | |
|---------------|-----------|
| Integers      |✅Supported|
| Byte Strings  |✅Supported|
| Text Strings  |✅Supported|
| Arrays        |✅Supported|
| Maps          |✅Supported|
| Indefinite Lengths |⛔Not Supported|
| Simples       |✅Supported|
| Floats        |✅Supported|
| Tags          |⛔Not Supported|

### Example: Parsing

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

## Encoding

🕒 WIP


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