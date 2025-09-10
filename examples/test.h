#ifndef CBOR_TEST_H
#define CBOR_TEST_H

#include "cbor.h"

#define TEST_ASSERT(condition, message) \
    do { \
        if (condition) { \
            printf("✅ PASS: %s\n", message); \
            tests_passed++; \
        } else { \
            printf("❌ FAIL: %s\n", message); \
            tests_failed++; \
        } \
    } while(0)

#endif /* CBOR_TEST_H */
