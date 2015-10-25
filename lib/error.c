#include <pthread.h>
#include <stdlib.h>

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
