#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "type.h"

int main(void)
{
	unsigned char buf[16];
	char s[] = "hello, world";

	assert(str2b(buf, 16, s) == &buf[16]);
	assert(strncmp((char *)buf, s, sizeof(s)) == 0);
	assert(buf[15] == 0);

	int32_t i = 0x2345678;
	assert(int32tob(buf, i) == &buf[4]);
	assert(b2int32(buf) == i);
	i = -123;
	int32tob(buf, i);
	assert(b2int32(buf) == i);

	float f = 0.0625;
	assert(float2b(buf, f) == &buf[4]);
	assert(b2float(buf) == f);

	assert(vstrsizeof(s) == 14);
	assert(vstr2b(buf, s) == &buf[14]);
	assert(vstrlen(buf) == 12);
	assert(b2vstr(buf, s) == &buf[14]);
	assert(strncmp(s, "hello, world", sizeof(s)) == 0);
	return 0;
}
