#include "IRContext.h"
#include <stdio.h>

void contextSawIRExit(struct IRContext*)
{
    printf("saw ir exit.\n");
}

void contextSawEof(struct IRContext*)
{
    printf("saw eof.\n");
}

void contextSawError(struct IRContext*)
{
    printf("saw error.\n");
}

void contextYYError(int line, int column, struct IRContext* context, const char* reason)
{
    printf("line %d column %d: error:%s.\n", line, column, reason);
}
