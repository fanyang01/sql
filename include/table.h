#ifndef _TABLE_H
#define _TABLE_H

#include "alloc.h"
#include "common.h"
#include "type.h"

#define NAMELEN 32
#define MAXCOLS 32
#define SCOLS_MAXLEN ((NAMELEN+3) * MAXCOLS)
#define INDICES_MAXLEN ((NAMELEN+1) * MAXCOLS)

/* +2 means two bytes used to store the length of string */
#define TABLE_MAXLEN (3*7 + NAMELEN+2 + \
		SCOLS_MAXLEN+2 + INDICES_MAXLEN+2 + MAXCOLS)

typedef struct {
	char name[NAMELEN + 1];
	char iname[NAMELEN + 1];
	char type, unique;
	unsigned char size;
	handle_t index;
} col_t;

typedef union {
	int i;
	float f;
	char *s;
} colv_t;

typedef struct table {
	handle_t next;		// next table metadata
	handle_t head;		// -> first record
	handle_t hxroots;	// -> [root1, 0, root3, 0]
	char *name;		// name of table
	char *scols;		// "ipcol1:fucol2:sncol3:incol4"
	char *indices;		// "index1::index3:"
	unsigned char *sizes;	// "[4, 4, 255, 4]" - never contains 0
	/* not encoded */
	int ncols;
	col_t *cols;
	struct table *next_table;
	struct table *prev_table;
} table_t;

#define INIT_TABLE(NAME) \
	char NAME##_name[NAMELEN]; \
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
		.ncols = 0, \
		.cols = NULL, \
		.next_table = NULL, \
		.prev_table = NULL, \
	}

// only encoding/decoding fields involved
extern void *table2b(void *buf, table_t * t);
extern size_t tblsizeof(table_t * t);
extern void *b2table(void *buf, table_t * t);

extern table_t *_alloc_table(const char *name, int ncols);
extern void _free_table(table_t * t);

extern handle_t alloc_table(ALLOC * a, table_t * t);
extern table_t *read_table(ALLOC * a, handle_t h);
extern int write_table(ALLOC * a, handle_t h, table_t * t);

#endif
