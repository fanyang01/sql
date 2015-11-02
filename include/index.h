#ifndef _INDEX_H
#define _INDEX_H

#include "alloc.h"
#include "common.h"
#include "table.h"
#include "record.h"

extern index_t *open_index(ALLOC * a, handle_t h, int type);
extern int new_index(ALLOC * a, table_t * t, const char *colname);
extern int delete_index(index_t * idx);

extern void index_set(index_t * idx, record_t * r, int i);
extern handle_t index_get(index_t * idx, colv_t * val);
extern void index_del(index_t * idx, colv_t * val);
extern int index_exist(index_t * idx, colv_t * val);

extern const void *_index_value_ptr(colv_t * val);

#define index_foreach(IDX, H, NS) \
	BTreeEnum NS##_iter, NS##_end; \
	for(EnumBegin(&NS##_iter, (IDX)), EnumEnd(&NS##_end); \
			(H = BTValue(&NS##_iter)) != 0 \
			&& IsValid(&NS##_iter) && !IsEqual(&NS##_iter, &NS##_end); \
			MoveNext(&NS##_iter))

#endif
