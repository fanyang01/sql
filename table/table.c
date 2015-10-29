#include "table.h"
#include "common.h"
#include "type.h"

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
