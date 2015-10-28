#ifndef _COMMON_H
#define _COMMON_H

#include "alloc.h"

extern uint16_t b2uint16(void *buf);
extern void uint16tob(void *buf, uint16_t i);
extern off_t hdl2off(handle_t handle);
extern handle_t off2hdl(off_t offset);
extern handle_t b2hdl(void *buf);
extern void hdl2b(void *buf, handle_t h);
extern handle_t read_handle(int fd, off_t offset);
extern int write_handle(int fd, handle_t h, off_t offset);

#endif
