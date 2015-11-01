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
	cols[2].unique = COL_UNIQUE;

	assert(new_table(db, "table1", cols, ncol) != 0);
	cols[1].unique = COL_NORMAL;
	assert(new_table(db, "table1", cols, ncol) != 0);
	strcpy(cols[2].name, "column3");
	assert(new_table(db, "table1", cols, ncol) == 0);

	assert(new_table(db, "table2", cols, ncol) == 0);
	assert(new_table(db, "table3", cols, ncol) == 0);

	assert((t1 = db_find_table(db, "table")) == NULL);
	assert((t2 = db_find_table(db, "table2")) != NULL);
	assert(strcmp(t2->name, "table2") == 0);
	assert((t1 = db_find_table(db, "table1")) != NULL);
	assert(strcmp(t1->name, "table1") == 0);
	assert((t3 = db_find_table(db, "table3")) != NULL);
	assert(strcmp(t3->name, "table3") == 0);

	assert(delete_table(db, t3) == 0);
	assert((t3 = db_find_table(db, "table3")) == NULL);
	assert((t1 = db_find_table(db, "table1")) != NULL);
	assert(strcmp(t1->name, "table1") == 0);
	assert((t2 = db_find_table(db, "table2")) != NULL);
	assert(strcmp(t2->name, "table2") == 0);

	assert(delete_table(db, t2) == 0);
	assert((t2 = db_find_table(db, "table2")) == NULL);
	assert((t1 = db_find_table(db, "table1")) != NULL);
	assert(strcmp(t1->name, "table1") == 0);

	closedb(db);
}
