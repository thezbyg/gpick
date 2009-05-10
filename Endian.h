
#ifndef ENDIAN_H_
#define ENDIAN_H_

#define	ULONG_SWAP_LE_BE(ulong)	(((unsigned long) \
	(((unsigned long) (ulong)) & 0x000000ffU) << 24) | \
	(((unsigned long) (ulong)) & 0x0000ff00U) <<  8) | \
	(((unsigned long) (ulong)) & 0x00ff0000U) >>  8) | \
	(((unsigned long) (ulong)) & 0xff000000U) >> 24)))

#ifdef BIG_ENDIAN_P
#define ULONG_TO_LE(ulong) (ULONG_SWAP_LE_BE(ulong))
#define ULONG_FROM_LE(ulong) (ULONG_SWAP_LE_BE(ulong))
#warning BigEndian
#else
#define ULONG_TO_LE(ulong) (ulong)
#define ULONG_FROM_LE(ulong) (ulong)
#endif

#endif /* ENDIAN_H_ */
