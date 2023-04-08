// SPDX-License-Identifier: BSD-3-Clause
//
// Copyright © 2019 Keith Packard

#include <picotls.h>
#include <string.h>
#include <stdint.h>

/* This needs to be global so that __aeabi_read_tp can
 * refer to it in an asm statement
 */
void *__tls;

/* The size of the thread control block.
 * TLS relocations are generated relative to
 * a location this far *before* the first thread
 * variable (!)
 * NB: The actual size before tp also includes padding
 * to align up to the alignment of .tdata/.tbss.
 */
#define TCB_SIZE	8

void _set_tls(void *tls)
{
	__tls = (uint8_t *) tls - TCB_SIZE;
}
