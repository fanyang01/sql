#include "common.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
	unsigned char buf[8];

	uint16tob(buf, 0x1234);
	assert(buf[0] == 0x12 && buf[1] == 0x34);

	uint16_t i16 = b2uint16(buf);
	assert(i16 == 0x1234);

	handle_t h = 0x34567890;
	hdl2byte(buf, h);
	assert(h == byte2hdl(buf));

	FILE *f = tmpfile();
	assert(f != NULL);
	int fd = fileno(f);
	assert(write_handle(fd, h, 0) == 0);
	assert(read_handle(fd, 0) == h);
	return 0;
}
