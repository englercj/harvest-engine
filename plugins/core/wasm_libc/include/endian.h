// Copyright Chad Engler

#pragma once

#include "_common.h"

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BIG_ENDIAN __BIG_ENDIAN
#define LITTLE_ENDIAN __LITTLE_ENDIAN
#define PDP_ENDIAN __PDP_ENDIAN
#define BYTE_ORDER __BYTE_ORDER

_Forceinline uint16_t __bswap16(uint16_t x) { return (x << 8) | (x >> 8); }
_Forceinline uint32_t __bswap32(uint32_t x) { return (x >> 24) | ((x >> 8) & 0xff00) | ((x << 8) & 0xff0000) | (x << 24); }
_Forceinline uint64_t __bswap64(uint64_t x) { return ((__bswap32(x) + 0ULL) << 32) | __bswap32(x >> 32); }

#if __BYTE_ORDER == __LITTLE_ENDIAN
_Forceinline uint16_t htobe16(uint16_t x) { return __bswap16(x); }
_Forceinline uint16_t be16toh(uint16_t x) { return __bswap16(x); }
_Forceinline uint32_t htobe32(uint32_t x) { return __bswap32(x); }
_Forceinline uint32_t be32toh(uint32_t x) { return __bswap32(x); }
_Forceinline uint64_t htobe64(uint64_t x) { return __bswap64(x); }
_Forceinline uint64_t be64toh(uint64_t x) { return __bswap64(x); }
_Forceinline uint16_t htole16(uint16_t x) { return (uint16_t)(x); }
_Forceinline uint16_t le16toh(uint16_t x) { return (uint16_t)(x); }
_Forceinline uint32_t htole32(uint32_t x) { return (uint32_t)(x); }
_Forceinline uint32_t le32toh(uint32_t x) { return (uint32_t)(x); }
_Forceinline uint64_t htole64(uint64_t x) { return (uint64_t)(x); }
_Forceinline uint64_t le64toh(uint64_t x) { return (uint64_t)(x); }

_Forceinline uint16_t betoh16(uint16_t x) { return __bswap16(x); }
_Forceinline uint32_t betoh32(uint32_t x) { return __bswap32(x); }
_Forceinline uint64_t betoh64(uint64_t x) { return __bswap64(x); }
_Forceinline uint16_t letoh16(uint16_t x) { return (uint16_t)(x); }
_Forceinline uint32_t letoh32(uint32_t x) { return (uint32_t)(x); }
_Forceinline uint64_t letoh64(uint64_t x) { return (uint64_t)(x); }
#else
_Forceinline uint16_t htobe16(uint16_t x) { return (uint16_t)(x); }
_Forceinline uint16_t be16toh(uint16_t x) { return (uint16_t)(x); }
_Forceinline uint32_t htobe32(uint32_t x) { return (uint32_t)(x); }
_Forceinline uint32_t be32toh(uint32_t x) { return (uint32_t)(x); }
_Forceinline uint64_t htobe64(uint64_t x) { return (uint64_t)(x); }
_Forceinline uint64_t be64toh(uint64_t x) { return (uint64_t)(x); }
_Forceinline uint16_t htole16(uint16_t x) { return __bswap16(x); }
_Forceinline uint16_t le16toh(uint16_t x) { return __bswap16(x); }
_Forceinline uint32_t htole32(uint32_t x) { return __bswap32(x); }
_Forceinline uint32_t le32toh(uint32_t x) { return __bswap32(x); }
_Forceinline uint64_t htole64(uint64_t x) { return __bswap64(x); }
_Forceinline uint64_t le64toh(uint64_t x) { return __bswap64(x); }

_Forceinline uint16_t betoh16(uint16_t x) { return (uint16_t)(x); }
_Forceinline uint32_t betoh32(uint32_t x) { return (uint32_t)(x); }
_Forceinline uint64_t betoh64(uint64_t x) { return (uint64_t)(x); }
_Forceinline uint16_t letoh16(uint16_t x) { return __bswap16(x); }
_Forceinline uint32_t letoh32(uint32_t x) { return __bswap32(x); }
_Forceinline uint64_t letoh64(uint64_t x) { return __bswap64(x); }
#endif

#ifdef __cplusplus
}
#endif
