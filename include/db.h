#ifndef _DB_H
#define _DB_H

#include "xerror.h"
#include "alloc.h"
#include "type.h"
#include "common.h"
#include "table.h"
#include "record.h"
#include "index.h"
#include "cursor.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define STMT_CREAT_TABLE 1
#define STMT_DROP_TABLE 2
#define STMT_INSERT 3
#define STMT_SELECT 4
#define STMT_DELETE 5
#define STMT_CREAT_INDEX 6
#define STMT_DROP_INDEX 7

#define MAXNCOND 32

typedef struct {
	int type;		// statement type
	char table[NAMELEN];	// table name
	char index[NAMELEN];	// index name - create/drop index
	char attr[NAMELEN];	// attribute name - create/drop index
	int ncond;
	cond_t *conds;		// array of conditions - select/delete
	int ncol;
	col_t *cols;		// array of columns - create table
	int nval;
	colv_t *vals;		// array of column values - insert
} stmt_t;

typedef struct {
	ALLOC a;
	char *name;
	handle_t root;		// v at handle #1, point to first table metadata
	table_t *thead;
} DB;

extern DB *opendb(const char *path, int oflag, ...);
extern void closedb(DB * db);

extern int create_table(DB * db, const char *tname,
			const col_t * cols, int ncol);
extern int drop_table(DB * db, const char *tname);

extern int create_index(DB * db, const char *tname, const char *colname,
			const char *iname);
extern int drop_index(DB * db, const char *iname);

extern int insert_into(DB * db, const char *tname,
		       char **colnames, colv_t * vals, int len);

extern table_t *db_find_table(DB * db, const char *tname);

#endif
