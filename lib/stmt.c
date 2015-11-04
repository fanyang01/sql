#include "db.h"
#include <stdlib.h>

#define INT_P_WIDTH 8
#define FLOAT_P_WIDTH 8

static int _exec_stmt(DB * db, stmt_t * stmt);
static int select_and_print(DB * db, const char *tname, cond_t * conds,
			    int ncond);
static void print_line(table_t * t);
static void print_head_line(table_t * t);
static void print_thead(table_t * t);
static void print_record(table_t * t, record_t * r);

void exec_stmt(DB * db, stmt_t * stmt)
{
	if (_exec_stmt(db, stmt) < 0) {
		if (errno != 0) {
			perror("FATAL SYSTEM ERROR");
			exit(1);
		}
		if (xerrno != 0) {
			if (xerrno < FATAL_MAX) {	// fatal error
				perror("FATAL ERROR");
				exit(2);
			}
			perror("ERROR");
			return;
		}
		fprintf(stderr, "Undetected Error\n");
		exit(3);
	}
}

int _exec_stmt(DB * db, stmt_t * stmt)
{
	switch (stmt->type) {
	case STMT_CREAT_TABLE:
		return create_table(db, stmt->table, stmt->cols, stmt->ncol);
	case STMT_DROP_TABLE:
		return drop_table(db, stmt->table);
	case STMT_CREAT_INDEX:
		return create_index(db, stmt->table, stmt->attr, stmt->index);
	case STMT_DROP_INDEX:
		return drop_index(db, stmt->index);
	case STMT_INSERT:
		return insert_into(db, stmt->table, NULL, stmt->vals,
				   stmt->nval);
	case STMT_DELETE:
		return delete_from(db, stmt->table, stmt->conds, stmt->ncond);
	case STMT_SELECT:
		return select_and_print(db, stmt->table, stmt->conds,
					stmt->ncond);
	}
	xerrno = ERR_INVSTMT;
	return -1;
}

int select_and_print(DB * db, const char *tname, cond_t * conds, int ncond)
{
	table_t *t;
	cursor_t *cur;
	record_t *r;
	int count;
	int ret = 0;
	int error, xerror;

	if ((t = db_find_table(db, tname)) == NULL) {
		xerrno = ERR_NOTABLE;
		return -1;
	}
	if ((cur = select_from(db, tname, conds, ncond)) == NULL)
		return -1;

	print_thead(t);

	count = 0;
	while ((r = select_next(db, cur)) != NULL) {
		print_record(t, r);
		free_record(r);
		count++;
	}
	if (select_error(cur)) {
		error = errno;
		xerror = xerrno;
		ret = -1;
	}
	printf("Total %d %s\n\n", count, count > 1 ? "records" : "record");
	free_cursor(cur);

	errno = error;
	xerrno = xerror;
	return ret;
}

void print_head_line(table_t * t)
{
	printf("+");
	for (int i = 0; i < t->ncols; i++) {
		switch (t->cols[i].type) {
		case TYPE_INT:
			for (int j = 0; j < INT_P_WIDTH; j++)
				printf("=");
			break;
		case TYPE_FLOAT:
			for (int j = 0; j < FLOAT_P_WIDTH; j++)
				printf("=");
			break;
		case TYPE_STRING:
			for (int j = 0; j < t->cols[i].size; j++)
				printf("=");
			break;
		}
		printf("=+");
	}
	printf("\n");
}

void print_line(table_t * t)
{
	printf("+");
	for (int i = 0; i < t->ncols; i++) {
		switch (t->cols[i].type) {
		case TYPE_INT:
			for (int j = 0; j < INT_P_WIDTH; j++)
				printf("-");
			break;
		case TYPE_FLOAT:
			for (int j = 0; j < FLOAT_P_WIDTH; j++)
				printf("-");
			break;
		case TYPE_STRING:
			for (int j = 0; j < t->cols[i].size; j++)
				printf("-");
			break;
		}
		printf("-+");
	}
	printf("\n");
}

void print_thead(table_t * t)
{
	print_line(t);

	printf("|");
	for (int i = 0; i < t->ncols; i++) {
		switch (t->cols[i].type) {
		case TYPE_INT:
			printf("%*s", INT_P_WIDTH, t->cols[i].name);
			break;
		case TYPE_FLOAT:
			printf("%*s", FLOAT_P_WIDTH, t->cols[i].name);
			break;
		case TYPE_STRING:
			printf("%*s", t->cols[i].size, t->cols[i].name);
			break;
		}
		printf(" |");
	}
	printf("\n");
	print_head_line(t);
}

void print_record(table_t * t, record_t * r)
{
	printf("|");
	for (int i = 0; i < t->ncols; i++) {
		switch (t->cols[i].type) {
		case TYPE_INT:
			printf("%*d", INT_P_WIDTH, r->vals[i].v.i);
			break;
		case TYPE_FLOAT:
			printf("%*.*f", FLOAT_P_WIDTH - 2, 2, r->vals[i].v.f);
			break;
		case TYPE_STRING:
			printf("%*s", t->cols[i].size, r->vals[i].v.s);
			break;
		}
		printf(" |");
	}
	printf("\n");
	print_line(t);
}
