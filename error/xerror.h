#ifndef _XERROR_H
#define _XERROR_H

#include <stdio.h>
#include <string.h>
#include <errno.h>

extern int *_errno_value(void);
#define xerrno (*_errno_value())

#define preserve_errno(stmt) do { \
	int _err = errno, _xerr = xerrno; \
	stmt; \
	errno = _err; xerrno = _xerr; \
} while(0)

#define return_if(cond, no) do { \
	if(cond) { \
		xerrno = (no); \
		return -1; \
	} \
} while(0)

#define FATAL_FSIZE 1		// Invalid file size
#define FATAL_READLESS 2		// Read less than expected length
#define FATAL_BLKSIZE 3		// Wrong block size
#define FATAL_BLKTAG 4		// Wrong block tag

#define xperror(s) fprintf(stderr, "%s: %s\n", (s), __err_strings[xerrno])

#define perror(s) do { if(errno) perror(s); if(xerrno) xperror(s); } while(0)

#endif
