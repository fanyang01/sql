#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include "file.h"
#include "xerror.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

ssize_t readat(int fd, void *buf, size_t count, off_t offset)
{
	ssize_t n = pread(fd, buf, count, offset);
	if (n != count)
		xerrno = FATAL_RIO;
	return n;
}

ssize_t writeat(int fd, void *buf, size_t count, off_t offset)
{
	ssize_t n = pwrite(fd, buf, count, offset);
	if (n != count)
		xerrno = FATAL_WIO;
	return n;
}

int alloc(int fd, off_t offset, off_t len)
{
	// TODO: result in slower insertion
	//
	// pre-allocate one more page
	/* if (fallocate(fd, FALLOC_FL_KEEP_SIZE, offset, */
	/*            ((((offset + len) >> 12) + 2) << 12) - offset) != 0) */
	/*      return -1; */
	/* return posix_fallocate(fd, offset, len); */
	/* return fallocate(fd, 0, offset, len); */
	if (fsize(fd) < offset + len)
		return ftruncate(fd, offset + len);
	return 0;
}

int dealloc(int fd, off_t offset, off_t len)
{
	return fallocate(fd, FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE,
			 offset, len);
}

off_t fsize(int fd)
{
	struct stat st;
	if (fstat(fd, &st) == -1)
		return -1;
	return st.st_size;
}
