#ifndef EXAMPLE_IDENTIFY_H
#define EXAMPLE_IDENTIFY_H

#include <stdint.h>
#include "cbor.h"

// X macro for iterating over the identify parameters
#define IDENTIFY_PARAMETERS           \
  X(rg, REGISTERED)          \
  X(b, BRAND)                \
  X(m, MODEL)                \
  X(t, TYPE)                 \
  X(pv, PROTOCOLVERSION)     \
  X(md, MANUFACTUREDATE)     \
  X(fw, FIRMWARE)            \
  X(sg, SIGNAL)              \
  X(hbp, HEARTBEATPERIOD)    \
  X(dd, DEVICEDATE)          \
  /*X(dls, DAYLIGHTSAVING)*/ \
  /*X(tz, TIMEZONE) */       \
  X(rp, RESTARTPERIOD)       \
  X(rds, READDATALIFESPAN)   \
  /* X(svs, SERVERS)*/       \
  /* X(ntp, NTP)*/           \
  /* X(iwl, IPADDRESSWHITELIST)*/\
  X(rti, RETRYINTERVAL)      \
  X(rtc, RETRYCOUNT)         \
  X(mps, MAXPACKAGESIZE)     \
  /* X(mqtt, MQTT) */        \
  X(cif, COMMUNICATIONINTERFACES)\
  X(sps, SERIALPORTS)        \
  X(ioi, IOINTERFACES)       \
  /* X(ar, ASTRORELAY) */    \
  /* X(mo, MODULE) */        \
  X(mes, METERS)

#define X(name, key) const char CBOR_KEY_ ## key[] = #name;
IDENTIFY_PARAMETERS
#undef X

// Bit shift amounts
enum identify_response_shifts {
  #define X(name, key) IDENTIFY_SHIFT_ ## key,
  IDENTIFY_PARAMETERS
  #undef X
  IDENTIFY_PARAMETERS_COUNT
};

// Bit masks
enum identify_response_bits {
  #define X(name, key) IDENTIFY_MASK_ ## key = (1 << IDENTIFY_SHIFT_ ## key),
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
