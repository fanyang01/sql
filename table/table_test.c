#include "table.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

static int equal(table_t * t1, table_t * t2)
{
	return t1->next == t2->next &&
	    t1->head == t2->head &&
	    t1->hxroots == t2->hxroots &&
	    strcmp(t1->name, t2->name) == 0 &&
	    strcmp(t1->scols, t2->scols) == 0 &&
	    strcmp(t1->indices, t2->indices) == 0 &&
	    strcmp((char *)t1->sizes, (char *)t2->sizes) == 0;
}

int main(void)
{
	INIT_TABLE(t1);
	INIT_TABLE(t2);
	unsigned char buf[TABLE_MAXLEN];

	t1.next = 123;
	t1.head = 456;
	t1.hxroots = 789;
	strncpy(t1.name, "table_name", NAMELEN);
	strncpy(t1.scols, "icol1:fcol2:scol3:icol4", SCOLS_MAXLEN);
	strncpy(t1.indices, "index1::index3:", INDICES_MAXLEN);
	strncpy((char *)t1.sizes, "\x04\x04\xFF\x04", MAXCOLS);

	assert((unsigned char *)table2b(buf, &t1) - buf == tblsizeof(&t1));
	assert((unsigned char *)b2table(buf, &t2) - buf == tblsizeof(&t1));
	assert(equal(&t1, &t2));
	return 0;
}
