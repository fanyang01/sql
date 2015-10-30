#include "alloc.h"
#include <stdlib.h>

void *buf_get(ALLOC * a, size_t len)
{
	return calloc(1, len);
}

void buf_put(ALLOC * a, void *p)
{
	free(p);
}
