#include "db.h"
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

int main(void)
{
	DB *db;
	col_t *cols;
	int ncol;
	/* table_t *t; */

	db = opendb("/tmp/xyz.db", O_RDWR | O_CREAT | O_TRUNC, 0644);
	unlink("/tmp/xyz.db");
	assert(db != NULL);

	ncol = 3;
	cols = calloc(3, sizeof(col_t));
	assert(cols != NULL);

	strcpy(cols[0].name, "int");
	cols[0].type = TYPE_INT;
	cols[0].unique = COL_PRIMARY;
	strcpy(cols[1].name, "float");
	cols[1].type = TYPE_FLOAT;
	cols[1].unique = COL_NORMAL;
	strcpy(cols[2].name, "string");
	cols[2].type = TYPE_STRING;
	cols[2].size = 20;
	cols[2].unique = COL_NORMAL;

	assert(create_table(db, "t", cols, ncol) == 0);

	char buf[20];
	colv_t *vals = calloc(3, sizeof(colv_t));
	assert(vals != NULL);
	vals[0].type = TYPE_INT;
	vals[0].v.i = 8;
	vals[1].type = TYPE_FLOAT;
	vals[1].v.f = 0.25;
	vals[2].type = TYPE_STRING;
	vals[2].v.s = buf;
	strcpy(buf, "hello, world");

	assert(insert_into(db, "t", NULL, vals, 3) == 0);
	vals[0].v.i = 6;
	assert(insert_into(db, "t", NULL, vals, 3) == 0);
	vals[0].v.i = 7;
	assert(insert_into(db, "t", NULL, vals, 3) == 0);
	vals[0].v.i = 5;
	assert(insert_into(db, "t", NULL, vals, 3) == 0);
	vals[0].v.i = 4;
	assert(insert_into(db, "t", NULL, vals, 3) == 0);
	vals[0].v.i = 2;
	assert(insert_into(db, "t", NULL, vals, 3) == 0);
	vals[0].v.i = 1;
	assert(insert_into(db, "t", NULL, vals, 3) == 0);
	vals[0].v.i = 3;
	assert(insert_into(db, "t", NULL, vals, 3) == 0);

	record_t *r;
	cursor_t *cur;
	int ncond = 0;
	int visited[8];

	cond_t *conds = calloc(5, sizeof(cond_t));
	assert(conds != NULL);

	assert((cur = select_from(db, "t", conds, ncond)) != NULL);
	bzero(visited, sizeof(visited));
	while ((r = select_next(db, cur)) != NULL) {
		assert(r->vals[0].v.i > 0 && r->vals[0].v.i <= 8);
		visited[r->vals[0].v.i - 1] += 1;
		assert(r->vals[1].v.f == 0.25);
		assert(strcmp(r->vals[2].v.s, "hello, world") == 0);
		free_record(r);
	}
	for (int i = 0; i < 8; i++)
		assert(visited[i] == 1);
	assert(select_error(cur) == 0);
	free_cursor(cur);

	strcpy(conds[0].attr, "int");
	conds[0].op = OP_NEQ;
	conds[0].operand.v.i = 8;
	ncond = 1;
	assert((cur = select_from(db, "t", conds, ncond)) == NULL);
	conds[0].operand.type = TYPE_INT;
	assert((cur = select_from(db, "t", conds, ncond)) != NULL);
	bzero(visited, sizeof(visited));
	while ((r = select_next(db, cur)) != NULL) {
		assert(r->vals[0].v.i != 8);
		visited[r->vals[0].v.i - 1] += 1;
		assert(r->vals[1].v.f == 0.25);
		assert(strcmp(r->vals[2].v.s, "hello, world") == 0);
		free_record(r);
	}
	for (int i = 0; i < 7; i++)
		assert(visited[i] == 1);

	strcpy(conds[1].attr, "int");
	conds[1].op = OP_LT;
	conds[1].operand.v.i = 5;
	conds[1].operand.type = TYPE_INT;
	ncond = 2;
	assert((cur = select_from(db, "t", conds, ncond)) != NULL);
	bzero(visited, sizeof(visited));
	// using index
	int k = 0;
	while ((r = select_next(db, cur)) != NULL) {
		assert(r->vals[0].v.i != 8 && r->vals[0].v.i < 5);
		assert(r->vals[0].v.i == ++k);
		visited[r->vals[0].v.i - 1] += 1;
		assert(r->vals[1].v.f == 0.25);
		assert(strcmp(r->vals[2].v.s, "hello, world") == 0);
		free_record(r);
	}
	for (int i = 0; i < 4; i++)
		assert(visited[i] == 1);

	closedb(db);
}
