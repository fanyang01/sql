#ifndef _INDEX_H
#define _INDEX_H

#include "alloc.h"
#include "common.h"
#include "table.h"
#include "record.h"

extern int open_index(ALLOC * a, table_t * t, int i);
extern int new_index(ALLOC * a, table_t * t, int i);
extern int delete_index(ALLOC * a, table_t * t, int i);

extern void index_set(index_t * idx, record_t * r, int i);
extern handle_t index_get(index_t * idx, colv_t * val);
extern void index_del(index_t * idx, colv_t * val, handle_t h);
extern int index_exist(index_t * idx, colv_t * val);

extern const void *_index_value_ptr(colv_t * val);

#endif
