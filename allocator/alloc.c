#include "alloc.h"
#include "file.h"
#include "xerror.h"
#include "common.h"
#include <sys/stat.h>
#include <string.h>
#include <strings.h>

static int free_blk(ALLOC * a, handle_t h, handle_t atoms);
static int use_blk(ALLOC * a, handle_t h, void *buf, size_t len);
static int flt_remove(ALLOC * a, int idx, handle_t prev, handle_t next);
static int list_setprev(ALLOC * a, handle_t x, handle_t prev);
static int list_setnext(ALLOC * a, handle_t x, handle_t next);
static handle_t flt_find(ALLOC * a, handle_t req, int *idx);
static size_t len_need(size_t len);
static handle_t len2atom(size_t len);
static int flt_idx(handle_t size);

int init_allocator(ALLOC * a, int fd, int oflag)
{
	a->fd = fd;
	if ((a->lru = calloc(LRU_NSLOT, sizeof(struct lru_node *))) == NULL) {
		xerrno = FATAL_NOMEM;
		return -1;
	}
	bzero(a->flt, FLT_LEN);
	if (oflag & O_CREAT) {
		if (ftruncate(fd, 0) == -1)
			return -1;
		if (alloc(fd, 0, FLT_LEN) == -1)
			return -1;
		if (writeat(fd, a->flt, FLT_LEN, 0) != FLT_LEN)
			return -1;
		return 0;
	}
	/* open an existing allocator */
	if (fsize(fd) <= 0) {
		xerrno = FATAL_INVDB;
		return -1;
	}
	if (readat(fd, a->flt, FLT_LEN, 0) != FLT_LEN)
		return -1;
	return 0;
}

handle_t alloc_blk(ALLOC * a, void *buf, size_t len)
{
	handle_t req = len2atom(len), h;
	int idx;

	if (len > CTBLK_MAXLONG) {
		xerrno = FATAL_BLKSIZE;
		return 0;
	}
	if ((h = flt_find(a, req, &idx)) == 0) {	// no free blocks
		off_t tail = fsize(a->fd);

		if (tail <= 0 || tail % ATOM_LEN != 0) {
			xerrno = FATAL_INVDB;
			return 0;
		}
		h = off2hdl(tail);
		if (alloc(a->fd, tail, req * ATOM_LEN) == -1)
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
	case FRBLK_SINGLE:
		atoms = 1;
		prev = b2hdl(&atom[1]);
		next = b2hdl(&atom[8]);
		// TODO: verify last byte
		break;
	case FRBLK_LONG:
		if ((atoms = read_handle(a->fd, hdl2off(h) + 16)) == 0) {
			xerrno = FATAL_BLKSIZE;
			return 0;
		}
		prev = b2hdl(&atom[1]);
		next = b2hdl(&atom[8]);
		// TODO: verify last byte
		break;
	default:
		xerrno = FATAL_BLKTAG;
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
	if (alloc(a->fd, hdl2off(h), req * ATOM_LEN) == -1)
		return 0;
	if (use_blk(a, h, buf, len) != 0)
		return 0;
	return h;
}

void *read_blk(ALLOC * a, handle_t handle, void *buf, size_t * len)
{
	if (handle == 0)
		return NULL;
	off_t offset;
	size_t need;
	unsigned char bytes[ATOM_LEN];
	int newbuf = 0, redirect = 0;

 Retry:
	offset = hdl2off(handle);
	if (readat(a->fd, bytes, 8, offset) != 8)
		return NULL;
	switch (bytes[0]) {
	case CTBLK_SHORT:
		need = (size_t) bytes[1];
		offset += 2;
		break;
	case CTBLK_LONG:
		need = (size_t) b2uint16(&bytes[1]);
		offset += 3;
		break;
	case REBLK:
		if (redirect)	// allow redirect only once
			return NULL;
		handle = b2hdl(&bytes[1]);
		redirect = 1;
		goto Retry;
	default:
		xerrno = FATAL_BLKTAG;
		return NULL;
	}
	if (buf == NULL || need > *len) {
		if ((buf = buf_get(a, need)) == NULL)
			return NULL;
		*len = need;
		newbuf = 1;
	}
	if (readat(a->fd, buf, need, offset) != need) {
		if (newbuf)
			buf_put(a, buf);
		return NULL;
	}
	return buf;
}

// TODO: merge adjacent free blocks.
// TODO: create file holes for large free block
int dealloc_blk(ALLOC * a, handle_t handle)
{
	off_t offset;
	size_t len;
	handle_t redirect, atoms;
	int redirected = 0;
	unsigned char bytes[8];

 Retry:
	offset = hdl2off(handle);
	if (readat(a->fd, bytes, 8, offset) != 8)
		return -1;
	switch (bytes[0]) {
	case CTBLK_SHORT:
		len = (size_t) bytes[1];
		break;
	case CTBLK_LONG:
		len = (size_t) b2uint16(&bytes[1]);
		break;
	case REBLK:
		if (redirected)	// allow only once redirect
			return -1;
		redirect = b2hdl(&bytes[1]);
		if (hdl2off(handle + 1) >= fsize(a->fd)) {
			if (ftruncate(a->fd, offset) == -1)
				return -1;
		} else {
			if (free_blk(a, handle, 1) != 0)
				return -1;
		}
		handle = redirect;
		redirected = 1;
		goto Retry;
	default:
		xerrno = FATAL_BLKTAG;
		return -1;
	}
	atoms = len2atom(len);
	return free_blk(a, handle, atoms);
}

int realloc_blk(ALLOC * a, handle_t handle, void *buf, size_t len)
{
	off_t offset;
	size_t olen;
	handle_t prev_handle = 0, new_handle, old_atoms, new_atoms;
	int redirected = 0;
	unsigned char bytes[16];

 Retry:
	offset = hdl2off(handle);
	if (readat(a->fd, bytes, 8, offset) != 8)
		return -1;
	switch (bytes[0]) {
	case CTBLK_SHORT:
		olen = (size_t) bytes[1];
		break;
	case CTBLK_LONG:
		olen = (size_t) b2uint16(&bytes[1]);
		break;
	case REBLK:
		if (redirected)	// allow only once redirect
			return -1;
		prev_handle = handle;
		handle = b2hdl(&bytes[1]);
		redirected = 1;
		goto Retry;
	default:
		xerrno = FATAL_BLKTAG;
		return -1;
	}
	old_atoms = len2atom(olen);
	new_atoms = len2atom(len);
	if (old_atoms == new_atoms)
		return use_blk(a, handle, buf, len);
	else if (old_atoms > new_atoms) {
		handle_t h_free = handle + new_atoms;
		if (free_blk(a, h_free, old_atoms - new_atoms) != 0)
			return -1;
		return use_blk(a, handle, buf, len);
	}

	if (prev_handle != 0) {
		if (dealloc_blk(a, handle) != 0)
			return -1;
	} else {
		if (old_atoms > 1)
			if (free_blk(a, handle + 1, old_atoms - 1) != 0)
				return -1;
		prev_handle = handle;
	}
	if ((new_handle = alloc_blk(a, buf, len)) == 0)
		return -1;
	bytes[0] = REBLK;
	hdl2b(&bytes[1], new_handle);
	bzero(&bytes[8], 8);
	if (writeat(a->fd, bytes, 16, hdl2off(prev_handle)) != 16)
		return -1;
	return 0;
}

int free_blk(ALLOC * a, handle_t h, handle_t atoms)
{
	if (hdl2off(h + atoms) >= fsize(a->fd))
		return ftruncate(a->fd, hdl2off(h));

	unsigned char buf[16];
	handle_t prev = 0, next;
	int idx;

	switch (atoms) {
	case 0:
		return -1;
	case 1:
		buf[0] = buf[15] = FRBLK_SINGLE;
		break;
	default:
		buf[0] = buf[15] = FRBLK_LONG;
		if (write_handle(a->fd, atoms, hdl2off(h) + 16) != 0)
			return -1;
	}
	if (writeat(a->fd, buf, sizeof(buf), hdl2off(h)) != sizeof(buf))
		return -1;
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
	size_t padding = len2atom(len) * ATOM_LEN - need;
	unsigned char *b;

	if ((b = buf_get(a, need + padding)) == NULL)
		return -1;

	if (len <= CTBLK_MAXSHORT) {
		b[0] = CTBLK_SHORT;
		b[1] = (unsigned char)len;
		memcpy(&b[2], buf, len);
		bzero(&b[2 + len], padding);
	} else {
		b[0] = CTBLK_LONG;
		uint16tob(&b[1], len);
		memcpy(&b[3], buf, len);
		bzero(&b[3 + len], padding);
	}
	if (writeat(a->fd, b, need + padding, hdl2off(h)) != need + padding) {
		buf_put(a, b);
		return -1;
	}
	buf_put(a, b);
	return 0;
}

int flt_remove(ALLOC * a, int idx, handle_t prev, handle_t next)
{
	if (prev == 0) {	// head
		a->flt[idx] = next;
		if (writeat(a->fd, a->flt, FLT_LEN, 0) != FLT_LEN)
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
	for (int i = flt_idx(req); i < FLT_SIZE; i++)
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
	return (len_need(len) - 1) / ATOM_LEN + 1;
}

int flt_idx(handle_t atoms)
{
	int i = 0;
	while ((atoms >>= 1) != 0)
		i++;
	return i;
}
