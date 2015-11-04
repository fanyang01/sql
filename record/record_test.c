#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "table.h"
#include "record.h"

int main(void)
{
	table_t *t = _alloc_table("test_table", 3);

	strcpy(t->cols[0].name, "column1");
	t->cols[0].type = TYPE_INT;
	t->cols[0].unique = COL_PRIMARY;
	strcpy(t->cols[1].name, "column2");
	t->cols[1].type = TYPE_FLOAT;
	t->cols[1].unique = COL_NORMAL;
	strcpy(t->cols[2].name, "column3");
	t->cols[2].type = TYPE_STRING;
	t->cols[2].unique = COL_UNIQUE;
	t->cols[2].size = 13;

	FILE *f = tmpfile();
	int fd = fileno(f);
	ALLOC allocator, *a = &allocator;
	handle_t h;

	init_allocator(a, fd, O_CREAT | O_TRUNC);
	h = alloc_table(a, t);
	assert(h != 0);

	// test record operation
	record_t *r = _alloc_record(t);
	assert(r != NULL && r->len == 3);
	assert(r->vals[0].type == TYPE_INT);
	assert(r->vals[1].type == TYPE_FLOAT);
	assert(r->vals[2].type == TYPE_STRING);

	r->vals[0].v.i = 123;
	r->vals[1].v.f = 0.125;
	strcpy(r->vals[2].v.s, "hello, world");

	assert(alloc_record(a, t, r) != 0 && r->self != 0);
	assert(t->head == r->self && t->tail == r->self);

	record_t *r1 = read_record(a, t, r->self);
	assert(r1 != NULL && r1->self == r->self && r1->len == 3);
	assert(r1->prev == 0 && r1->next == 0);
	assert(r1->vals[0].type == TYPE_INT);
	assert(r1->vals[1].type == TYPE_FLOAT);
	assert(r1->vals[2].type == TYPE_STRING);

	assert(r1->vals[0].v.i == 123);
	assert(r1->vals[1].v.f == 0.125);
	assert(strcmp(r->vals[2].v.s, "hello, world") == 0);

	r1->vals[0].v.i = 124;
	strcpy(r1->vals[2].v.s, "foobar");

	assert(alloc_record(a, t, r1) != 0 && r1->self != 0);

	h = r1->self;
	_free_record(r1);
	r1 = read_record(a, t, h);
	assert(r1 != NULL);
	assert(r1->prev == r->self && r1->next == 0);
	assert(t->head == r->self && t->tail == r1->self);
	assert(r1->vals[0].v.i == 124);
	assert(r1->vals[1].v.f == 0.125);
	assert(strcmp(r1->vals[2].v.s, "foobar") == 0);

	assert(delete_record(a, t, r1->self) == 0);
	assert(read_record(a, t, r1->self) == NULL);
	assert(t->head == r->self && t->tail == r->self);

	h = r->self;
	_free_record(r);
	r = read_record(a, t, h);
	assert(r != NULL);
	assert(r->prev == 0 && r->next == 0);
	assert(r->vals[0].v.i == 123);

	r->vals[0].v.i = 125;
	assert(update_record(a, t, r->self, r) == 0);

	h = r->self;
	_free_record(r);
	r = read_record(a, t, h);
	assert(r != NULL);
	assert(r->vals[0].v.i == 125);

	assert(clear_table(a, t) == 0);
	assert(t->head == 0 && t->tail == 0);

	_free_table(t);
	_free_record(r);
	_free_record(r1);

	return 0;
}
