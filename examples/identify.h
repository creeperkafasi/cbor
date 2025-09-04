#ifndef EXAMPLE_IDENTIFY_H
#define EXAMPLE_IDENTIFY_H

#include <stdint.h>
#include "cbor.h"

// X macro for iterating over the identify parameters
#define IDENTIFY_PARAMETERS           \
  X(rg, CBOR_KEY_REGISTERED)          \
  X(b, CBOR_KEY_BRAND)                \
  X(m, CBOR_KEY_MODEL)                \
  X(t, CBOR_KEY_TYPE)                 \
  X(pv, CBOR_KEY_PROTOCOLVERSION)     \
  X(md, CBOR_KEY_MANUFACTUREDATE)     \
  X(fw, CBOR_KEY_FIRMWARE)            \
  X(sg, CBOR_KEY_SIGNAL)              \
  X(hbp, CBOR_KEY_HEARTBEATPERIOD)    \
  X(dd, CBOR_KEY_DEVICEDATE)          \
  /*X(dls, CBOR_KEY_DAYLIGHTSAVING)*/ \
  /*X(tz, CBOR_KEY_TIMEZONE) */       \
  X(rp, CBOR_KEY_RESTARTPERIOD)       \
  X(rds, CBOR_KEY_READDATALIFESPAN)   \
  /* X(svs, CBOR_KEY_SERVERS)*/       \
  /* X(ntp, CBOR_KEY_NTP)*/           \
  /* X(iwl, CBOR_KEY_IPADDRESSWHITELIST)*/\
  X(rti, CBOR_KEY_RETRYINTERVAL)      \
  X(rtc, CBOR_KEY_RETRYCOUNT)         \
  X(mps, CBOR_KEY_MAXPACKAGESIZE)     \
  /* X(mqtt, CBOR_KEY_MQTT) */        \
  X(cif, CBOR_KEY_COMMUNICATIONINTERFACES)\
  X(sps, CBOR_KEY_SERIALPORTS)        \
  X(ioi, CBOR_KEY_IOINTERFACES)       \
  /* X(ar, CBOR_KEY_ASTRORELAY) */    \
  /* X(mo, CBOR_KEY_MODULE) */        \
  X(mes, CBOR_KEY_METERS)

#define X(name, key) const char key[] = #name;
IDENTIFY_PARAMETERS
#undef X

// Bit shift amounts
enum identify_response_shifts {
  #define X(name, key) IDENTIFY_SHIFT_ ## name,
  IDENTIFY_PARAMETERS
  #undef X
  IDENTIFY_PARAMETERS_COUNT
};

// Bit masks
enum identify_response_bits {
  #define X(name, key) IDENTIFY_MASK_ ## name = (1 << IDENTIFY_SHIFT_ ## name),
  IDENTIFY_PARAMETERS
  #undef X
  IDENTIFY_MASK_ALL = (1 << IDENTIFY_PARAMETERS_COUNT) - 1
};

typedef uint32_t identify_bitmap_t;
_Static_assert(IDENTIFY_PARAMETERS_COUNT <= (sizeof(identify_bitmap_t) * 8), "Too many identify parameters for bitmap");

typedef struct {
    slice_t f;
    slice_t sn;
} device_info_t;

typedef struct {
    device_info_t d;
    int fn;
    int64_t rid;
    identify_bitmap_t request_bitmap;
} identification_request_t;

#endif // EXAMPLE_IDENTIFY_H
