#ifndef _FILE_H
#define _FILE_H

#include <unistd.h>

extern ssize_t readat(int fd, void *buf, size_t count, off_t offset);
extern ssize_t writeat(int fd, void *buf, size_t count, off_t offset);
extern int alloc(int fd, off_t offset, off_t len);
extern int dealloc(int fd, off_t offset, off_t len);
extern off_t fsize(int fd);
extern int fshrink(int fd, off_t len);

#endif
