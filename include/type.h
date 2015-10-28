#ifndef _SQL_TYPE_H
#define _SQL_TYPE_H

#include <stdint.h>
#include <sys/types.h>

#define TYPE_INT_SIZE 4
#define TYPE_FLOAT_SIZE 4

#define TYPE_INT 'i'
#define TYPE_FLOAT 'f'
#define TYPE_STRING 's'

/*
 * support SQL types:
 * - int
 * - float
 * - fix-length string: char(n), n <= 255
 *
 * store format
 * ============
 * - int: 4 bytes, big-endian
 * - float: 4 bytes, big-endian
 * - string: n bytes, null-terminating and null-padding
 *
 * var-length string
 * =================
 * - for name of tables, columns, ...
 * - need n + 2 bytes
 * - first two bytes store the length of string(NOT include terminating null byte), big-endian
 * - next n bytes store string without terminating null byte
 */
#define MAX_VARSTRLEN ((1<<16) - 1)

extern void str2byte(void *buf, size_t fixlen, const char *s);
extern void int32tob(void *buf, int32_t i);
extern int32_t b2int32(void *buf);
extern void float2byte(void *buf, float f);
extern float byte2float(void *buf);

#endif
