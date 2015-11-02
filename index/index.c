#include "table.h"
#include "record.h"
#include "xerror.h"
#include <stdlib.h>

const void *_index_value_ptr(colv_t * val)
{
	switch (val->type) {
	case TYPE_INT:
		return &(val->value.i);
	case TYPE_FLOAT:
		return &(val->value.f);
	case TYPE_STRING:
		return val->value.s;
	}
	abort();
}

index_t *open_index(ALLOC * a, handle_t h, int type)
{
	index_t *idx = calloc(1, sizeof(index_t));

	if (idx == NULL) {
		xerrno = FATAL_NOMEM;
		return NULL;
	}

	switch (type) {
	case TYPE_INT:
		OpenBTree(idx, a, cmpInt, h);
		break;
	case TYPE_FLOAT:
		OpenBTree(idx, a, cmpFloat, h);
		break;
	case TYPE_STRING:
		OpenBTree(idx, a, cmpStr, h);
		break;
	}
	return idx;
}

// colname must be validated.
int new_index(ALLOC * a, table_t * t, const char *colname)
{
	int i;
	index_t *idx;
	CMP collate;
	handle_t root, h, next;

	if ((i = table_find_col(t, colname)) < 0) {
		xerrno = ERR_NOCOL;
		return -1;
	}
	if (t->cols[i].index != 0) {
		xerrno = ERR_DPIDX;
		return -1;
	}
	idx = calloc(1, sizeof(index_t));
	if (idx == NULL) {
		xerrno = FATAL_NOMEM;
		return -1;
	}
	switch (t->cols[i].type) {
	case TYPE_INT:
		collate = cmpInt;
		break;
	case TYPE_FLOAT:
		collate = cmpFloat;
		break;
	case TYPE_STRING:
		collate = cmpStr;
		break;
	}
	if ((root = CreateBTree(idx, a, collate)) == 0)
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
	return 0;
 Error:
	preserve_errno(free(idx));
	return -1;
}

int delete_index(index_t * idx)
{
	ClearBTree(idx);
	free(idx);
	return 0;
}

void index_set(index_t * idx, record_t * r, int i)
{
	SetKey(idx, _index_value_ptr(&r->vals[i]), r->self);
}

handle_t index_get(index_t * idx, colv_t * val)
{
	return GetKey(idx, _index_value_ptr(val));
}

void index_del(index_t * idx, colv_t * val)
{
	DeleteKey(idx, _index_value_ptr(val));
}

int index_exist(index_t * idx, colv_t * val)
{
	return index_get(idx, val) != 0;
}
