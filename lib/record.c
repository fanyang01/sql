#include "record.h"
#include "type.h"
#include "xerror.h"
#include <bsd/string.h>

static int validate_record(ALLOC * a, table_t * t, record_t * r);
static size_t _sizeof_record(table_t * t, record_t * r);
static void *record2b(void *buf, table_t * t, record_t * r);
static int _record_setprev(ALLOC * a, handle_t h, handle_t prev);
static int _record_setnext(ALLOC * a, handle_t h, handle_t next);
static int _list_add_record(ALLOC * a, table_t * t, record_t * r);
static int _list_del_record(ALLOC * a, table_t * t, record_t * r);

int validate_record(ALLOC * a, table_t * t, record_t * r)
{
	if (t->ncols != r->len)
		return 0;
	for (int i = 0; i < t->ncols; i++)
		if (t->cols[i].type != r->vals[i].type)
			return 0;
	for (int i = 0; i < t->ncols; i++)
		if (r->vals[i].type == TYPE_STRING)
			if (strlen(r->vals[i].value.s) >= t->cols[i].size)
				return 0;
	// TODO: validate the unique constraint
	return 1;
}

size_t _sizeof_record(table_t * t, record_t * r)
{
	size_t len = 0;

	for (int i = 0; i < t->ncols; i++)
		len += t->sizes[i];
	return 2 * 7 + len;
}

void *record2b(void *buf, table_t * t, record_t * r)
{
	void *p = buf;

	p = hdl2b(p, r->prev);
	p = hdl2b(p, r->next);
	for (int i = 0; i < r->len; i++) {
		switch (r->vals[i].type) {
		case TYPE_INT:
			p = int32tob(p, r->vals[i].value.i);
			break;
		case TYPE_FLOAT:
			p = float2b(p, r->vals[i].value.f);
			break;
		case TYPE_STRING:
			p = str2b(p, t->sizes[i], r->vals[i].value.s);
			break;
		}
	}
	return p;
}

// skip leading prev and next handle
void *record2b_skip(void *buf, table_t * t, record_t * r)
{
	void *p = buf;

	p += 2 * 7;
	for (int i = 0; i < r->len; i++) {
		switch (r->vals[i].type) {
		case TYPE_INT:
			p = int32tob(p, r->vals[i].value.i);
			break;
		case TYPE_FLOAT:
			p = float2b(p, r->vals[i].value.f);
			break;
		case TYPE_STRING:
			p = str2b(p, t->sizes[i], r->vals[i].value.s);
			break;
		}
	}
	return p;
}

int _record_setprev(ALLOC * a, handle_t h, handle_t prev)
{
	void *buf;
	size_t len = 0;
	int ret;

	if ((buf = read_blk(a, h, NULL, &len)) == NULL)
		return -1;
	hdl2b(buf, prev);
	ret = realloc_blk(a, h, buf, len);
	buf_put(a, buf);
	return ret;
}

int _record_setnext(ALLOC * a, handle_t h, handle_t next)
{
	void *buf;
	size_t len = 0;
	int ret;

	if ((buf = read_blk(a, h, NULL, &len)) == NULL)
		return -1;
	hdl2b(buf + 7, next);
	ret = realloc_blk(a, h, buf, len);
	buf_put(a, buf);
	return ret;
}

int _list_add_record(ALLOC * a, table_t * t, record_t * r)
{
	int changed = 0;
	if (r->prev != 0) {
		if (_record_setnext(a, r->prev, r->self) < 0)
			return -1;
	} else {
		t->head = r->self;
		changed = 1;
	}
	if (r->next != 0) {
		if (_record_setprev(a, r->next, r->self) < 0)
			return -1;
	} else {
		t->tail = r->self;
		changed = 1;
	}
	if (changed)
		return write_table(a, t->self, t);
	return 0;
}

int _list_del_record(ALLOC * a, table_t * t, record_t * r)
{
	int changed = 0;
	if (r->prev != 0) {
		if (_record_setnext(a, r->prev, r->next) < 0)
			return -1;
	} else {
		t->head = r->next;
		changed = 1;
	}
	if (r->next != 0) {
		if (_record_setprev(a, r->next, r->prev) < 0)
			return -1;
	} else {
		t->tail = r->prev;
		changed = 1;
	}
	if (changed)
		return write_table(a, t->self, t);
	return 0;
}

handle_t alloc_record(ALLOC * a, table_t * t, record_t * r)
{
	unsigned char *buf, *p;
	handle_t h = 0;

	if (!validate_record(a, t, r))
		return 0;
	if ((buf = buf_get(a, _sizeof_record(t, r))) == NULL)
		return 0;

	r->prev = t->tail;
	r->next = 0;
	p = record2b(buf, t, r);
	h = alloc_blk(a, buf, p - buf);
	buf_put(a, buf);
	if (h == 0)
		return 0;
	r->self = h;
	if (_list_add_record(a, t, r) < 0)
		return 0;
	return h;
}

record_t *_alloc_record(table_t * t)
{
	record_t *r;
	char *p;

	if ((r = malloc(sizeof(record_t) + t->ncols * sizeof(colv_t))) == NULL)
		return NULL;

	for (int i = 0; i < t->ncols; i++) {
		r->vals[i].type = t->cols[i].type;
		switch (t->cols[i].type) {
		case TYPE_STRING:
			if ((p = calloc(1, t->sizes[i])) == NULL) {
				preserve_errno(_free_record(r));
				return NULL;
			}
			r->vals[i].value.s = p;
		}
	}
	r->len = t->ncols;
	return r;
}

record_t *read_record(ALLOC * a, table_t * t, handle_t h)
{
	record_t *r;
	void *buf, *p;
	size_t len = 0;

	if ((r = _alloc_record(t)) == NULL)
		return NULL;
	if ((buf = read_blk(a, h, NULL, &len)) == NULL)
		goto Error;

	p = buf;
	r->prev = b2hdl(p);
	p += 7;
	r->next = b2hdl(p);
	p += 7;
	for (int i = 0; i < t->ncols; i++) {
		switch (t->cols[i].type) {
		case TYPE_INT:
			r->vals[i].value.i = b2int32(p);
			p += 4;
			break;
		case TYPE_FLOAT:
			r->vals[i].value.f = b2float(p);
			p += 4;
			break;
		case TYPE_STRING:
			strlcpy(r->vals[i].value.s, p, t->sizes[i]);
			break;
		default:
			goto Error;
		}
	}
	buf_put(a, buf);
	r->self = h;
	return r;

 Error:
	if (buf != NULL)
		preserve_errno(buf_put(a, buf));
	preserve_errno(_free_record(r));
	return NULL;
}

int update_record(ALLOC * a, table_t * t, handle_t h, record_t * r)
{
	unsigned char *buf, *p;
	size_t len = 0;
	int ret;

	if (!validate_record(a, t, r))
		return -1;
	if ((buf = read_blk(a, h, NULL, &len)) == NULL)
		return -1;

	p = record2b_skip(buf, t, r);
	ret = realloc_blk(a, h, buf, p - buf);
	buf_put(a, buf);
	return ret;
}

int delete_record(ALLOC * a, table_t * t, handle_t h)
{
	record_t *r;
	int ret = -1;

	if ((r = read_record(a, t, h)) == NULL)
		return -1;
	if (_list_del_record(a, t, r) < 0)
		goto Error;
	if (dealloc_blk(a, h) < 0)
		goto Error;
	ret = 0;
 Error:
	_free_record(r);
	return ret;

}

int clear_table(ALLOC * a, table_t * t)
{
	handle_t h;
	record_t *r;
	int ret;

	for (h = t->head; h != 0;) {
		if ((r = read_record(a, t, h)) == NULL)
			return -1;
		h = r->next;
		ret = delete_record(a, t, r->self);
		_free_record(r);
		if (ret < 0)
			return -1;
	}
	return 0;
}

void _free_record(record_t * r)
{
	for (int i = 0; i < r->len; i++)
		if (r->vals[i].type == TYPE_STRING)
			if (r->vals[i].value.s != NULL)
				free(r->vals[i].value.s);
	free(r);
}
