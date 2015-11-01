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

#define FATAL_INVDB 0x1		// Invalid or damaged DB file
#define FATAL_RIO 0x2		// Read error
#define FATAL_WIO 0x3		// Write error
#define FATAL_BLKSIZE 0x4	// Wrong block size
#define FATAL_BLKTAG 0x5	// Wrong block tag
#define FATAL_OFFSET 0x6	// Invalid file offset
#define FATAL_NOMEM 0x7		// Out of memory
#define FATAL_BLKNO 0x8		// Unexpected block number

#define ERR_NCOL 0x11		// Unmatched number of columns
#define ERR_COLTYPE 0x12	// Unmatched column type
#define ERR_TOOLONG 0x13	// String too long
#define ERR_DPCNAME 0x14	// Duplicate column name
#define ERR_NPRIMARY 0x15	// Incorrect number of primary key
#define ERR_DPTABLE 0x16	// A table with the same name exists

#define xperror(s) fprintf(stderr, "%s: %s\n", (s), __err_strings[xerrno])
#define perror(s) do { if(errno) perror(s); if(xerrno) xperror(s); } while(0)

#endif
