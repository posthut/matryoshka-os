/**
 * MatryoshkaOS - Minimal string / memory utilities
 *
 * GCC -ffreestanding may emit implicit calls to memcpy/memset/memmove/memcmp,
 * so these must exist as linkable symbols (kernel/string.c).
 */

#ifndef MATRYOSHKA_STRING_H
#define MATRYOSHKA_STRING_H

#include <matryoshka/types.h>

size_t  strlen(const char *s);
int     strcmp(const char *a, const char *b);
int     strncmp(const char *a, const char *b, size_t n);
char   *strcpy(char *dst, const char *src);
char   *strncpy(char *dst, const char *src, size_t n);

void   *memset(void *s, int c, size_t n);
void   *memcpy(void *dst, const void *src, size_t n);
void   *memmove(void *dst, const void *src, size_t n);
int     memcmp(const void *a, const void *b, size_t n);

#endif /* MATRYOSHKA_STRING_H */
