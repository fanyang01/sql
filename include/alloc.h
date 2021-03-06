#ifndef _ALLOC_H
#define _ALLOC_H

/*
 * handle:
 * - store as 7 bytes in NETWORK ORDER
 * - zero v means NULL
 * 
 *
 * FLT:
 *
 * - 14 slots, 8 bytes each slot
 * - each slot is head of a double-linked list of free blocks whose
 *   size in atoms are between [2^index, 2^(index+1))
 * - except the last slot: [2^index, +inf)
 *
 *
 * block type:
 *
 * - short content block
 * - long content block
 * - relocated used block
 * - single-atom free block
 * - long free block
 *
 * short content block
 * ===================
 * - 1st byte: 0x1
 * - 2nd byte: length of content, [0, 254(0xFE)]
 * - number of atoms: [1, 16]
 *
 * long content block
 * ==================
 * - 1st byte: 0x2
 * - 2nd and 3rd bytes: length of content, [255(0xFF), 65533(0xFFFD)],
 *   NETWORK ORDER
 * - number of atoms: [17, 4096]
 *
 * relocated used block
 * ====================
 * - 1st byte: 0x4
 * - 2...8 bytes: handle to a short or long content block
 * - number of atoms: 1
 *
 * single-atom free block
 * ======================
 * - 1st byte: 0x8
 * - 2...8 bytes: handle to previous free block
 * - 9...15 bytes: handle to next free block
 * - 16th byte: 0x8
 * - number of atoms: 1
 *
 * long free block
 * ===============
 * - 1st byte: 0x10
 * - 2...8 bytes: handle to previous free block
 * - 9...15 bytes: handle to next free block
 * - 16th byte: 0x10
 * - 17...23 bytes: size of this block in atoms, [2, 2^56-1], NETWORK ORDER
 */
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define FLT_SIZE 14		/* 2 ^ 14 = 64K / 16  */
#define ATOM_LEN 16
#define FLT_LEN (FLT_SIZE * sizeof(handle_t))

#define LRU_NSLOT 2017		// a prime
#define LRU_SIZE (1<<22)

#define CTBLK_MAXSHORT 254
#define CTBLK_MAXLONG 65533
#define CTBLK_SHORT 0x1
#define CTBLK_LONG 0x2
#define REBLK 0x4
#define FRBLK_SINGLE 0x8
#define FRBLK_LONG 0x10

typedef uint64_t handle_t;

struct lru_node {
	struct lru_node *hash_next;
	struct lru_node *prev, *next;
	handle_t self;
	void *block;
	size_t size;
};

typedef struct {
	int fd;
	handle_t flt[FLT_SIZE];
	struct lru_node **lru;
	struct lru_node *lru_head;
	struct lru_node *lru_tail;
	size_t lru_size;
} ALLOC;

/*
 * Returns an allocator on file fd according to mode:
 * - if oflag ORed with O_CREAT, create a new allocator;
 * - else, open an existing allocator.
 */
extern int init_allocator(ALLOC * a, int fd, int oflag);
/*
 * Allocate a block large enough and copy len bytes into it.
 *
 * CRUD: create
 */
extern handle_t alloc_blk(ALLOC * a, void *buf, size_t len);
/*
 * Read the entire block content referenced by handle.
 * If buf is not large enough to place the content, a new buffer is
 * allocated and be returned.
 * Callers should check whether buf == return v, and if not,
 * buf_put() MUST be called for the return v.
 * On return, *len is set to the length of block content in bytes.
 *
 * CRUD: read
 */
extern void *read_blk(ALLOC * a, handle_t handle, void *buf, size_t * len);
/*
 * CRUD: update
 */
extern int realloc_blk(ALLOC * a, handle_t handle, void *buf, size_t len);
/*
 * Deallocate a used block.
 *
 * CRUD: delete
 */
extern int dealloc_blk(ALLOC * a, handle_t handle);

extern void *buf_get(ALLOC * a, size_t len);
extern void buf_put(ALLOC * a, void *p);
extern int cache_set(ALLOC * a, handle_t h, void *buf, size_t len);
extern void *cache_get(ALLOC * a, handle_t h, size_t * len);
extern void cache_del(ALLOC * a, handle_t h);

#endif
