#ifndef CBOR_COMPAT_ENDIAN_H
#define CBOR_COMPAT_ENDIAN_H

#include <stdint.h>

# if __BYTE_ORDER == __LITTLE_ENDIAN
#  define htobe16(x) __builtin_bswap16 (x)
#  define htole16(x) __uint16_identity (x)
#  define be16toh(x) __builtin_bswap16 (x)
#  define le16toh(x) __uint16_identity (x)

#  define htobe32(x) __builtin_bswap32 (x)
#  define htole32(x) __uint32_identity (x)
#  define be32toh(x) __builtin_bswap32 (x)
#  define le32toh(x) __uint32_identity (x)

#  define htobe64(x) __builtin_bswap64 (x)
#  define htole64(x) __uint64_identity (x)
#  define be64toh(x) __builtin_bswap64 (x)
#  define le64toh(x) __uint64_identity (x)

# else
#  define htobe16(x) __uint16_identity (x)
#  define htole16(x) __builtin_bswap16 (x)
#  define be16toh(x) __uint16_identity (x)
#  define le16toh(x) __builtin_bswap16 (x)

#  define htobe32(x) __uint32_identity (x)
#  define htole32(x) __builtin_bswap32 (x)
#  define be32toh(x) __uint32_identity (x)
#  define le32toh(x) __builtin_bswap32 (x)

#  define htobe64(x) __uint64_identity (x)
#  define htole64(x) __builtin_bswap64 (x)
#  define be64toh(x) __uint64_identity (x)
#  define le64toh(x) __builtin_bswap64 (x)
# endif

#endif /* CBOR_COMPAT_ENDIAN_H */