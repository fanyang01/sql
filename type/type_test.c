#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "type.h"

int main(void)
{
	unsigned char buf[16];
	char s[] = "hello, world";

	str2b(buf, 16, s);
	assert(strncmp((char *)buf, s, sizeof(s)) == 0);
	assert(buf[15] == 0);

	int32_t i = 0x2345678;
	int32tob(buf, i);
	assert(b2int32(buf) == i);
	i = -123;
	int32tob(buf, i);
	assert(b2int32(buf) == i);

	float f = 0.0625;
	float2b(buf, f);
	assert(b2float(buf) == f);

	return 0;
}
