#include "db.h"

cursor_t *select_from(DB * db, const char *tname, cond_t * conds, int ncond)
{
	table_t *t;

	if ((t = db_find_table(db, tname)) == NULL) {
		xerrno = ERR_NOTABLE;
		return NULL;
	}
	return init_cursor(t, conds, ncond);
}

record_t *select_next(DB * db, cursor_t * cursor)
{
	return cursor_next(&db->a, cursor);
}

int select_error(cursor_t * cursor)
{
	return cursor_is_error(cursor);
}

void free_cursor(cursor_t * cursor)
{
	_free_cursor(cursor);
}

void free_record(record_t * r)
{
	_free_record(r);
}

int delete_from(DB * db, const char *tname, cond_t * conds, int ncond)
{
	table_t *t;
	cursor_t *cur;
	record_t *r;
	int ret = -1;

	if ((t = db_find_table(db, tname)) == NULL) {
		xerrno = ERR_NOTABLE;
		return -1;
	}
	if ((cur = init_cursor(t, conds, ncond)) == NULL)
		return -1;

	while ((r = cursor_next(&db->a, cur)) != NULL) {
		if (delete_record(&db->a, t, r->self) < 0)
			goto Error;
		_free_record(r);
	}
	if (cursor_is_error(cur))
		goto Error;
	ret = 0;
 Error:
	_free_cursor(cur);
	return ret;
}
