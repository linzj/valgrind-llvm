#include "IRContextInternal.h"
#include "log.h"
#include "Helpers.h"
extern "C" {
#include <libvex_guest_amd64.h>
}
#include <stdio.h>
#define CONTEXT() \
    static_cast<struct IRContextInternal*>(context)
#define PUSH_BACK(stmt) \
    CONTEXT()->m_statments.push_back(stmt)

void contextSawIRExit(struct IRContext* context)
{
    IRStmt* stmt = IRStmt_Exit(IRExpr_Const(IRConst_U1(1)), Ijk_Boring, IRConst_U64(reinterpret_cast<uint64_t>(helperIRExit)), offsetof(VexGuestAMD64State, guest_RIP));
    PUSH_BACK(stmt);
    LOGE("saw ir exit\n");
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
