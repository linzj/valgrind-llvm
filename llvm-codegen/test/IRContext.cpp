#include "IRContextInternal.h"
#include "log.h"
#include "Helpers.h"
#include "VexHeaders.h"
#include "Check.h"
#include "RegisterOperation.h"
#include <stdio.h>
#include <string.h>
#define CONTEXT() \
    static_cast<struct IRContextInternal*>(context)
#define PUSH_BACK_STMT(stmt) \
    addStmtToIRSB(CONTEXT()->m_irsb, stmt)

#define PUSH_BACK_REGINIT(name, val) \
    CONTEXT()                        \
        ->m_registerInit.push_back({ name, val })

#define PUSH_BACK_CHECK(c) \
    CONTEXT()              \
        ->m_checks.push_back(c)

void contextSawIRExit(struct IRContext* context, unsigned long long addr)
{
    IRStmt* stmt = IRStmt_Exit(IRExpr_Const(IRConst_U1(1)), Ijk_Boring, IRConst_U64(static_cast<uint64_t>(addr)), offsetof(VexGuestState, guest_RIP));
    PUSH_BACK_STMT(stmt);
    LOGE("saw ir exit\n");
}

void contextSawRegisterInit(struct IRContext* context, const char* registerName, unsigned long long val)
{
    LOGE("%s: val = %llx.\n", __FUNCTION__, val);
    PUSH_BACK_REGINIT(registerName, RegisterInitControl::createConstantInit(val));
}

void contextSawInitOption(struct IRContext* context, const char* opt)
{
    LOGE("%s: opt = %s.\n", __FUNCTION__, opt);

    if (strcmp(opt, "novex") == 0) {
        CONTEXT()
            ->m_novex
            = true;
    }
    else {
        LOGE("%s:unknow option.\n", __FUNCTION__);
    }
}

void contextSawRegisterInitMemory(struct IRContext* context, const char* registerName, unsigned long long size, unsigned long long val)
{
    LOGE("%s: size = %llx, val = %llx.\n", __FUNCTION__, size, val);
    PUSH_BACK_REGINIT(registerName, RegisterInitControl::createMemoryInit(size, val));
}

void contextSawCheckRegisterConst(struct IRContext* context, const char* registerName, unsigned long long val)
{
    LOGE("%s: registerName = %s, val = %llx.\n", __FUNCTION__, registerName, val);
    PUSH_BACK_CHECK(Check::createCheckRegisterEqConst(registerName, val));
}

void contextSawCheckRegister(struct IRContext* context, const char* registerName1, const char* registerName2)
{
    LOGE("%s: registerName1 = %s, registerName2 = %s.\n", __FUNCTION__, registerName1, registerName2);
    PUSH_BACK_CHECK(Check::createCheckRegisterEq(registerName1, registerName2));
}

void contextSawCheckState(struct IRContext* context, unsigned long long val)
{
    LOGE("%s: val = %llx.\n", __FUNCTION__, val);
    PUSH_BACK_CHECK(Check::createCheckState(val));
}

void contextSawCheckMemory(struct IRContext* context, const char* registerName, unsigned long long val)
{
    LOGE("%s: registerName = %s, val = %llx.\n", __FUNCTION__, registerName, val);
    PUSH_BACK_CHECK(Check::createCheckMemory(registerName, val));
}

void contextSawIRWr(struct IRContext* context, const char* id, void* expr)
{
    LOGE("%s: id = %s, expr = %p.\n", __FUNCTION__, id, expr);
    auto&& tmpMap = CONTEXT()->getTempMap();
    auto found = tmpMap.find(id);
    IRTemp tmp;
    if (found != tmpMap.end()) {
        tmp = found->second;
    }
    else {
        tmp = newIRTemp(CONTEXT()->m_irsb->tyenv, Ity_I64);
        tmpMap[id] = tmp;
    }
    IRStmt* stmt = IRStmt_WrTmp(tmp, static_cast<IRExpr*>(expr));
    PUSH_BACK_STMT(stmt);
}

void contextSawIRPut(struct IRContext* context, unsigned long long where, void* expr)
{
    LOGE("%s: where = %llu, expr = %p.\n", __FUNCTION__, where, expr);
    IRStmt* stmt = IRStmt_Put(where, static_cast<IRExpr*>(expr));
    PUSH_BACK_STMT(stmt);
}

void contextSawIRStore(struct IRContext* context, void* valExpr, void* addrExpr)
{
    LOGE("%s:.\n", __FUNCTION__);
    IRStmt* stmt = IRStmt_Store(Iend_LE, static_cast<IRExpr*>(addrExpr), static_cast<IRExpr*>(valExpr));
    PUSH_BACK_STMT(stmt);
}

void contextSawIRLoadG(struct IRContext* context, const char* id, void* conditionExpr, void* addrExpr, void* defaultVal)
{
    LOGE("%s:.\n", __FUNCTION__);
    auto&& tmpMap = CONTEXT()->getTempMap();
    auto found = tmpMap.find(id);
    IRTemp tmp;
    if (found != tmpMap.end()) {
        tmp = found->second;
    }
    else {
        tmp = newIRTemp(CONTEXT()->m_irsb->tyenv, Ity_I64);
        tmpMap[id] = tmp;
    }
    IRStmt* stmt = IRStmt_LoadG(Iend_LE, ILGop_Ident32, tmp, static_cast<IRExpr*>(addrExpr), static_cast<IRExpr*>(defaultVal), static_cast<IRExpr*>(conditionExpr));
    PUSH_BACK_STMT(stmt);
}

void contextSawIRStoreG(struct IRContext* context, void* conditionExpr, void* valExpr, void* addrExpr)
{
    LOGE("%s:.\n", __FUNCTION__);
    IRStmt* stmt = IRStmt_StoreG(Iend_LE, static_cast<IRExpr*>(addrExpr), static_cast<IRExpr*>(valExpr), static_cast<IRExpr*>(conditionExpr));
    PUSH_BACK_STMT(stmt);
}

void* contextNewConstExpr(struct IRContext* context, unsigned long long val)
{
    LOGE("%s: val = %llu.\n", __FUNCTION__, val);
    IRExpr* expr = IRExpr_Const(IRConst_U64(val));
    return expr;
}

void* contextNewRdTmpExpr(struct IRContext* context, const char* id)
{
    LOGE("%s: name = %s.\n", __FUNCTION__, id);

    auto&& tmpMap = CONTEXT()->getTempMap();
    auto found = tmpMap.find(id);
    if (found == tmpMap.end()) {
        return nullptr;
    }
    return IRExpr_RdTmp(found->second);
}

void* contextNewLoadExpr(struct IRContext* context, void* expr)
{
    LOGE("%s:.\n", __FUNCTION__);
    return IRExpr_Load(Iend_LE, Ity_I64, static_cast<IRExpr*>(expr));
}

void* contextNewGetExpr(struct IRContext* context, const char* regName)
{
    LOGE("%s: regName = %s.\n", __FUNCTION__, regName);
    RegisterOperation& op = RegisterOperation::getDefault();
    size_t offset = op.getRegisterPointerOffset(regName);
    return IRExpr_Get(offset, Ity_I64);
}

void contextYYError(int line, int column, struct IRContext* context, const char* reason, const char* text)
{
    printf("line %d column %d: error:%s; text: %s.\n", line, column, reason, text);
}
