#include "file.h"
#include "alloc.h"
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <arpa/inet.h>

#define preserve_errno(stmt) do { \
	int _err = errno; stmt; errno = _err; \
} while(0)

extern void *buf_get(ALLOC * a, size_t len);
extern void buf_put(ALLOC * a, void *p);

static int free_blk(ALLOC * a, handle_t h, handle_t atoms);
static int use_blk(ALLOC * a, handle_t h, void *buf, size_t len);
static int flt_remove(ALLOC * a, int idx, handle_t prev, handle_t next);
static int list_setprev(ALLOC * a, handle_t x, handle_t prev);
static int list_setnext(ALLOC * a, handle_t x, handle_t next);
static handle_t flt_find(ALLOC * a, handle_t req, int *idx);
static size_t len_need(size_t len);
static handle_t len2atom(size_t len);
static int flt_idx(handle_t size);
static off_t hdl2off(handle_t handle);
static handle_t off2hdl(off_t offset);
static handle_t byte2hdl(void *buf);
static handle_t read_handle(int fd, off_t offset);
static int write_handle(int fd, handle_t h, off_t offset);

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

	switch (mode) {
	case ALLOC_M_OPEN:
		if (fsize(fd) <= 0)
			return NULL;
		if ((allocator = calloc(sizeof(ALLOC), 1)) == NULL)
			return NULL;

		allocator->fd = fd;
		if (readat(fd, &allocator->flt,
			   ALLOC_FLT_LEN, 0) != ALLOC_FLT_LEN) {
			preserve_errno(free(allocator));
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
			preserve_errno(free(allocator));
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
	int idx;

	if (len > CTBLK_MAXLONG)
		return 0;
	if ((h = flt_find(a, req, &idx)) == 0) {	// no free blocks
		off_t tail = fsize(a->fd);
		off_t need = len_need(len);
		unsigned char *b;

		if (tail <= 0)
			return 0;
		h = off2hdl(tail);
		if (alloc(a->fd, tail, req * ALLOC_ATOM_LEN) == -1)
			return 0;
		if (use_blk(a, h, buf, len) != 0)
			return 0;
		return h;
	}

	unsigned char tag;
	handle_t prev, next, atoms, h_free, atoms_free;
	unsigned char atom[16];

	if (readat(a->fd, atom, 16, hdl2off(h)) != 16)
		return 0;
	tag = atom[0];
	switch (tag) {
	case FRBLK_FLAG_SINGLE:
		atoms = 1;
		prev = byte2hdl(&atom[1]);
		next = byte2hdl(&atom[8]);
		// TODO: verify last byte
		break;
	case FRBLK_FLAG_LONG:
		if ((atoms = read_handle(a->fd, hdl2off(h) + 16)) == 0)
			return 0;
		prev = byte2hdl(&atom[1]);
		next = byte2hdl(&atom[8]);
		// TODO: verify last byte
		break;
	default:
		return 0;
	}
	if (flt_remove(a, idx, prev, next) != 0)
		return 0;
	if (atoms > req) {
		h_free = h + req;
		atoms_free = atoms - req;
		if (free_blk(a, h_free, atoms_free) != 0)
			return 0;
	}
	if (use_blk(a, h, buf, len) != 0)
		return 0;
	return h;
}

int free_blk(ALLOC * a, handle_t h, handle_t atoms)
{
	unsigned char buf[16];
	handle_t prev = 0, next;
	int idx;

	switch (atoms) {
	case 0:
		return -1;
	case 1:
		buf[0] = buf[15] = FRBLK_FLAG_SINGLE;
		break;
	default:
		buf[0] = buf[15] = FRBLK_FLAG_LONG;
		if (write_handle(a->fd, atoms, hdl2off(h) + 16) != 0)
			return -1;
	}
	idx = flt_idx(atoms);
	next = a->flt[idx];
	a->flt[idx] = h;
	if (list_setprev(a, h, prev) != 0)
		return -1;
	if (list_setnext(a, h, next) != 0)
		return -1;
	return list_setprev(a, next, h);
}

int use_blk(ALLOC * a, handle_t h, void *buf, size_t len)
{
	size_t need = len_need(len);
	unsigned char *b;

	if ((b = buf_get(a, need)) == NULL)
		return -1;

	if (len <= CTBLK_MAXSHORT) {
		b[0] = CTBLK_FLAG_SHORT;
		b[1] = (unsigned char)len;
		memcpy(&b[2], buf, len);
	} else {
		b[0] = CTBLK_FLAG_LONG;
		*((uint16_t *) (&b[1])) = htons((uint16_t) len);
		memcpy(&b[3], buf, len);
	}
	if (writeat(a->fd, b, need, hdl2off(h)) != need) {
		preserve_errno(buf_put(a, b));
		return -1;
	}
	buf_put(a, b);
	return 0;
}

int flt_remove(ALLOC * a, int idx, handle_t prev, handle_t next)
{
	if (prev == 0) {	// head
		a->flt[idx] = next;
		if (writeat(a->fd, a->flt, ALLOC_FLT_LEN, 0) != ALLOC_FLT_SIZE)
			return -1;
		return list_setprev(a, next, 0);
	}
	if (list_setprev(a, next, prev) != 0)
		return -1;
	return list_setnext(a, prev, next);
}

int list_setprev(ALLOC * a, handle_t x, handle_t prev)
{
	if (x == 0)
		return 0;
	return write_handle(a->fd, prev, hdl2off(x) + 1);
}

int list_setnext(ALLOC * a, handle_t x, handle_t next)
{
	if (x == 0)
		return 0;
	return write_handle(a->fd, next, hdl2off(x) + 8);
}

handle_t flt_find(ALLOC * a, handle_t req, int *idx)
{
	handle_t h = 0;
	for (int i = flt_idx(req); i < ALLOC_FLT_SIZE; i++)
		if (a->flt[i] != 0) {
			h = a->flt[i];
			*idx = i;
		}
	return h;
}

size_t len_need(size_t len)
{
	return (len > CTBLK_MAXSHORT ? len + 3 : len + 2);
}

handle_t len2atom(size_t len)
{
	return (len_need(len) - 1) / ALLOC_ATOM_LEN + 1;
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

int write_handle(int fd, handle_t h, off_t offset)
{
	unsigned char s[8];

	*((uint64_t *) (&s[0])) = htobe64(h);

	if (writeat(fd, s, 7, offset) != 7)
		return -1;
	return 0;
}
