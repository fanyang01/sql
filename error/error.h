#ifndef _X_ERROR_H
#define _X_ERROR_H

extern int *_errno_value(void);
#define x_errno (*_errno_value())

#define ERR_FSIZE 1		// Invalid file size
#define ERR_READLESS 2		// Read less than expected length
#define ERR_BLKSIZE 3		// Wrong block size
#define ERR_BLKTAG 4		// Wrong block tag

#endif
