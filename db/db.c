#include "db.h"
#include "xerror.h"
#include "common.h"
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <bsd/string.h>

static int _load_tables(DB * db);
static DB *_alloc_db(size_t pathlen);
static void _free_db(DB * db);
static int _db_setroot(DB * db, table_t * t);
static int _table_setnext(ALLOC * a, table_t * t, table_t * next);
static int _list_add_table(DB * db, table_t * t);
static int _list_remove_table(DB * db, table_t * t);
static int _validate_cols(const col_t * cols, int ncol);

DB *opendb(const char *path, int oflag, ...)
{
	DB *db;
	size_t len;
	int mode, fd;

	len = strlen(path);
	if ((db = _alloc_db(len)) == NULL)
		return NULL;
	strcpy(db->name, path);

	if (oflag & O_CREAT) {
		va_list ap;

		va_start(ap, oflag);
		mode = va_arg(ap, int);
		fd = open(path, oflag, mode);
	} else {
		fd = open(path, oflag);
	}

	if (fd < 0 || init_allocator(&db->a, fd, oflag) < 0)
		goto Error;

	unsigned char buf[7];

	if (oflag & O_CREAT) {
		bzero(buf, 7);
		if (alloc_blk(&db->a, buf, 7) != 1)
			goto Error;
	} else {
		len = 7;
		void *p = read_blk(&db->a, 1, buf, &len);

		if (p == NULL)
			goto Error;
		if (len != 7)
			goto Error;
		db->root = b2hdl(p);
		if (p != buf)
			buf_put(&db->a, p);
	}
	if (_load_tables(db) < 0)
		goto Error;

	return db;

 Error:
	preserve_errno(_free_db(db));
	return NULL;
}

int _load_tables(DB * db)
{
	handle_t h;
	table_t *t, *prev = NULL;
	table_t **pp = &db->thead;

	for (h = db->root; h != 0; h = t->next) {
		if ((t = read_table(&db->a, h)) == NULL)
			return -1;
		t->prev_table = prev;
		*pp = prev = t;
		pp = &t->next_table;
	}
	return 0;
}

void closedb(DB * db)
{
	close(db->a.fd);
	_free_db(db);
}

DB *_alloc_db(size_t pathlen)
{
	DB *db;

	if ((db = calloc(1, sizeof(DB))) == NULL)
		return NULL;
	if ((db->name = malloc(pathlen + 1)) == NULL) {
		free(db);
		return NULL;
	}
	return db;
}

void _free_db(DB * db)
{
	table_t *t, *next;

	free(db->name);
	for (t = db->thead; t != NULL; t = next) {
		next = t->next_table;
		_free_table(t);
	}
	free(db);
}

int _db_setroot(DB * db, table_t * t)
{
	unsigned char buf[7];

	db->thead = t;
	if (t == NULL)
		db->root = 0;
	else
		db->root = t->self;
	hdl2b(buf, db->root);
	return realloc_blk(&db->a, 1, buf, sizeof(buf));
}

int _table_setnext(ALLOC * a, table_t * t, table_t * next)
{
	t->next_table = next;
	if (next == NULL)
		t->next = next->self;
	else
		t->next = 0;
	return write_table(a, t->self, t);
}

// premise: t->next_table and t->prev_table has been set properly.
int _list_add_table(DB * db, table_t * t)
{
	if (t->prev_table == NULL) {
		if (_db_setroot(db, t) < 0)
			return -1;
	} else if (_table_setnext(&db->a, t->prev_table, t) < 0) {
		return -1;
	}
	if (t->next_table != NULL)
		t->next_table->prev_table = t;
	return 0;
}

int _list_remove_table(DB * db, table_t * t)
{
	if (t->prev_table == NULL) {
		if (_db_setroot(db, t->next_table) < 0)
			return -1;
	} else if (_table_setnext(&db->a, t->prev_table, t->next_table) < 0) {
		return -1;
	}
	if (t->next_table != NULL)
		t->next_table->prev_table = t->prev_table;
	return 0;
}

int _validate_cols(const col_t * cols, int ncol)
{
	if (ncol > MAXCOLS)
		return 0;
	for (int i = 0; i < ncol; i++)
		for (int j = i + 1; j < ncol; j++)
			if (strcmp(cols[i].name, cols[j].name) == 0)
				return 0;
	int count = 0;
	for (int i = 0; i < ncol; i++)
		if (cols[i].unique == COL_PRIMARY)
			count++;
	if (count != 1)
		return 0;
	return 1;
}

int new_table(DB * db, const char *tname, const col_t * cols, int ncol)
{
	table_t *t;

	if (db_find_table(db, tname) != NULL)
		return -1;
	if (!_validate_cols(cols, ncol))
		return -1;
	if ((t = _alloc_table(tname, ncol)) == NULL)
		return -1;

	strlcpy(t->name, tname, NAMELEN + 1);
	memcpy(t->cols, cols, ncol * sizeof(col_t));
	t->next_table = db->thead;
	if (db->thead != NULL)
		t->next = db->thead->self;
	else
		t->next = 0;

	if (alloc_table(&db->a, t) == 0)
		goto Error;
	if (_list_add_table(db, t) < 0)
		goto Error;

	return 0;
 Error:
	_free_table(t);
	return -1;
}

int delete_table(DB * db, table_t * t)
{
	if (clear_table(&db->a, t) < 0)
		return -1;
	if (_list_remove_table(db, t) < 0)
		return -1;
	if (dealloc_blk(&db->a, t->self) < 0)
		return -1;
	_free_table(t);
	return 0;
}

table_t *db_find_table(DB * db, const char *tname)
{
	table_t *t;

	for (t = db->thead; t != NULL; t = t->next_table)
		if (strcmp(t->name, tname) == 0)
			return t;
	return NULL;
}
