#ifndef _DB_H
#define _DB_H

#include "alloc.h"
#include "table.h"
#include "record.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef struct {
	ALLOC a;
	char *name;
	handle_t root;		// value at handle #1, point to first table metadata
	table_t *thead;
} DB;

extern DB *opendb(const char *path, int oflag, ...);
extern void closedb(DB * db);
extern int delete_table(ALLOC * a, table_t * t);
extern table_t *db_find_table(DB * db, const char *tname);

#endif
