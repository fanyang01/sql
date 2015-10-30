#include "db.h"
#include "xerror.h"
#include "common.h"
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>

static DB *_alloc_db(size_t pathlen);
static void _free_db(DB * db);

DB *opendb(const char *path, int oflag, ...)
{
	DB *db;
	size_t len;
	int mode, fd;

	len = strlen(path);
	if ((db = _alloc_db(len)) == NULL)
		return NULL;
	strcpy(db->name, path);

	if (oflag & O_CREAT) {
		va_list ap;

		va_start(ap, oflag);
		mode = va_arg(ap, int);
		fd = open(path, oflag, mode);
	} else {
		fd = open(path, oflag);
	}

	if (fd < 0 || init_allocator(&db->a, fd, oflag) < 0)
		goto Error;

	unsigned char buf[7];

	if (oflag & O_CREAT) {
		bzero(buf, 7);
		if (alloc_blk(&db->a, buf, 7) != 1)
			goto Error;
	} else {
		len = 7;
		void *p = read_blk(&db->a, 1, buf, &len);

		if (p == NULL)
			goto Error;
		if (len != 7)
			goto Error;
		db->db_root = b2hdl(p);
		if (p != buf)
			buf_put(&db->a, p);
	}
	return db;

 Error:
	preserve_errno(_free_db(db));
	return NULL;
}

void closedb(DB * db)
{
	close(db->a.fd);
	_free_db(db);
}

DB *_alloc_db(size_t pathlen)
{
	DB *db;
	if ((db = calloc(1, sizeof(DB))) == NULL)
		return NULL;
	if ((db->name = malloc(pathlen + 1)) == NULL) {
		free(db);
		return NULL;
	}
	return db;
}

void _free_db(DB * db)
{
	free(db->name);
	free(db);
}
