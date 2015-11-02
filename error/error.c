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
	[ERR_DPIDX] = "An index on this column exists",
	[ERR_UNIQ] = "Not meet the unique constraint",
	[ERR_TMNCOL] = "Too many columns",
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
