#include "db.h"
#include <stdlib.h>

#define INT_P_WIDTH 10
#define FLOAT_P_WIDTH 8

static int _exec_stmt(DB * db, stmt_t * stmt);
static int select_and_print(DB * db, const char *tname,
			    col_t * cols, int ncol, cond_t * conds, int ncond);
static int _max(int a, int b);
static void print_head_line(col_t * cols, int ncol, int width[]);
static void print_line(col_t * cols, int ncol, int width[]);
static void print_thead(col_t * cols, int ncol, int width[]);
static void print_record(table_t * t, col_t * cols, int ncol, record_t * r,
			 int width[]);

void exec_stmt(DB * db, stmt_t * stmt)
{
	if (_exec_stmt(db, stmt) < 0) {
		if (xerrno != 0) {
			if (xerrno < FATAL_MAX) {	// fatal error
				perror("FATAL ERROR");
				exit(2);
			}
			perror("ERROR");
			return;
		}
		if (errno != 0) {
			perror("FATAL SYSTEM ERROR");
			exit(1);
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
		return select_and_print(db, stmt->table, stmt->cols, stmt->ncol,
					stmt->conds, stmt->ncond);
	}
	xerrno = ERR_INVSTMT;
	return -1;
}

static int select_and_print(DB * db, const char *tname,
			    col_t * cols, int ncol, cond_t * conds, int ncond)
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
	for (int i = 0; i < ncol; i++) {
		int k;
		if ((k = table_find_col(t, cols[i].name)) < 0) {
			xerrno = ERR_NOCOL;
			return -1;
		}
		cols[i].type = t->cols[k].type;
		cols[i].size = t->cols[k].size;
	}

	if (ncol == 0) {
		ncol = t->ncols;
		cols = t->cols;
	}

	int *width;

	if ((width = calloc(ncol, sizeof(int))) == NULL) {
		perror("out of memory");
		return -1;
	}

	for (int i = 0; i < ncol; i++) {
		int k = table_find_col(t, cols[i].name);
		switch (t->cols[k].type) {
		case TYPE_INT:
			width[i] =
			    _max(INT_P_WIDTH, strlen(t->cols[k].name) + 1);
			break;
		case TYPE_FLOAT:
			width[i] =
			    _max(FLOAT_P_WIDTH, strlen(t->cols[k].name) + 1);
			break;
		case TYPE_STRING:
			width[i] =
			    _max(t->cols[k].size, strlen(t->cols[k].name) + 1);
			break;
		}
	}

	if ((cur = select_from(db, tname, conds, ncond)) == NULL) {
		preserve_errno(free(width));
		return -1;
	}

	print_thead(cols, ncol, width);

	count = 0;
	while ((r = select_next(db, cur)) != NULL) {
		print_record(t, cols, ncol, r, width);
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
	free(width);
	errno = error;
	xerrno = xerror;
	return ret;
}

int _max(int a, int b)
{
	return a > b ? a : b;
}

void print_head_line(col_t * cols, int ncol, int width[])
{
	printf("+");
	for (int i = 0; i < ncol; i++) {
		switch (cols[i].type) {
		case TYPE_INT:
			for (int j = 0; j < width[i]; j++)
				printf("=");
			break;
		case TYPE_FLOAT:
			for (int j = 0; j < width[i]; j++)
				printf("=");
			break;
		case TYPE_STRING:
			for (int j = 0; j < width[i]; j++)
				printf("=");
			break;
		}
		printf("=+");
	}
	printf("\n");
}

void print_line(col_t * cols, int ncol, int width[])
{
	printf("+");
	for (int i = 0; i < ncol; i++) {
		switch (cols[i].type) {
		case TYPE_INT:
			for (int j = 0; j < width[i]; j++)
				printf("-");
			break;
		case TYPE_FLOAT:
			for (int j = 0; j < width[i]; j++)
				printf("-");
			break;
		case TYPE_STRING:
			for (int j = 0; j < width[i]; j++)
				printf("-");
			break;
		}
		printf("-+");
	}
	printf("\n");
}

void print_thead(col_t * cols, int ncol, int width[])
{
	print_head_line(cols, ncol, width);
	printf("|");
	for (int i = 0; i < ncol; i++) {
		switch (cols[i].type) {
		case TYPE_INT:
			printf("%*s", width[i], cols[i].name);
			break;
		case TYPE_FLOAT:
			printf("%*s", width[i], cols[i].name);
			break;
		case TYPE_STRING:
			printf("%*s", width[i], cols[i].name);
			break;
		}
		printf(" |");
	}
	printf("\n");
	print_head_line(cols, ncol, width);
}

void print_record(table_t * t, col_t * cols, int ncol, record_t * r,
		  int width[])
{
	if (ncol == 0) {
		cols = t->cols;
		ncol = t->ncols;
	}
	printf("|");
	for (int i = 0; i < ncol; i++) {
		int k = table_find_col(t, cols[i].name);
		switch (cols[i].type) {
		case TYPE_INT:
			printf("%*d", width[i], r->vals[k].v.i);
			break;
		case TYPE_FLOAT:
			printf("%*.*f", width[i], 2, r->vals[k].v.f);
			break;
		case TYPE_STRING:
			printf("%*s", width[i], r->vals[k].v.s);
			break;
		}
		printf(" |");
	}
	printf("\n");
	print_line(cols, ncol, width);
}

void show_tables(DB * db)
{
	for (table_t * t = db->thead; t != NULL; t = t->next_table)
		printf("%s\t", t->name);
	printf("\n");
}

void show_indices(DB * db)
{
	for (table_t * t = db->thead; t != NULL; t = t->next_table)
		for (int i = 0; i < t->ncols; i++)
			if (t->cols[i].index != 0)
				printf("%s: %s(%s)\t", t->cols[i].iname,
				       t->name, t->cols[i].name);
	printf("\n");
}

void show_table_info(DB * db)
{
	printf("\n");
	for (table_t * t = db->thead; t != NULL; t = t->next_table) {
		printf("TABLE %s (\n", t->name);
		for (int i = 0; i < t->ncols; i++) {
			printf("\t%s\t", t->cols[i].name);
			switch (t->cols[i].type) {
			case TYPE_INT:
				printf("int");
				break;
			case TYPE_FLOAT:
				printf("float");
				break;
			case TYPE_STRING:
				printf("char(%d)", t->cols[i].size);
				break;
			}
			switch (t->cols[i].unique) {
			case COL_PRIMARY:
				printf(", PRIMARY KEY (%s)", t->cols[i].name);
				break;
			case COL_UNIQUE:
				printf(" UNIQUE");
				break;
			}
			if (i != t->ncols - 1)
				printf(",");
			printf("\n");
		}
		printf(")\n\n");
	}
}
