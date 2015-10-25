#ifndef _XERROR_H
#define _XERROR_H

#include <stdio.h>
#include <string.h>
#include <errno.h>

extern int *_errno_value(void);
#define xerrno (*_errno_value())

#define ERR_FSIZE 1		// Invalid file size
#define ERR_READLESS 2		// Read less than expected length
#define ERR_BLKSIZE 3		// Wrong block size
#define ERR_BLKTAG 4		// Wrong block tag

char *err_strings[] = {
	[0] = "Success",
	[ERR_FSIZE] = "Invalid file size",
	[ERR_READLESS] = "Read less than expected length",
	[ERR_BLKSIZE] = "Wrong block size",
	[ERR_BLKTAG] = "Wrong block tag",
};

#define xstrerror(no) (err_strings[(no)])
#define xperror(s) fprintf(stderr, "%s: %s\n", (s), err_strings[xerrno])

#define perror(s) do { if(errno) perror(s); else xperror(s); } while(0)
#define strerror(no) (errno ? strerror(no) : xstrerror(no))

#endif
