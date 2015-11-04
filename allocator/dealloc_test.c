#include "alloc.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
	FILE *f = tmpfile();
	assert(f != NULL);
	int fd = fileno(f);
	ALLOC allocator;
	ALLOC *a = &allocator;

	assert(init_allocator(a, fd, O_CREAT | O_TRUNC) == 0);

	char buf[] = "hello, world";
	handle_t h, h1, h2, h3;
	assert((h1 = alloc_blk(a, buf, sizeof(buf))) != 0);
	assert((h2 = alloc_blk(a, buf, sizeof(buf))) != 0);
	assert((h3 = alloc_blk(a, buf, sizeof(buf))) != 0);
	assert(dealloc_blk(a, h2) == 0);
	assert((h = alloc_blk(a, buf, sizeof(buf))) != 0);

	return 0;
}
