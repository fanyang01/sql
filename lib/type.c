#include "common.h"
#include <stdint.h>
#include <sys/types.h>
#include <string.h>

void *str2b(void *buf, size_t fixlen, const char *s)
{
	char *p = buf;
	int i;

	for (i = 0; i < fixlen - 1 && *s != '\0'; i++)
		*p++ = *s++;
	while (i++ < fixlen)
		*p++ = '\0';
	return p;
}

void *int32tob(void *buf, int32_t n)
{
	unsigned char *p = buf;
	uint32_t u = *((uint32_t *) (&n));

	for (int i = 0; i < 4; i++)
		*p++ = (u >> ((3 - i) * 8)) & 0xFF;
	return p;
}

int32_t b2int32(void *buf)
{
	uint32_t n = 0;
	unsigned char *p = buf;

	for (int i = 0; i < 4; i++) {
		n <<= 8;
		n |= *p++;
	}
	return *((int32_t *) (&n));
}

void *float2b(void *buf, float f)
{
	unsigned char *p = buf;
	uint32_t u = *((uint32_t *) (&f));

	for (int i = 0; i < 4; i++)
		*p++ = (u >> ((3 - i) * 8)) & 0xFF;
	return p;
}

float b2float(void *buf)
{
	uint32_t n = 0;
	unsigned char *p = buf;

	for (int i = 0; i < 4; i++) {
		n <<= 8;
		n |= *p++;
	}
	return *((float *)(&n));
}

void *vstr2b(void *buf, const char *s)
{
	size_t len = strlen(s);
	char *p = buf;
	uint16tob(p, (uint16_t) len);
	p += 2;
	bcopy(s, p, (uint16_t) len);
	return p + len;
}

size_t vstrsizeof(const char *s)
{
	return 2 + strlen(s);
}

size_t vstrlen(void *buf)
{
	return b2uint16(buf);
}

void *b2vstr(void *buf, char *dst)
{
	char *p = buf;
	size_t len = b2uint16(buf);
	bcopy(p + 2, dst, len);
	dst[len] = '\0';
	return p + 2 + len;
}
