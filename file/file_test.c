#include "file.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

int main(void)
{
	FILE *f = tmpfile();
	assert(f != NULL);
	int fd = fileno(f);
	char buf[] = "1234567890";

	assert(fsize(fd) == 0);
	assert(alloc(fd, 0, 4093) == 0);
	assert(fsize(fd) == 4093);
	assert(alloc(fd, 0, 4096) == 0);
	assert(fsize(fd) == 4096);
	assert(dealloc(fd, 0, 4096) == 0);
	assert(fsize(fd) == 4096);

	assert(writeat(fd, buf, sizeof(buf), 0) == sizeof(buf));
	assert(readat(fd, buf, sizeof(buf), 0) == sizeof(buf));
	assert(strncmp(buf, "1234567890", 11) == 0);

	return 0;
}
