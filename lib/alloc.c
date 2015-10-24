#include "file.h"
#include "alloc.h"
#include <errno.h>
#include <sys/stat.h>

static off_t hdl2off(handle_t handle);
static handle_t off2hdl(off_t offset);

/*
 * handle: store as 7 bytes in NETWORK ORDER
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
ALLOC *new_allocator(int fd, int mode)
{
	ALLOC *allocator;
	struct stat st;
	int err;

	switch (mode) {
	case ALLOC_M_OPEN:
		if (fstat(fd, &st) == -1 || st.st_size == 0)
			return NULL;
		if ((allocator = calloc(sizeof(ALLOC), 1)) == NULL)
			return NULL;

		allocator->fd = fd;
		if (readat(fd, &allocator->flt,
			   ALLOC_FLT_LEN, 0) != ALLOC_FLT_LEN) {
			err = errno;
			free(allocator);
			errno = err;
			return NULL;
		}
		return allocator;
	case ALLOC_M_NEW:
		if (ftruncate(fd, 0) == -1)
			return NULL;
		if (alloc(fd, 0, ALLOC_FLT_LEN) == -1)
			return NULL;
		if ((allocator = calloc(sizeof(ALLOC), 1)) == NULL)
			return NULL;
		allocator->fd = fd;
		if (writeat(fd, &allocator->flt,
			    ALLOC_FLT_LEN, 0) != ALLOC_FLT_LEN) {
			err = errno;
			free(allocator);
			errno = err;
			return NULL;
		}
		return allocator;
	default:
		errno = EINVAL;
		return NULL;
	}
}

off_t hdl2off(handle_t handle)
{
	return ALLOC_FLT_LEN + (handle - 1) * ALLOC_ATOM_LEN;
}

handle_t off2hdl(off_t offset)
{
	if (offset < ALLOC_FLT_LEN)
		return 0;
	return (offset - ALLOC_FLT_LEN) / ALLOC_ATOM_LEN + 1;
}

handle_t byte2hdl(void *buf)
{
	unsigned char *p = buf;
	uint64_t handle = 0;

	for (int i = 0; i < 7; i++, p++) {
		handle <<= 8;
		handle |= *p;
	}
	return handle;
}
