#ifndef _STMT_H
#define _STMT_H

#include "table.h"
#include "record.h"

#define OP_EQ 1
#define OP_NEQ (OP_EQ << 1)
#define OP_GT (OP_NEQ << 1)
#define OP_GE (OP_GT << 1)
#define OP_LT (OP_GE << 1)
#define OP_LE (OP_LT << 1)

typedef struct {
	char attr[NAMELEN + 1];
	int op;
	colv_t operand;
	// private
	int icol;
} cond_t;

typedef struct {
	handle_t h;
	index_t *idx;
	BTreeEnum *iter;
	BTreeEnum *to;
	int end;
	int error;
	int ncond;
	cond_t conds[];
} cursor_t;

extern void _free_cursor(cursor_t * c);
extern cursor_t *init_cursor(table_t * t, cond_t * conds, int ncond);
extern int assert_cond(table_t * t, record_t * r, cond_t * cond);

#endif
