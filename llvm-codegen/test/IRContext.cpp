#include "IRContextInternal.h"
#include "log.h"
#include "Helpers.h"
#include "VexHeaders.h"
#include <stdio.h>
#define CONTEXT() \
    static_cast<struct IRContextInternal*>(context)
#define PUSH_BACK_STMT(stmt) \
    CONTEXT()->m_statments.push_back(stmt)

#define PUSH_BACK_REGINIT(name, val) \
    CONTEXT()->m_registerInit.push_back({ name, val })

void contextSawIRExit(struct IRContext* context, unsigned long long addr)
{
    IRStmt* stmt = IRStmt_Exit(IRExpr_Const(IRConst_U1(1)), Ijk_Boring, IRConst_U64(static_cast<uint64_t>(addr)), offsetof(VexGuestAMD64State, guest_RIP));
    PUSH_BACK_STMT(stmt);
    LOGE("saw ir exit\n");
}

void contextSawRegisterInit(struct IRContext* context, const char* registerName, unsigned long long val)
{
    LOGE("%s: val = %llx.\n", __FUNCTION__, val);
    PUSH_BACK_REGINIT(registerName, val);
}

void contextSawCheckRegisterConst(struct IRContext* context, const char* registerName, unsigned long long val)
{
    LOGE("%s: registerName = %s, val = %llx.\n", __FUNCTION__, registerName, val);
}

void contextSawCheckRegister(struct IRContext* context, const char* registerName1, const char* registerName2)
{
    LOGE("%s: registerName1 = %s, registerName2 = %s.\n", __FUNCTION__, registerName1, registerName2);
}

void contextSawChecktState(struct IRContext* context, unsigned long long val)
{
    LOGE("%s: val = %llx.\n", __FUNCTION__, val);
}

void contextYYError(int line, int column, struct IRContext* context, const char* reason)
{
    printf("line %d column %d: error:%s.\n", line, column, reason);
}
