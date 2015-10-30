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

typedef struct {
	int len;
	colv_t vals[];
} record_t;

extern handle_t alloc_record(ALLOC * a, table_t * t, record_t * r);
extern record_t *read_record(ALLOC * a, table_t * t, handle_t h);
extern int update_record(ALLOC * a, table_t * t, handle_t h, record_t * r);
extern void free_record(record_t * r);

#endif
