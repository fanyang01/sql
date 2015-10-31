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
	char attr[NAMELEN];
	int op;
	colv_t operand;
} cond_t;

#define STMT_CREAT_TABLE 1
#define STMT_DROP_TABLE 2
#define STMT_INSERT 3
#define STMT_SELECT 4
#define STMT_DELETE 5
#define STMT_CREAT_INDEX 6
#define STMT_DROP_INDEX 7

typedef struct {
	int type;		// statement type
	char table[NAMELEN];	// table name
	char index[NAMELEN];	// index name - create/drop index
	char attr[NAMELEN];	// attribute name - create/drop index
	int ncond;
	cond_t *conds;		// array of conditions - select/delete
	int ncol;
	col_t *cols;		// array of columns - create table
	int nval;
	colv_t *vals;		// array of column values - insert
} stmt_t;

#endif
