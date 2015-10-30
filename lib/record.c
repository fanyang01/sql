#include "record.h"
#include "type.h"
#include "xerror.h"
#include <bsd/string.h>

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
	return len;
}

void *record2b(void *buf, table_t * t, record_t * r)
{
	void *p = buf;
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

handle_t alloc_record(ALLOC * a, table_t * t, record_t * r)
{
	unsigned char *buf, *p;
	handle_t h = 0;

	if (!validate_record(a, t, r))
		return 0;
	if ((buf = buf_get(a, _sizeof_record(t, r))) == NULL)
		return 0;

	p = record2b(buf, t, r);
	h = alloc_blk(a, buf, p - buf);
	buf_put(a, buf);
	return h;
}

record_t *read_record(ALLOC * a, table_t * t, handle_t h)
{
	record_t *r;
	void *buf, *p;
	char *dst;
	size_t len = 0;

	if ((r = malloc(sizeof(record_t) + t->ncols * sizeof(colv_t))) == NULL)
		return NULL;
	if ((buf = read_blk(a, h, NULL, &len)) == NULL)
		goto Error;

	p = buf;
	for (int i = 0; i < t->ncols; i++) {
		r->vals[i].type = t->cols[i].type;
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
			dst = malloc(t->sizes[i]);
			if (dst == NULL)
				goto Error;
			strlcpy(dst, p, t->sizes[i]);
			r->vals[i].value.s = dst;
			break;
		default:
			goto Error;
		}
	}
	buf_put(a, buf);
	return r;

 Error:
	if (buf != NULL)
		preserve_errno(buf_put(a, buf));
	preserve_errno(free(r));
	return NULL;
}

int update_record(ALLOC * a, table_t * t, handle_t h, record_t * r)
{
	unsigned char *buf, *p;
	int ret;

	if (!validate_record(a, t, r))
		return 0;
	if ((buf = buf_get(a, _sizeof_record(t, r))) == NULL)
		return 0;

	p = record2b(buf, t, r);
	ret = realloc_blk(a, h, buf, p - buf);
	buf_put(a, buf);
	return ret;
}

void free_record(record_t * r)
{
	for (int i = 0; i < r->len; i++)
		if (r->vals[i].type == TYPE_STRING)
			free(r->vals[i].value.s);
	free(r);
}
