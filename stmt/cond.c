#include "db.h"

int _validate_cond(table_t * t, cond_t * cond)
{
	int idx;
	if ((idx = table_find_col(t, cond->attr)) < 0) {
		xerrno = ERR_NOCOL;
		return -1;
	}
	if (t->cols[idx].type != cond->operand.type) {
		xerrno = ERR_COLTYPE;
		return -1;
	}
	switch (cond->op) {
	case OP_EQ:
		break;
	case OP_NEQ:
		break;
	case OP_GE:
		break;
	case OP_GT:
		break;
	case OP_LE:
		break;
	case OP_LT:
		break;
	default:
		xerrno = ERR_INVOP;
		return -1;
	}
	return 0;
}
