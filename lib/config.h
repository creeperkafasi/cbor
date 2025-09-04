#ifndef CBOR_CONFIG_H
#define CBOR_CONFIG_H

#define CBOR_DEBUG_REPR

// Stack optimization settings
#ifdef TARGET_EMBEDDED
    #define CBOR_OPTIMIZE_STACK 1
    #define CBOR_MAX_RECURSION_DEPTH 4  // Limit for Contiki-NG
#endif

#endif /*CBOR_CONFIG_H*/
