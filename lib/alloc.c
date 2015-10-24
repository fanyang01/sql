#include "file.h"
#include "alloc.h"
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <arpa/inet.h>

extern void *buf_get(ALLOC * a, size_t len);
extern void buf_put(ALLOC * a, void *p);

static handle_t flt_find(ALLOC * a, handle_t req);
static size_t len_act(size_t len);
static handle_t len2atom(size_t len);
static int flt_idx(handle_t size);
static off_t hdl2off(handle_t handle);
static handle_t off2hdl(off_t offset);
static handle_t byte2hdl(void *buf);
static handle_t read_handle(int fd, off_t offset);

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
	int err;

	switch (mode) {
	case ALLOC_M_OPEN:
		if (fsize(fd) <= 0)
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

handle_t alloc_blk(ALLOC * a, void *buf, size_t len)
{
	handle_t req = len2atom(len), h;
	int err = 0;

	if (len > CTBLK_MAXLONG)
		return 0;
	if ((h = flt_find(a, req)) == 0) {
		off_t size = fsize(a->fd);
		off_t actlen = len_act(size);
		unsigned char *b;

		if (size <= 0)
			return 0;
		h = off2hdl(size);
		if ((b = buf_get(a, actlen)) == NULL)
			return 0;

		if (len <= CTBLK_MAXSHORT) {
			b[0] = CTBLK_FLAG_SHORT;
			b[1] = (unsigned char)len;
			memcpy(&b[2], buf, len);
		} else {
			b[0] = CTBLK_FLAG_LONG;
			*((uint16_t *) (&b[1])) = htons((uint16_t) len);
			memcpy(&b[3], buf, len);
		}
		if (alloc(a->fd, size, actlen) == -1) {
			err = errno;
			buf_put(a, b);
			errno = err;
			return 0;
		}
		if (writeat(a->fd, b, actlen, size) != actlen) {
			err = errno;
			buf_put(a, b);
			errno = err;
			return 0;
		}
		return h;
	}
}

handle_t flt_find(ALLOC * a, handle_t req)
{
	for (int i = flt_idx(req); i < ALLOC_FLT_SIZE; i++)
		if (a->flt[i] != 0)
			return a->flt[i];
	return 0;
}

size_t len_act(size_t len)
{
	return (len > CTBLK_MAXSHORT ? len + 3 : len + 2);
}

handle_t len2atom(size_t len)
{
	return (len_act(len) - 1) / ALLOC_ATOM_LEN + 1;
}

int flt_idx(handle_t size)
{
	int i = 0;
	while ((size >>= 1) != 0)
		i++;
	return i;
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

handle_t read_handle(int fd, off_t offset)
{
	unsigned char s[7];

	if (readat(fd, s, 7, offset) != 7)
		return 0;
	return byte2hdl(s);
}
