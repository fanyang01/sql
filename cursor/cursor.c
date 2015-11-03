#include "db.h"
#include <stdint.h>
#include <stdbool.h>
#include <bsd/string.h>

static int _validate_cond(table_t * t, cond_t * conds, int ncond);
static cursor_t *_alloc_cursor(int ncond);

int _validate_cond(table_t * t, cond_t * conds, int ncond)
{
	if (ncond > MAXNCOND) {
		xerrno = ERR_TMNCOND;
		return -1;
	}
	for (int i = 0; i < ncond; i++) {
		int j;
		if ((j = table_find_col(t, conds[i].attr)) < 0) {
			xerrno = ERR_NOCOL;
			return -1;
		}
		conds[i].icol = j;
		if (t->cols[j].type != conds[i].operand.type) {
			xerrno = ERR_COLTYPE;
			return -1;
		}
		if (t->cols[j].type == TYPE_STRING)
			if (strlen(conds[i].operand.v.s) >=
			    t->sizes[conds[i].icol]) {
				xerrno = ERR_TOOLONG;
				return -1;
			}

		switch (conds[i].op) {
		case OP_EQ:
		case OP_NEQ:
		case OP_GE:
		case OP_GT:
		case OP_LE:
		case OP_LT:
			break;
		default:
			xerrno = ERR_INVOP;
			return -1;
		}
	}
	return 0;
}

cursor_t *_alloc_cursor(int ncond)
{
	cursor_t *c;
	BTreeEnum *enums;

	if ((c = calloc(1, sizeof(cursor_t) + ncond * sizeof(cond_t))) == NULL) {
		xerrno = FATAL_NOMEM;
		return NULL;
	}
	if ((enums = calloc(2, sizeof(BTreeEnum))) == NULL) {
		free(c);
		xerrno = FATAL_NOMEM;
		return NULL;
	}
	c->iter = &enums[0];
	c->to = &enums[1];
	c->ncond = ncond;
	return c;
}

void _free_cursor(cursor_t * c)
{
	free(c->iter);
	for (int i = 0; i < c->ncond; i++)
		if (c->conds[i].operand.type == TYPE_STRING)
			if (c->conds[i].operand.v.s != NULL)
				free(c->conds[i].operand.v.s);

	free(c);
}

// naive implementation
cursor_t *plan_query(table_t * t, cond_t * conds, int ncond)
{
	int mark[MAXNCOND];
	cursor_t *cur;

	if (_validate_cond(t, conds, ncond) < 0)
		return NULL;
	if ((cur = _alloc_cursor(ncond)) == NULL)
		return NULL;

	bzero(mark, sizeof(mark));
	for (int i = 0; i < ncond; i++) {
		int k = conds[i].icol;

		switch (t->cols[k].index) {
		case 0:
			break;
		default:
			mark[i] |= 0x10;
		}

		switch (conds[i].op) {
		case OP_NEQ:
			mark[i] &= ~0x10;
			break;
		case OP_EQ:
			mark[i] |= 0x8;
			break;
		}

		switch (t->cols[k].unique) {
		case COL_PRIMARY:
		case COL_UNIQUE:
			mark[i] |= 0x4;
		}

		switch (t->cols[k].type) {
		case TYPE_INT:
			mark[i] |= 0x2;
			break;
		case TYPE_FLOAT:
			mark[i] |= 0x1;
			break;
		}
	}

	int best = -1;
	for (int i = 0; i < ncond; i++) {
		if (i == 0) {
			best = 0;
			continue;
		}
		if (mark[i] > mark[best])
			best = i;
	}
	// use first best condition
	for (int i = 0; i < ncond; i++)
		if (mark[i] == mark[best]) {
			best = i;
			break;
		}

	if (best >= 0 && (mark[best] & 0x10)) {
		cur->idx = t->cols[conds[best].icol].idx;
		const void *key = _index_value_ptr(&conds[best].operand);
		switch (conds[best].op) {
		case OP_EQ:
			EnumLower_bound(cur->iter, cur->idx, key);
			EnumUpper_bound(cur->to, cur->idx, key);
			break;
		case OP_GE:
			EnumLower_bound(cur->iter, cur->idx, key);
			EnumEnd(cur->to, cur->idx);
			break;
		case OP_GT:
			EnumUpper_bound(cur->iter, cur->idx, key);
			EnumEnd(cur->to, cur->idx);
			break;
		case OP_LE:
			EnumBegin(cur->iter, cur->idx);
			EnumUpper_bound(cur->to, cur->idx, key);
			break;
		case OP_LT:
			EnumBegin(cur->iter, cur->idx);
			EnumLower_bound(cur->to, cur->idx, key);
			break;
		default:
			abort();
		}
	}
	// deep copy
	for (int i = 0, j = 0; i < ncond; i++) {
		char *p;

		if (i == best)
			continue;
		cur->conds[j].op = conds[i].op;
		cur->conds[j].operand.type = conds[i].operand.type;

		switch (t->cols[conds[i].icol].type) {
		case TYPE_INT:
		case TYPE_FLOAT:
			cur->conds[j].operand.v.i = conds[i].operand.v.i;
			break;
		case TYPE_STRING:
			if ((p = calloc(1, t->sizes[conds[i].icol])) == NULL) {
				xerrno = FATAL_NOMEM;
				goto Error;
			}
			strlcpy(p, conds[i].operand.v.s,
				t->sizes[conds[i].icol]);
			break;
		}
		cur->conds[j++].icol = conds[i].icol;
	}

	cur->ncond = (best >= 0 && (mark[best] & 0x10)) ? ncond - 1 : ncond;
	return cur;
 Error:
	_free_cursor(cur);
	return NULL;
}
