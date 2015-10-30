#include "common.h"
#include "file.h"
#include <endian.h>
#include <arpa/inet.h>

uint16_t b2uint16(void *buf)
{
	uint16_t i = 0;
	unsigned char *p = buf;
	i |= (uint16_t) * p++;
	i <<= 8;
	i |= *p;
	return i;
}

void *uint16tob(void *buf, uint16_t i)
{
	unsigned char *p = buf;
	p[0] = (unsigned char)((i >> 8) & 0xFF);
	p[1] = (unsigned char)(i & 0xFF);
	return p + 2;
}

handle_t b2hdl(void *buf)
{
	unsigned char *p = buf;
	uint64_t handle = 0;

	for (int i = 0; i < 7; i++, p++) {
		handle <<= 8;
		handle |= *p;
	}
	return handle;
}

void *hdl2b(void *buf, handle_t h)
{
	unsigned char *p = buf;
	for (int i = 0; i < 7; i++)
		*p++ = (h >> ((6 - i) * 8)) & 0xFF;
	return p;
}

off_t hdl2off(handle_t handle)
{
	return FLT_LEN + (handle - 1) * ATOM_LEN;
}

handle_t off2hdl(off_t offset)
{
	if (offset < FLT_LEN)
		return 0;
	return (offset - FLT_LEN) / ATOM_LEN + 1;
}

handle_t read_handle(int fd, off_t offset)
{
	unsigned char s[7];

	if (readat(fd, s, 7, offset) != 7)
		return 0;
	return b2hdl(s);
}

int write_handle(int fd, handle_t h, off_t offset)
{
	unsigned char s[7];

	hdl2b(s, h);
	if (writeat(fd, s, 7, offset) != 7)
		return -1;
	return 0;
}
