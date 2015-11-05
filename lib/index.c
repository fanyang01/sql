#include "table.h"
#include "record.h"
#include "xerror.h"
#include <stdlib.h>
#include <strings.h>

static CMP _collate_of(int type);
static int _is_unique_col(int flag);

CMP _collate_of(int type)
{
	switch (type) {
	case TYPE_INT:
		return cmpInt;
	case TYPE_FLOAT:
		return cmpFloat;
	case TYPE_STRING:
		return cmpStr;
	}
	abort();
}

int _is_unique_col(int flag)
{
	switch (flag) {
	case COL_PRIMARY:
	case COL_UNIQUE:
		return 1;
		break;
	case COL_NORMAL:
		return 0;
	}
	abort();
}

const void *_index_value_ptr(colv_t * val)
{
	switch (val->type) {
	case TYPE_INT:
		return &(val->v.i);
	case TYPE_FLOAT:
		return &(val->v.f);
	case TYPE_STRING:
		return val->v.s;
	}
	abort();
}

int open_index(ALLOC * a, table_t * t, int i)
{
	index_t *idx = calloc(1, sizeof(index_t));

	if (idx == NULL) {
		xerrno = FATAL_NOMEM;
		return -1;
	}
	OpenBTree(idx, a, _is_unique_col(t->cols[i].unique),
		  t->cols[i].size,
		  _collate_of(t->cols[i].type), t->cols[i].index);
	t->cols[i].idx = idx;
	return 0;
}

int new_index(ALLOC * a, table_t * t, int i)
{
	index_t *idx;
	handle_t root, h, next;

	if (t->cols[i].index != 0) {
		xerrno = ERR_DPIDX;
		return -1;
	}
	idx = calloc(1, sizeof(index_t));
	if (idx == NULL) {
		xerrno = FATAL_NOMEM;
		return -1;
	}

	if ((root = CreateBTree(idx, a, _is_unique_col(t->cols[i].unique),
				t->cols[i].size,
				_collate_of(t->cols[i].type))) == 0)
		goto Error;

	// index records already in table
	record_t *r;
	for (h = t->head; h != 0; h = next) {
		if ((r = read_record(a, t, h)) == NULL)
			goto Error;
		next = r->next;
		SetKey(idx, _index_value_ptr(&r->vals[i]), r->self);
		_free_record(r);
	}

	t->cols[i].index = root;
	t->cols[i].idx = idx;
	// caller must write table
	return 0;
 Error:
	preserve_errno(free(idx));
	return -1;
}

int delete_index(ALLOC * a, table_t * t, int i)
{
	ClearBTree(t->cols[i].idx);
	free(t->cols[i].idx);
	t->cols[i].idx = NULL;
	t->cols[i].index = 0;
	bzero(t->cols[i].iname, NAMELEN + 1);
	return write_table(a, t->self, t);
}

void index_set(index_t * idx, record_t * r, int i)
{
	SetKey(idx, _index_value_ptr(&r->vals[i]), r->self);
}

handle_t index_get(index_t * idx, colv_t * val)
{
	return GetKey(idx, _index_value_ptr(val));
}

void index_del(index_t * idx, colv_t * val, handle_t h)
{
	DeleteKey(idx, _index_value_ptr(val), h);
}

int index_exist(index_t * idx, colv_t * val)
{
	return index_get(idx, val) != 0;
}
