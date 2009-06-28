
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


#ifdef BIG_ENDIAN_P

#define UINT32_TO_LE(x) (BSWAP_32(x))
#define UINT32_FROM_LE(x) (BSWAP_32(x))

#define UINT64_TO_LE(x) (BSWAP_64(x))
#define UINT64_FROM_LE(x) (BSWAP_64(x))

#warning BigEndian
#else

#define UINT32_TO_LE(x) (x)
#define UINT32_FROM_LE(x) (x)

#define UINT64_TO_LE(x) (x)
#define UINT64_FROM_LE(x) (x)

#endif

#endif /* ENDIAN_H_ */
