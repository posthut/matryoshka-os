/**
 * MatryoshkaOS - Host/kernel type compatibility for unit tests.
 *
 * Include this FIRST in every test file.  It pulls in the standard
 * host types and then prevents <matryoshka/types.h> from redefining
 * them, so the rest of the kernel headers work transparently.
 */

#ifndef MSHKA_TEST_COMPAT_H
#define MSHKA_TEST_COMPAT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Guard the kernel's own type header */
#define MATRYOSHKA_TYPES_H

#ifndef NULL
#define NULL ((void *)0)
#endif

#endif /* MSHKA_TEST_COMPAT_H */
