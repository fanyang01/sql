#include "db.h"

cursor_t *select_from(DB * db, const char *tname, cond_t * conds, int len)
{
	table_t *t;

	if ((t = db_find_table(db, tname)) == NULL) {
		xerrno = ERR_NOTABLE;
		return NULL;
	}
	return init_cursor(t, conds, len);
}
