/*
 * Copyright (c) 2009-2010, Albertas Vyšniauskas
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of the software author nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ENDIAN_H_
#define ENDIAN_H_

#include <stdint.h>

#define	BSWAP_64(x)	(((uint64_t)(x) << 56) | \
					(((uint64_t)(x) << 40) & 0xff000000000000ULL) | \
					(((uint64_t)(x) << 24) & 0xff0000000000ULL) | \
					(((uint64_t)(x) << 8)  & 0xff00000000ULL) | \
					(((uint64_t)(x) >> 8)  & 0xff000000ULL) | \
					(((uint64_t)(x) >> 24) & 0xff0000ULL) | \
					(((uint64_t)(x) >> 40) & 0xff00ULL) | \
					((uint64_t)(x)  >> 56))

#define BSWAP_32(x)	(((uint32_t)(x) << 24) | \
					(((uint32_t)(x) << 8)  & 0xff0000ULL) | \
					(((uint32_t)(x) >> 8)  & 0xff00ULL) | \
					((uint32_t)(x)  >> 24))

#define BSWAP_16(x)	(((uint16_t)(x) << 8) | \
					((uint16_t)(x) >> 8))



#ifdef BIG_ENDIAN_P

#define UINT16_TO_LE(x) (BSWAP_16(x))
#define UINT16_FROM_LE(x) (BSWAP_16(x))

#define UINT32_TO_LE(x) (BSWAP_32(x))
#define UINT32_FROM_LE(x) (BSWAP_32(x))

#define UINT64_TO_LE(x) (BSWAP_64(x))
#define UINT64_FROM_LE(x) (BSWAP_64(x))

#define UINT16_TO_BE(x) (x)
#define UINT16_FROM_BE(x) (x)

#define UINT32_TO_BE(x) (x)
#define UINT32_FROM_BE(x) (x)

#define UINT64_TO_BE(x) (x)
#define UINT64_FROM_BE(x) (x)

#warning BigEndian
#else

#define UINT16_TO_LE(x) (x)
#define UINT16_FROM_LE(x) (x)

#define UINT32_TO_LE(x) (x)
#define UINT32_FROM_LE(x) (x)

#define UINT64_TO_LE(x) (x)
#define UINT64_FROM_LE(x) (x)

#define UINT16_TO_BE(x) (BSWAP_16(x))
#define UINT16_FROM_BE(x) (BSWAP_16(x))

#define UINT32_TO_BE(x) (BSWAP_32(x))
#define UINT32_FROM_BE(x) (BSWAP_32(x))

#define UINT64_TO_BE(x) (BSWAP_64(x))
#define UINT64_FROM_BE(x) (BSWAP_64(x))

#endif

#endif /* ENDIAN_H_ */
