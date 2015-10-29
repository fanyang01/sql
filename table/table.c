#include "table.h"
#include "common.h"
#include "alloc.h"
#include "type.h"
#include <strings.h>

void *table2b(void *buf, table_t * t)
{
	unsigned char *p = buf;
	p = hdl2b(p, t->next);
	p = hdl2b(p, t->head);
	p = hdl2b(p, t->hxroots);
	p = vstr2b(p, t->name);
	p = vstr2b(p, t->scols);
	p = vstr2b(p, t->indices);
	p = vstr2b(p, (char *)t->sizes);
	return p;
}

void *b2table(void *buf, table_t * t)
{
	unsigned char *p = buf;
	t->next = b2hdl(p);
	p += 7;
	t->head = b2hdl(p);
	p += 7;
	t->hxroots = b2hdl(p);
	p += 7;
	p = b2vstr(p, t->name);
	p = b2vstr(p, t->scols);
	p = b2vstr(p, t->indices);
	p = b2vstr(p, (char *)t->sizes);
	return p;
}

size_t tblsizeof(table_t * t)
{
	return 7 + 7 + 7 + vstrsizeof(t->name) + vstrsizeof(t->scols) +
	    vstrsizeof(t->indices) + vstrsizeof((char *)t->sizes);
}

handle_t alloc_table(ALLOC * a, table_t * t)
{
	unsigned char buf[TABLE_MAXLEN];
	size_t len;

	bzero(buf, sizeof(buf));
	len = (unsigned char *)table2b(buf, t) - buf;
	return alloc_blk(a, buf, len);
}

int read_table(ALLOC * a, handle_t h, table_t * t)
{
	unsigned char b[TABLE_MAXLEN];
	size_t len = TABLE_MAXLEN;
	unsigned char *buf;

	if ((buf = read_blk(a, h, b, &len)) == NULL)
		return -1;
	b2table(buf, t);
	if (buf != b)
		buf_put(a, buf);
	return 0;
}

int write_table(ALLOC * a, handle_t h, table_t * t)
{
	unsigned char buf[TABLE_MAXLEN];
	size_t len;

	bzero(buf, sizeof(buf));
	len = (unsigned char *)table2b(buf, t) - buf;
	return realloc_blk(a, h, buf, len);
}
