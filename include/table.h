#ifndef _TABLE_H
#define _TABLE_H

#include "alloc.h"
#include "common.h"
#include "type.h"

#define TABLENAME_MAXLEN 32
#define COLNAME_MAXLEN 32
#define INDEXNAME_MAXLEN 32
#define MAXCOLS 32
#define _SCOLS_MAXLEN ((COLNAME_MAXLEN+1) * MAXCOLS)
#define _INDICES_MAXLEN ((INDEXNAME_MAXLEN+1) * MAXCOLS)
#define TABLE_MAXSIZE (3*7 + 3*2 + TABLENAME_MAXLEN + \
		_SCOLS_MAXLEN + _INDICES_MAXLEN)

typedef struct {
	handle_t next;		// next table metadata
	char *name;		// name of table
	handle_t head;		// -> first record
	char *scols;		// "col1:col2:col3:col4"
	char *indices;		// "index1::index3:"
	handle_t hxroots;	// -> [root1, 0, root3, 0]
} table_t;

#define INIT_TABLE(NAME) \
	char NAME##name[TABLENAME_MAXLEN]; \
	char NAME##scols[_SCOLS_MAXLEN]; \
	char NAME##indices[_INDICES_MAXLEN]; \
	table_t NAME = { \
		.next = 0, \
		.name = NAME##name, \
		.head = 0, \
		.scols = NAME##scols, \
		.indices = NAME##indices, \
		.hxroots = 0, \
	}

extern void table2b(void *buf, table_t * t);
extern void b2table(void *buf, table_t * t);
extern int read_table(ALLOC * a, handle_t h, table_t * t);
extern int write_table(ALLOC * a, handle_t h, table_t * t);

#endif
