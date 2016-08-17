/* Copyright (C) 2016 Gan Quan <coin2028@hotmail.com>
 *
 * This file is part of AIMv6.
 *
 * AIMv6 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * AIMv6 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SYS_ENDIAN_H
#define _SYS_ENDIAN_H

#include <sys/types.h>

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define LITTLE_ENDIAN
#endif

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define BIG_ENDIAN
#endif

#define __bswap16(x)	(__builtin_bswap32(x) >> 16)
#define __bswap32(x)	__builtin_bswap32(x)
#define __bswap64(x)	__builtin_bswap64(x)

#if defined(LITTLE_ENDIAN)
static inline uint16_t htobe16(uint16_t x) { return __bswap16(x); }
static inline uint16_t htole16(uint16_t x) { return x; }
static inline uint16_t be16toh(uint16_t x) { return __bswap16(x); }
static inline uint16_t le16toh(uint16_t x) { return x; }
static inline uint32_t htobe32(uint32_t x) { return __bswap32(x); }
static inline uint32_t htole32(uint32_t x) { return x; }
static inline uint32_t be32toh(uint32_t x) { return __bswap32(x); }
static inline uint32_t le32toh(uint32_t x) { return x; }
static inline uint64_t htobe64(uint64_t x) { return __bswap64(x); }
static inline uint64_t htole64(uint64_t x) { return x; }
static inline uint64_t be64toh(uint64_t x) { return __bswap64(x); }
static inline uint64_t le64toh(uint64_t x) { return x; }
#elif defined(BIG_ENDIAN)
static inline uint16_t htole16(uint16_t x) { return __bswap16(x); }
static inline uint16_t htobe16(uint16_t x) { return x; }
static inline uint16_t le16toh(uint16_t x) { return __bswap16(x); }
static inline uint16_t be16toh(uint16_t x) { return x; }
static inline uint32_t htole32(uint32_t x) { return __bswap32(x); }
static inline uint32_t htobe32(uint32_t x) { return x; }
static inline uint32_t le32toh(uint32_t x) { return __bswap32(x); }
static inline uint32_t be32toh(uint32_t x) { return x; }
static inline uint64_t htole64(uint64_t x) { return __bswap64(x); }
static inline uint64_t htobe64(uint64_t x) { return x; }
static inline uint64_t le64toh(uint64_t x) { return __bswap64(x); }
static inline uint64_t be64toh(uint64_t x) { return x; }
#endif

#endif
