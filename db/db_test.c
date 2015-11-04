#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include "db.h"

int main(void)
{
	DB *db;
	col_t *cols;
	int ncol;
	table_t *t1, *t2, *t3;

	db = opendb("/tmp/xyz.db", O_RDWR | O_CREAT | O_TRUNC, 0644);
	unlink("/tmp/xyz.db");
	assert(db != NULL);
	assert(db->thead == NULL);

	ncol = 3;
	cols = calloc(3, sizeof(col_t));
	assert(cols != NULL);

	strcpy(cols[0].name, "column1");
	cols[0].type = TYPE_INT;
	cols[0].unique = COL_PRIMARY;
	strcpy(cols[1].name, "column2");
	cols[1].type = TYPE_FLOAT;
	cols[1].unique = COL_PRIMARY;
	strcpy(cols[2].name, "column2");
	cols[2].type = TYPE_STRING;
	cols[2].size = 13;
	cols[2].unique = COL_UNIQUE;

	assert(create_table(db, "table1", cols, ncol) != 0);
	cols[1].unique = COL_NORMAL;
	assert(create_table(db, "table1", cols, ncol) != 0);
	strcpy(cols[2].name, "column3");
	assert(create_table(db, "table1", cols, ncol) == 0);

	assert(create_table(db, "table2", cols, ncol) == 0);
	assert(create_table(db, "table3", cols, ncol) == 0);

	assert((t1 = db_find_table(db, "table")) == NULL);
	assert((t2 = db_find_table(db, "table2")) != NULL);
	assert(strcmp(t2->name, "table2") == 0);
	assert((t1 = db_find_table(db, "table1")) != NULL);
	assert(strcmp(t1->name, "table1") == 0);
	assert((t3 = db_find_table(db, "table3")) != NULL);
	assert(strcmp(t3->name, "table3") == 0);

	assert(drop_table(db, "table3") == 0);
	assert((t3 = db_find_table(db, "table3")) == NULL);
	assert((t1 = db_find_table(db, "table1")) != NULL);
	assert(strcmp(t1->name, "table1") == 0);
	assert((t2 = db_find_table(db, "table2")) != NULL);
	assert(strcmp(t2->name, "table2") == 0);

	assert(drop_table(db, "table2") == 0);
	assert((t2 = db_find_table(db, "table2")) == NULL);
	assert((t1 = db_find_table(db, "table1")) != NULL);
	assert(strcmp(t1->name, "table1") == 0);

	// primary key already has an index
	assert(create_index(db, "table1", "column1", "column1_index") != 0);
	assert(create_index(db, "table1", "column2", "column2_index") == 0);

	char buf[20];
	colv_t *vals = calloc(3, sizeof(colv_t));
	assert(vals != NULL);
	vals[0].type = TYPE_INT;
	vals[0].v.i = 6060;
	vals[1].type = TYPE_INT;
	vals[1].v.f = 0.25;
	vals[2].type = TYPE_STRING;
	vals[2].v.s = buf;
	strcpy(buf, "hello, world123");

	assert(insert_into(db, "table1", NULL, vals, 4) != 0);
	assert(insert_into(db, "table1", NULL, vals, 3) != 0);
	vals[1].type = TYPE_FLOAT;
	assert(insert_into(db, "table1", NULL, vals, 3) != 0);
	strcpy(buf, "hello, world");
	assert(insert_into(db, "table1", NULL, vals, 3) == 0);

	// unique constraint
	assert(insert_into(db, "table1", NULL, vals, 3) != 0);
	vals[0].v.i = 6061;
	assert(insert_into(db, "table1", NULL, vals, 3) != 0);
	strcpy(buf, "foobar");
	assert(insert_into(db, "table1", NULL, vals, 3) == 0);

	assert(drop_index(db, "column2_index") == 0);
	assert(create_index(db, "table1", "column2", "column2_index") == 0);
	assert(create_index(db, "table1", "column3", "column2_index") != 0);
	assert(create_index(db, "table1", "column3", "column3_index") == 0);

	closedb(db);
}
