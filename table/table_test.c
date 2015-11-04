#include "alloc.h"
#include "type.h"
#include "table.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

static int equal(table_t * t1, table_t * t2)
{
	return t1->next == t2->next &&
	    t1->head == t2->head &&
	    t1->tail == t2->tail &&
	    t1->hxroots == t2->hxroots &&
	    strcmp(t1->name, t2->name) == 0 &&
	    strcmp(t1->scols, t2->scols) == 0 &&
	    strcmp(t1->indices, t2->indices) == 0 &&
	    strcmp((char *)t1->sizes, (char *)t2->sizes) == 0;
}

int main(void)
{
	INIT_TABLE(t1);
	INIT_TABLE(t2);
	unsigned char buf[TABLE_MAXLEN];

	t1.next = 123;
	t1.head = 456;
	t1.hxroots = 789;
	strncpy(t1.name, "table_name", NAMELEN);
	strncpy(t1.scols, "icol1:fcol2:scol3:icol4", SCOLS_MAXLEN);
	strncpy(t1.indices, "index1::index3:", INDICES_MAXLEN);
	strncpy((char *)t1.sizes, "\x04\x04\xFF\x04", MAXCOLS);

	assert((unsigned char *)table2b(buf, &t1) - buf == tblsizeof(&t1));
	assert((unsigned char *)b2table(buf, &t2) - buf == tblsizeof(&t1));
	assert(equal(&t1, &t2));

	table_t *t = _alloc_table("test_table", 3);
	assert(t != NULL);
	strcpy(t->cols[0].name, "column1");
	t->cols[0].type = TYPE_INT;
	t->cols[0].unique = COL_PRIMARY;
	strcpy(t->cols[1].name, "column2");
	t->cols[1].type = TYPE_INT;
	t->cols[1].unique = COL_NORMAL;
	strcpy(t->cols[2].name, "column3");
	t->cols[2].type = TYPE_STRING;
	t->cols[2].unique = COL_UNIQUE;
	t->cols[2].size = 12;

	FILE *f = tmpfile();
	int fd = fileno(f);
	ALLOC allocator, *a = &allocator;
	handle_t h;
	table_t *t3;

	assert(init_allocator(a, fd, O_CREAT | O_TRUNC) == 0);
	assert((h = alloc_table(a, t)) != 0);
	assert((t3 = read_table(a, h)) != NULL);
	assert(equal(t, t3));
	assert(table_find_col(t3, "column") == -1);
	assert(table_find_index(t3, "auto_index_test_table_column1") == 0);
	assert(table_find_col(t3, "column1") == 0);
	assert(table_find_col(t3, "column2") == 1);
	assert(table_find_col(t3, "column3") == 2);
	_free_table(t3);

	strcpy(t->name, "table");
	assert((write_table(a, h, t)) == 0);
	assert((t3 = read_table(a, h)) != NULL);
	assert(equal(t, t3));

	return 0;
}
