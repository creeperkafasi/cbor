#include <stdio.h>

#if !__FILE_defined

#error "JSON encoding is not available without FILE support"

#elif !defined(CBOR_JSON)
#define CBOR_JSON

#include "cbor.h"
#include <stdbool.h>

typedef struct cbor_json_options_t {
    bool newline;
    int indent_step;
} cbor_json_options_t;

typedef struct cbor_json_print_state_t {
    cbor_json_options_t options;

    int indent;
    int* counter;
    FILE* target;
} cbor_json_print_state_t;

void print_cbor_value_json(cbor_value_t value, int indent, FILE* target);
cbor_custom_processor_result_t print_single_json(const cbor_value_t* value, void* arg);
cbor_custom_processor_result_t print_pair_json(const cbor_value_t* key, const cbor_value_t* value, void* arg);

#endif /* CBOR_JSON */