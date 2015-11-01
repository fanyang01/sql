#include "xerror.h"
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>

char *__err_strings[] = {
	[0] = "Success",
	[FATAL_FSIZE] = "Invalid file size",
	[FATAL_READLESS] = "Read less than expected length",
	[FATAL_BLKSIZE] = "Wrong block size",
	[FATAL_BLKTAG] = "Wrong block tag",
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
