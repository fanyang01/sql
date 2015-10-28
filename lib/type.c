#include <stdint.h>
#include <sys/types.h>

void str2b(void *buf, size_t fixlen, const char *s)
{
	char *p = buf;
	int i;

	for (i = 0; i < fixlen - 1 && *s != '\0'; i++)
		*p++ = *s++;
	while (i++ < fixlen)
		*p++ = '\0';
}

void int32tob(void *buf, int32_t n)
{
	unsigned char *p = buf;
	uint32_t u = *((uint32_t *) (&n));

	for (int i = 0; i < 4; i++)
		*p++ = (u >> ((3 - i) * 8)) & 0xFF;
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

void float2b(void *buf, float f)
{
	unsigned char *p = buf;
	uint32_t u = *((uint32_t *) (&f));

	for (int i = 0; i < 4; i++)
		*p++ = (u >> ((3 - i) * 8)) & 0xFF;
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
