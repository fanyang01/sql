#ifndef _RECORD_H
#define _RECORD_H

typedef struct {
	colv_t *cols;
	int ncols;
} record_t;

record_t *read_row(DB * db, handle_t h, col_t * cols, int ncols);
int update_row(DB * db, handle_t h, col_t * cols, record_t * r);
void free_record(col_t * cols, record_t * r);

#endif
