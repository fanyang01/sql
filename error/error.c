#include "xerror.h"
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>

char *__err_strings[] = {
	[0] = "Success",
	[FATAL_INVDB] = "Invalid or damaged DB file",
	[FATAL_RIO] = "Read error",
	[FATAL_WIO] = "Write error",
	[FATAL_BLKSIZE] = "Wrong block size",
	[FATAL_BLKTAG] = "Wrong block tag",
	[FATAL_OFFSET] = "Invalid file offset",
	[FATAL_NOMEM] = "Out of memory",
	[FATAL_BLKNO] = "Unexpected block number",
	[ERR_NCOL] = "Unmatched number of columns",
	[ERR_COLTYPE] = "Unmatched column type",
	[ERR_TOOLONG] = "String too long",
	[ERR_DPCNAME] = "Duplicate column name",
	[ERR_NPRIMARY] = "Incorrect number of primary key",
	[ERR_DPTABLE] = "A table with the same name exists",
	[ERR_NOCOL] = "Column not in table",
	[ERR_INVOP] = "Invalid selection operator",
	[ERR_DPIDX] =
	    "An index with the same name or on the same column exists",
	[ERR_UNIQ] = "Not meet the unique constraint",
	[ERR_TMNCOL] = "Too many columns",
	[ERR_NOTABLE] = "No such table",
	[ERR_NOIDX] = "No such index",
	[ERR_TMNCOND] = "Too much query conditions",
	[ERR_INVSTMT] = "Unsupported SQL statement",
	[ERR_INVTYPE] = "Unsupported column type",
	[ERR_ZEROSLEN] = "Length of fix-length string can't be zero",
};

static pthread_once_t _errno_once = PTHREAD_ONCE_INIT;
static pthread_key_t _key;

static void _init_errno(void)
{
	pthread_key_create(&_key, free);
}

int *_errno_value(void)
{
	int *p;
	pthread_once(&_errno_once, _init_errno);
	if ((p = pthread_getspecific(_key)) == NULL) {
		p = malloc(sizeof(int));
		pthread_setspecific(_key, p);
	}
	return p;
}
