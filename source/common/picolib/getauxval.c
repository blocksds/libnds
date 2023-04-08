// SPDX-License-Identifier: BSD-3-Clause
//
// Copyright © 2020 Keith Packard

#include <sys/auxv.h>
#include <errno.h>
#include <sys/cdefs.h>

unsigned long getauxval(unsigned long type)
{
	(void) type;
	errno = EINVAL;
	return 0;
}
__weak_reference(getauxval, __getauxval);
