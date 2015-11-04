#include "table.h"
#include "record.h"
#include "common.h"
#include "alloc.h"
#include "type.h"
#include "xerror.h"
#include "index.h"
#include <bsd/string.h>
#include <strings.h>

// use t->scols
static int _table_ncols(table_t * t);
static int _table_cols_unmarshal(ALLOC * a, table_t * t);
static int _table_cols_marshal(ALLOC * a, table_t * t);

void *table2b(void *buf, table_t * t)
{
	unsigned char *p = buf;
	int ncols = _table_ncols(t);

	p = hdl2b(p, t->next);
	p = hdl2b(p, t->head);
	p = hdl2b(p, t->tail);
	p = hdl2b(p, t->hxroots);
	p = vstr2b(p, t->name);
	p = vstr2b(p, t->scols);
	p = vstr2b(p, t->indices);
	bcopy(t->sizes, p, ncols);
	p += ncols;
	return p;
}

void *b2table(void *buf, table_t * t)
{
	unsigned char *p = buf;
	int ncols;

	t->next = b2hdl(p);
	p += 7;
	t->head = b2hdl(p);
	p += 7;
	t->tail = b2hdl(p);
	p += 7;
	t->hxroots = b2hdl(p);
	p += 7;
	p = b2vstr(p, t->name);
	p = b2vstr(p, t->scols);
	p = b2vstr(p, t->indices);

	ncols = _table_ncols(t);
	bcopy(p, t->sizes, ncols);
	p += ncols;
	return p;
}

size_t tblsizeof(table_t * t)
{
	return 7 + 7 + 7 + 7 + vstrsizeof(t->name) + vstrsizeof(t->scols) +
	    vstrsizeof(t->indices) + _table_ncols(t);
}

table_t *_alloc_table(const char *name, int ncols)
{
	table_t *t;

	if ((t = calloc(1, sizeof(table_t))) == NULL)
		return NULL;
	if ((t->name = calloc(1, strlen(name) + 1)) == NULL)
		goto Error;
	strcpy(t->name, name);

	if ((t->scols = calloc(1, (NAMELEN + 3) * ncols)) == NULL)
		goto Error;
	if ((t->indices = calloc(1, (NAMELEN + 1) * ncols)) == NULL)
		goto Error;
	if ((t->sizes = calloc(1, ncols)) == NULL)
		goto Error;

	t->ncols = ncols;
	if ((t->cols = calloc(ncols, sizeof(col_t))) == NULL)
		goto Error;
	return t;
 Error:
	preserve_errno(_free_table(t));
	return NULL;
}

void _free_table(table_t * t)
{
	if (t->name != NULL)
		free(t->name);
	if (t->scols != NULL)
		free(t->scols);
	if (t->indices != NULL)
		free(t->indices);
	if (t->sizes != NULL)
		free(t->sizes);
	if (t->cols != NULL) {
		for (int i = 0; i < t->ncols; i++)
			if (t->cols[i].idx != NULL)
				free(t->cols[i].idx);
		free(t->cols);
	}
}

table_t *read_table(ALLOC * a, handle_t h)
{
	INIT_TABLE(tmp);
	table_t *t;
	unsigned char buf[TABLE_MAXLEN];
	size_t len = TABLE_MAXLEN;
	void *p;

	if ((p = read_blk(a, h, buf, &len)) == NULL)
		return NULL;
	b2table(p, &tmp);
	tmp.ncols = _table_ncols(&tmp);
	if ((t = _alloc_table(tmp.name, tmp.ncols)) == NULL)
		return NULL;
	b2table(p, t);
	if (p != buf)
		buf_put(a, p);

	t->self = h;
	if (_table_cols_unmarshal(a, t) < 0) {
		preserve_errno(_free_table(t));
		return NULL;
	}
	return t;
}

int _table_ncols(table_t * t)
{
	char scols[SCOLS_MAXLEN];
	char *p = scols;
	int i = 0;

	strlcpy(scols, t->scols, sizeof(scols));
	do {
		strsep(&p, ":");
		i++;
	} while (p != NULL);

	return i;
}

// take a just-decoded table t, fill its 'ncols' and 'cols' field.
int _table_cols_unmarshal(ALLOC * a, table_t * t)
{
	char scols[SCOLS_MAXLEN];
	char indices[INDICES_MAXLEN];
	char *p, *token;
	int i;

	strlcpy(scols, t->scols, sizeof(scols));
	p = scols;
	i = 0;
	do {
		token = strsep(&p, ":");
		t->cols[i].type = *token++;
		t->cols[i].unique = *token++;
		strlcpy(t->cols[i++].name, token, NAMELEN + 1);
	} while (p != NULL);
	t->ncols = i;

	strlcpy(indices, t->indices, sizeof(indices));
	p = indices;
	i = 0;
	do {
		token = strsep(&p, ":");
		strlcpy(t->cols[i++].iname, token, NAMELEN + 1);
	} while (p != NULL);

	for (i = 0; i < t->ncols; i++)
		t->cols[i].size = t->sizes[i];

	void *buf;
	size_t len = 0;

	if ((buf = read_blk(a, t->hxroots, NULL, &len)) == NULL)
		return -1;
	if (len != 7 * t->ncols) {
		buf_put(a, buf);
		xerrno = FATAL_INVDB;
		return -1;
	}
	p = buf;
	for (i = 0; i < t->ncols; i++) {
		t->cols[i].index = b2hdl(p);
		p += 7;
	}
	buf_put(a, buf);

	// read indices
	for (i = 0; i < t->ncols; i++) {
		if (t->cols[i].index == 0)
			continue;
		if (open_index(a, t, i) < 0)
			return -1;
	}

	return 0;
}

handle_t alloc_table(ALLOC * a, table_t * t)
{
	unsigned char buf[TABLE_MAXLEN], *p;
	size_t len;
	int i;

	for (i = 0; i < t->ncols; i++) {
		switch (t->cols[i].type) {
		case TYPE_INT:
		case TYPE_FLOAT:
			t->cols[i].size = 4;
			break;
		}
		if (t->cols[i].unique == COL_PRIMARY) {
			if (new_index(a, t, i) < 0)
				return 0;
			snprintf(t->cols[i].iname, NAMELEN + 1,
				 "auto_index_%s_%s", t->name, t->cols[i].name);
		}
	}
	// _table_cols_marshal realloc block pointed by hxroots, so alloc it here
	// buf is much larger
	p = buf;
	for (i = 0; i < t->ncols; i++)
		p = hdl2b(p, t->cols[i].index);
	if ((t->hxroots = alloc_blk(a, buf, p - buf)) == 0)
		return 0;

	if (_table_cols_marshal(a, t) < 0)
		return 0;

	len = (unsigned char *)table2b(buf, t) - buf;
	t->self = alloc_blk(a, buf, len);
	return t->self;
}

int write_table(ALLOC * a, handle_t h, table_t * t)
{
	unsigned char buf[TABLE_MAXLEN];
	size_t len;

	if (_table_cols_marshal(a, t) < 0)
		return -1;
	bzero(buf, sizeof(buf));
	len = (unsigned char *)table2b(buf, t) - buf;
	return realloc_blk(a, h, buf, len);
}

int _table_cols_marshal(ALLOC * a, table_t * t)
{
	size_t len = (NAMELEN + 3) * t->ncols;
	char *p = t->scols, *end = t->scols + len;

	for (int i = 0; i < t->ncols; i++) {
		*p++ = t->cols[i].type;
		*p++ = t->cols[i].unique;
		p = p + strlcpy(p, t->cols[i].name, end - p);
		*p++ = ':';
	}
	*--p = 0;

	len = (NAMELEN + 1) * t->ncols;
	p = t->indices, end = t->indices + len;

	for (int i = 0; i < t->ncols; i++) {
		p = p + strlcpy(p, t->cols[i].iname, end - p);
		*p++ = ':';
	}
	*--p = 0;

	for (int i = 0; i < t->ncols; i++) {
		if (t->cols[i].type == TYPE_STRING)
			t->sizes[i] = t->cols[i].size;
		else		// int and float
			t->sizes[i] = 4;
	}

	void *buf;

	len = 0;
	if ((buf = read_blk(a, t->hxroots, NULL, &len)) == NULL)
		return -1;
	if (len != 7 * t->ncols) {
		buf_put(a, buf);
		xerrno = FATAL_INVDB;
		return -1;
	}
	p = buf;
	for (int i = 0; i < t->ncols; i++)
		p = hdl2b(p, t->cols[i].index);

	int ret = realloc_blk(a, t->hxroots, buf, len);
	buf_put(a, buf);
	return ret;
}

int table_find_col(table_t * t, const char *colname)
{
	for (int i = 0; i < t->ncols; i++)
		if (strcmp(t->cols[i].name, colname) == 0)
			return i;
	return -1;
}

int table_find_index(table_t * t, const char *iname)
{
	for (int i = 0; i < t->ncols; i++) {
		if (t->cols[i].iname == NULL)
			continue;
		if (strcmp(t->cols[i].iname, iname) == 0)
			return i;
	}
	return -1;
}
