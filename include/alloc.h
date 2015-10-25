#include <stdlib.h>
#include <stdint.h>

#define ALLOC_M_OPEN 0
#define ALLOC_M_NEW 1
#define ALLOC_FLT_SIZE 14	/* 2 ^ 14 = 64K / 16  */
#define ALLOC_ATOM_LEN 16
#define ALLOC_FLT_LEN (ALLOC_FLT_SIZE * sizeof(handle_t))

#define CTBLK_MAXSHORT 254
#define CTBLK_MAXLONG 65533
#define CTBLK_FLAG_SHORT 0x1
#define CTBLK_FLAG_LONG 0x2
#define REBLK_FLAG 0x4
#define FRBLK_FLAG_SINGLE 0x8
#define FRBLK_FLAG_LONG 0x10

typedef uint64_t handle_t;
typedef struct {
	int fd;
	handle_t flt[ALLOC_FLT_SIZE];
} ALLOC;

extern ALLOC *new_allocator(int fd, int mode);
extern handle_t alloc_blk(ALLOC * a, void *buf, size_t len);
extern int dealloc_blk(ALLOC * a, handle_t handle);
extern int realloc_blk(ALLOC * a, handle_t handle, void *buf, size_t len);
extern void *read_blk(ALLOC * a, handle_t handle, void *buf, size_t len);
