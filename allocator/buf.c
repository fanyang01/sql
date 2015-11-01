#include "alloc.h"
#include "xerror.h"
#include <stdlib.h>

void *buf_get(ALLOC * a, size_t len)
{
	void *p = calloc(1, len);
	if (p == NULL)
		xerrno = FATAL_NOMEM;
	return p;
}

void buf_put(ALLOC * a, void *p)
{
	preserve_errno(free(p));
}
