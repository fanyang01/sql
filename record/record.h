#ifndef _RECORD_H
#define _RECORD_H

#include "alloc.h"
#include "table.h"

typedef struct {
	char type;
	union {
		int i;
		float f;
		char *s;
	} value;
} colv_t;

/*
 * first 7 bytes: prev
 * next 7 bytes: next
 * rest: content
 */
typedef struct {
	handle_t self;
	int len;
	// encoded:
	handle_t prev, next;
	colv_t vals[];
} record_t;

extern record_t *_alloc_record(table_t * t);
extern void _free_record(record_t * r);
extern handle_t alloc_record(ALLOC * a, table_t * t, record_t * r);
extern record_t *read_record(ALLOC * a, table_t * t, handle_t h);
extern int update_record(ALLOC * a, table_t * t, handle_t h, record_t * r);
extern int delete_record(ALLOC * a, table_t * t, handle_t h);
extern int clear_table(ALLOC * a, table_t * t);

#endif
