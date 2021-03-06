#include "db.h"
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
static int _delete_table(DB * db, table_t * t);
static int _validate_cols(const col_t * cols, int ncol);
static void _sort_cols(table_t * t, char **colnames, colv_t * vals, int len);

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
		va_end(ap);

		fd = open(path, oflag, mode);
	} else {
		fd = open(path, oflag);
	}

	off_t flen = fsize(fd);

	if (fd < 0 || init_allocator(&db->a, fd, oflag) < 0)
		goto Error;

	unsigned char buf[7];

	if ((oflag & O_TRUNC) || flen == 0) {
		bzero(buf, 7);
		if (alloc_blk(&db->a, buf, 7) != 1) {
			xerrno = FATAL_BLKNO;
			goto Error;
		}
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

	if ((db = calloc(1, sizeof(DB))) == NULL) {
		xerrno = FATAL_NOMEM;
		return NULL;
	}
	if ((db->name = malloc(pathlen + 1)) == NULL) {
		free(db);
		xerrno = FATAL_NOMEM;
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
	if (ncol > MAXCOLS) {
		xerrno = ERR_TMNCOL;
		return -1;
	}
	for (int i = 0; i < ncol; i++)
		switch (cols[i].type) {
		case TYPE_INT:
		case TYPE_FLOAT:
			break;
		case TYPE_STRING:
			if (cols[i].size == 0) {
				xerrno = ERR_ZEROSLEN;
				return -1;
			}
			break;
		default:
			xerrno = ERR_INVTYPE;
			return -1;
		}
	for (int i = 0; i < ncol; i++)
		for (int j = i + 1; j < ncol; j++)
			if (strcmp(cols[i].name, cols[j].name) == 0) {
				xerrno = ERR_DPCNAME;
				return -1;
			}
	int count = 0;
	for (int i = 0; i < ncol; i++)
		if (cols[i].unique == COL_PRIMARY)
			count++;
	if (count != 1) {
		xerrno = ERR_NPRIMARY;
		return -1;
	}
	return 0;
}

int create_table(DB * db, const char *tname, const col_t * cols, int ncol)
{
	table_t *t;

	if (db_find_table(db, tname) != NULL) {
		xerrno = ERR_DPTABLE;
		return -1;
	}
	if (_validate_cols(cols, ncol) < 0)
		return -1;
	if ((t = _alloc_table(tname, ncol)) == NULL)
		return -1;

	strlcpy(t->name, tname, NAMELEN + 1);
	for (int i = 0; i < ncol; i++) {
		t->cols[i].type = cols[i].type;
		t->cols[i].unique = cols[i].unique;
		t->cols[i].size = cols[i].size;
		strlcpy(t->cols[i].name, cols[i].name, NAMELEN + 1);
	}
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
	preserve_errno(_free_table(t));
	return -1;
}

int drop_table(DB * db, const char *tname)
{
	table_t *t = db_find_table(db, tname);

	if (t == NULL) {
		xerrno = ERR_NOTABLE;
		return -1;
	}
	return _delete_table(db, t);
}

int _delete_table(DB * db, table_t * t)
{
	if (clear_table(&db->a, t) < 0)
		return -1;
	for (int i = 0; i < t->ncols; i++)
		if (t->cols[i].index != 0)
			if (delete_index(&db->a, t, i) < 0)
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

int create_index(DB * db, const char *tname, const char *colname,
		 const char *iname)
{
	table_t *t;
	int i;

	for (t = db->thead; t != NULL; t = t->next_table)
		for (int i = 0; i < t->ncols; i++)
			if (strcmp(t->cols[i].iname, iname) == 0) {
				xerrno = ERR_DPIDX;
				return -1;
			}

	if ((t = db_find_table(db, tname)) == NULL) {
		xerrno = ERR_NOTABLE;
		return -1;
	}
	if ((i = table_find_col(t, colname)) < 0) {
		xerrno = ERR_NOCOL;
		return -1;
	}
	if (new_index(&db->a, t, i) < 0)
		return -1;
	strlcpy(t->cols[i].iname, iname, NAMELEN + 1);
	return write_table(&db->a, t->self, t);
}

int drop_index(DB * db, const char *iname)
{
	table_t *t;

	for (t = db->thead; t != NULL; t = t->next_table)
		for (int i = 0; i < t->ncols; i++)
			if (strcmp(t->cols[i].iname, iname) == 0)
				return delete_index(&db->a, t, i);
	xerrno = ERR_NOIDX;
	return -1;
}

int insert_into(DB * db, const char *tname,
		char **colnames, colv_t * vals, int len)
{
	table_t *t;
	record_t *r;

	if ((t = db_find_table(db, tname)) == NULL) {
		xerrno = ERR_NOTABLE;
		return -1;
	}
	if (len != t->ncols) {
		xerrno = ERR_NCOL;
		return -1;
	}
	// validate column names and sort them
	if (colnames != NULL) {
		for (int i = 0; i < len; i++)
			for (int j = i + 1; j < len; j++)
				if (strncmp(colnames[i], colnames[j],
					    NAMELEN + 1) == 0) {
					xerrno = ERR_DPCNAME;
					return -1;
				}
		for (int i = 0; i < len; i++)
			if (table_find_col(t, colnames[i]) < 0) {
				xerrno = ERR_NOCOL;
				return -1;
			}
		_sort_cols(t, colnames, vals, len);
	}
	// validate values
	for (int i = 0; i < len; i++) {
		if (vals[i].type != t->cols[i].type) {
			xerrno = ERR_COLTYPE;
			return -1;
		}
		if (vals[i].type == TYPE_STRING)
			if (strlen(vals[i].v.s) >= t->sizes[i]) {
				xerrno = ERR_TOOLONG;
				return -1;
			}
	}

	if ((r = _alloc_record(t)) == NULL)
		return -1;
	// deep copy
	for (int i = 0; i < r->len; i++) {
		switch (t->cols[i].type) {
		case TYPE_INT:
		case TYPE_FLOAT:
			r->vals[i].v.i = vals[i].v.i;
			break;
		case TYPE_STRING:
			strlcpy(r->vals[i].v.s, vals[i].v.s, t->sizes[i]);
			break;
		}
	}

	handle_t h = alloc_record(&db->a, t, r);
	preserve_errno(_free_record(r));
	return h == 0 ? -1 : 0;
}

void _sort_cols(table_t * t, char **colnames, colv_t * vals, int len)
{

	for (int i = 0; i < len; i++) {
		int pos;
		for (pos = 0; pos < len; pos++)
			if (strcmp(t->cols[i].name, colnames[pos]) == 0)
				break;
		if (pos != i) {
			char *s = colnames[i];
			colnames[i] = colnames[pos];
			colnames[pos] = s;

			colv_t tmp = vals[i];
			vals[i] = vals[pos];
			vals[pos] = tmp;
		}
	}
}
