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
	record_t *r;
	handle_t h, hprev;
	int match;

	if ((t = db_find_table(db, tname)) == NULL) {
		xerrno = ERR_NOTABLE;
		return -1;
	}

	for (h = t->tail; h != 0; h = hprev) {
		if ((r = read_record(&db->a, t, h)) == NULL)
			return -1;
		hprev = r->prev;
		match = conds_match(t, r, conds, ncond);
		_free_record(r);
		if (!match)
			continue;
		if (delete_record(&db->a, t, h) < 0)
			return -1;
	}
	return 0;
}
