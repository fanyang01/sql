#ifndef _XERROR_H
#define _XERROR_H

#include <stdio.h>
#include <string.h>
#include <errno.h>

extern char *__err_strings[];
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

#define FATAL_MAX 0x10

#define ERR_NCOL 0x11		// Unmatched number of columns
#define ERR_COLTYPE 0x12	// Unmatched column type
#define ERR_TOOLONG 0x13	// String too long
#define ERR_DPCNAME 0x14	// Duplicate column name
#define ERR_NPRIMARY 0x15	// Incorrect number of primary key
#define ERR_DPTABLE 0x16	// A table with the same name exists
#define ERR_NOCOL 0x17		// Column not in table
#define ERR_INVOP 0x18		// Invalid selection operator
#define ERR_DPIDX 0x19		// An index with the same name or on the same column exists
#define ERR_UNIQ 0x1A		// Not meet the unique constraint
#define ERR_TMNCOL 0x1B		// Too many columns
#define ERR_NOTABLE 0x1C	// No such table
#define ERR_NOIDX 0x1D		// No such index
#define ERR_TMNCOND 0x1E	// Too much query conditions
#define ERR_INVSTMT 0x1F	// Unsupported SQL statement
#define ERR_INVTYPE 0x20	// Unsupported column type
#define ERR_ZEROSLEN 0x21	// Length of fix-length string can't be zero

#define xperror(s) fprintf(stderr, "%s: %s\n", (s), __err_strings[xerrno])
#define perror(s) do { \
	if(xerrno) { \
		xperror(s); \
		break; \
	} \
	if(errno) perror(s); \
} while(0)

#endif
