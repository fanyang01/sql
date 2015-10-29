#ifndef _TABLE_H
#define _TABLE_H

#include "alloc.h"
#include "common.h"
#include "type.h"

#define TABLENAME_MAXLEN 32
#define COLNAME_MAXLEN 32
#define INDEXNAME_MAXLEN 32
#define MAXCOLS 32
#define SCOLS_MAXLEN ((COLNAME_MAXLEN+2) * MAXCOLS)
#define INDICES_MAXLEN ((INDEXNAME_MAXLEN+1) * MAXCOLS)

/* +2 means two bytes used to store the length of string */
#define TABLE_MAXLEN (3*7 + TABLENAME_MAXLEN+2 + \
		SCOLS_MAXLEN+2 + INDICES_MAXLEN+2 + MAXCOLS+2)

typedef struct {
	handle_t next;		// next table metadata
	handle_t head;		// -> first record
	handle_t hxroots;	// -> [root1, 0, root3, 0]
	char *name;		// name of table
	char *scols;		// "icol1:fcol2:scol3:icol4"
	char *indices;		// "index1::index3:"
	unsigned char *sizes;	// "[4, 4, 255, 4]" - never contains 0
} table_t;

#define INIT_TABLE(NAME) \
	char NAME##_name[TABLENAME_MAXLEN]; \
	char NAME##_scols[SCOLS_MAXLEN]; \
	char NAME##_indices[INDICES_MAXLEN]; \
	unsigned char NAME##_sizes[MAXCOLS]; \
	table_t NAME = { \
		.next = 0, \
		.name = NAME##_name, \
		.head = 0, \
		.scols = NAME##_scols, \
		.indices = NAME##_indices, \
		.hxroots = 0, \
		.sizes = NAME##_sizes, \
	}

extern void *table2b(void *buf, table_t * t);
extern size_t tblsizeof(table_t * t);
extern void *b2table(void *buf, table_t * t);
extern handle_t alloc_table(ALLOC * a, table_t * t);
extern int read_table(ALLOC * a, handle_t h, table_t * t);
extern int write_table(ALLOC * a, handle_t h, table_t * t);

#endif
