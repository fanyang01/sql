#include "file.h"
#include "alloc.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <strings.h>

int main(void)
{
	FILE *f = tmpfile();
	assert(f != NULL);
	int fd = fileno(f);

	ALLOC *a;
	assert((a = new_allocator(fd, ALLOC_M_OPEN)) == NULL);

	assert((a = new_allocator(fd, ALLOC_M_NEW)) != NULL);
	assert(fsize(fd) == ALLOC_FLT_LEN);

	char buf[] = "hello, world";
	handle_t h, h1;
	assert((h = alloc_blk(a, buf, sizeof(buf))) != 0);
	assert(h == 1);
	assert(fsize(fd) % 16 == 0);

	size_t size = sizeof(buf);
	bzero(buf, sizeof(buf));
	assert(read_blk(a, h, buf, &size) == buf);
	assert(size == sizeof(buf));
	assert(strncmp(buf, "hello, world", sizeof(buf)) == 0);

	char buf1[] =
	    "hellohellohellohellohellohellohellohellohellohellohellohellohellohellohelloh";
	char buf2[] =
	    "hellohellohellohellohellohellohellohellohellohellohellohellohellohellohelloh";
	h1 = h;
	assert((h = alloc_blk(a, buf1, sizeof(buf1))) != 0);
	assert(fsize(fd) % 16 == 0);

	size = sizeof(buf1);
	bzero(buf1, sizeof(buf1));
	assert(read_blk(a, h, buf1, &size) == buf1);
	assert(size == sizeof(buf1));
	assert(strncmp(buf1, buf2, sizeof(buf1)) == 0);

	// Is buf's "hello, world" still there?
	size = sizeof(buf);
	bzero(buf, sizeof(buf));
	assert(read_blk(a, h1, buf, &size) == buf);
	assert(size == sizeof(buf));
	assert(strncmp(buf, "hello, world", sizeof(buf)) == 0);

	// replace buf1's content
	assert(realloc_blk(a, h, buf, sizeof(buf)) == 0);
	size = sizeof(buf);
	bzero(buf, sizeof(buf));
	assert(read_blk(a, h1, buf, &size) == buf);
	assert(size == sizeof(buf));
	assert(strncmp(buf, "hello, world", sizeof(buf)) == 0);

	printf("PASS\n");
	return 0;
}
