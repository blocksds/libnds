// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2013 Jason Rogers (Dovoto)
// Copyright (C) 2013 Michael Theall (mtheall)

// Definitons for DS assertions

#include <nds/arm9/console.h>

#include <stdio.h>
#include <stdarg.h>

void __sassert(const char *fileName, int lineNumber, const char* conditionString, const char* format, ...)
{
    va_list ap;

    consoleDemoInit();

    printf("\x1b[j"               /* clear screen */
           "\x1b[42mAssertion!\n" /* print in green? */
           "\x1b[39mFile: \n"     /* print in default color */
           "%s\n\n"               /* print filename */
           "Line: %d\n\n"         /* print line number */
           "Condition:\n"
           "%s\n\n"               /* print condition message */
           "\x1b[41m",            /* change font color to red */
           fileName, lineNumber, conditionString);

    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);

    //todo: exit properly
    while(1);
}
