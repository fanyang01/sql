#ifndef _TABLE_H
#define _TABLE_H

#include "common.h"

typedef struct {
	handle_t next;		// next table metadata
	char *name;		// name of table
	handle_t hhead;		// -> head -> first record
	char *scols;		// "col1:col2:col3:col4"
	char *indices;		// "index1::index3:"
	handle_t hxroots;	// -> [root1, 0, root3, 0]
} table_meta;

#endif
