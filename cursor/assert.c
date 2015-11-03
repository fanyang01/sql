#include "db.h"
#include <stdlib.h>
#include <string.h>

static int assert_int(int rv, int op, int cv);
static int assert_float(float rv, int op, float cv);
static int assert_string(const char *rv, int op, const char *cv);

int assert_cond(table_t * t, record_t * r, cond_t * cond)
{
	int i = table_find_col(t, cond->attr);
	switch (t->cols[i].type) {
	case TYPE_INT:
		return assert_int(r->vals[i].v.i, cond->op,
				  cond->operand.v.i);
	case TYPE_FLOAT:
		return assert_float(r->vals[i].v.f, cond->op,
				    cond->operand.v.f);
	case TYPE_STRING:
		return assert_string(r->vals[i].v.s, cond->op,
				     cond->operand.v.s);
	}
	// never get here
	abort();
}

int assert_int(int rv, int op, int cv)
{
	switch (op) {
	case OP_EQ:
		return rv == cv;
	case OP_NEQ:
		return rv != cv;
	case OP_GT:
		return rv > cv;
	case OP_GE:
		return rv >= cv;
	case OP_LT:
		return rv < cv;
	case OP_LE:
		return rv <= cv;
	}
	// never get here
	abort();
}

int assert_float(float rv, int op, float cv)
{
	switch (op) {
	case OP_EQ:
		return rv == cv;
	case OP_NEQ:
		return rv != cv;
	case OP_GT:
		return rv > cv;
	case OP_GE:
		return rv >= cv;
	case OP_LT:
		return rv < cv;
	case OP_LE:
		return rv <= cv;
	}
	// never get here
	abort();
}

int assert_string(const char *rv, int op, const char *cv)
{
	int cmp = strcmp(rv, cv);
	switch (op) {
	case OP_EQ:
		return cmp == 0;
	case OP_NEQ:
		return cmp != 0;
	case OP_GT:
		return cmp > 0;
	case OP_GE:
		return cmp >= 0;
	case OP_LT:
		return cmp < 0;
	case OP_LE:
		return cmp <= 0;
	}
	// never get here
	abort();
}
