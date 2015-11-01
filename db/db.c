#include "db.h"
#include "xerror.h"
#include "common.h"
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>

static DB *_alloc_db(size_t pathlen);
static void _free_db(DB * db);
static int _load_tables(DB * db);

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

int delete_table(ALLOC * a, table_t * t)
{
	if (clear_table(a, t) < 0)
		return -1;
	if (t->prev_table != NULL)
		t->prev_table->next_table = t->next_table;
	if (t->next_table != NULL)
		t->next_table->prev_table = t->prev_table;
	if (dealloc_blk(a, t->self) < 0)
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
