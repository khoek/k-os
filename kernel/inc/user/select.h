#ifndef KERNEL_USER_SELECT_H
#define KERNEL_USER_SELECT_H

#include "common/types.h"

#define NBBY 8
#define FD_SETSIZE 64

typedef uint32_t fd_mask;
#define NFDBITS	(sizeof(fd_mask) * NBBY)

#ifndef howmany
#define	howmany(x, y)	(((x)+((y)-1))/(y))
#endif

typedef	struct fd_set {
	fd_mask	fds_bits[howmany(FD_SETSIZE, NFDBITS)];
} fd_set;

#define	FD_SET(n, p)   ((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#define	FD_CLR(n, p)   ((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#define	FD_ISSET(n, p) ((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
#define	FD_ZERO(p)     memset((p), 0, sizeof(*(p)))

#endif
