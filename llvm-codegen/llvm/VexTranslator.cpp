#include <unordered_map>
#include "VexTranslator.h"
#include "Output.h"
#include "log.h"

VexTranslator::VexTranslator(const VexTranslatorEnv& env)
    : m_code(nullptr)
    , m_codeSize(0)
    , m_env(env)
{
}

VexTranslator::~VexTranslator()
{
}

namespace {
class VexTranslatorImpl : public VexTranslator {
    using namespace jit;

public:
    VexTranslatorImpl(const VexTranslatorEnv& env);

private:
    virtual bool translate(IRSB*) override;
    bool translateStmt(IRStmt* stmt);
    bool translateNext();

    LValue genGuestArrayOffset(IRRegArray* descr,
        IRExpr* off, Int bias);
    bool translatePut(IRStmt* stmt);
    bool translatePutI(IRStmt* stmt);
    bool translateWrTmp(IRStmt* stmt);

    LValue translateExpr(IRExpr* expr);
    LValue translateRdTmp(IRExpr* expr);
    LValue translateConst(IRExpr* expr);
    inline void _ensureType(LValue val, IRType type) __attribute__((pure));
#define ensureType(val, type) \
    _ensureType(val, type)

    typedef std::unordered_map<IRTemp, LValue> TmpValMap;
    Output m_output;
    IRSB* m_bb;
    TmpValMap m_tmpValMap;
};

VexTranslatorImpl::VexTranslatorImpl(const VexTranslatorEnv& env)
    : VexTranslator(env)
    , m_bb(nullptr)
{
}

bool VexTranslatorImpl::translate(IRSB* bb) override
{
    m_bb = bb;
    for (int i = 0; i < bb->stmts_used; i++)
        if (bb->stmts[i]) {
            bool r = translateStmt(bb->stmts[i]);
            if (!r) {
                return false;
            }
        }
    return translateNext();
}

bool VexTranslatorImpl::translateStmt(IRStmt* stmt)
{
    switch (stmt->tag) {
    case Ist_Store: {
        EMASSERT("not yet implement" && false);
        return false;
    } break;
    case Ist_Put: {
        return translatePut(stmt);
    } break;
    case Ist_PutI: {
        return translatePutI(stmt);
    } break;
    case Ist_WrTmp: {
        return translateWrTmp(stmt);
    } break;
    default:
        EMASSERT("not yet implement" && false);
        return false;
    }
    return false;
}

LValue VexTranslatorImpl::genGuestArrayOffset(IRRegArray* descr,
    IRExpr* off, Int bias)
{
    LValue roff = translateExpr(off);
    LValue tmp = roff;
    Int elemSz = sizeofIRType(descr->elemTy);
    if (bias != 0) {
        /* Make sure the bias is sane, in the sense that there are
           no significant bits above bit 30 in it. */
        EMASSERT(-10000 < bias && bias < 10000);
        tmp = m_output.buildAdd(m_output.constInt32(bias), tmp);
    }
    tmp = m_output.buildAnd(m_output.constInt32(7), tmp);
    EMASSERT(elemSz == 1 || elemSz == 8);
    tmp = m_output.buildShl(tmp, elemSz == 8 ? 3 : 0);
    tmp = m_output.buildAdd(tmp, m_output.constInt32(descr->base));
    return tmp;
}

bool VexTranslatorImpl::translatePut(IRStmt* stmt)
{
    LValue expr = translateExpr(stmt->Ist.Put.data);
    m_output.buildStoreArgIndex(expr, stmt->Ist.Put.offset / sizeof(intptr_t));
    return true;
}

bool VexTranslatorImpl::translatePutI(IRStmt* stmt)
{
    IRPutI* puti = stmt->Ist.PutI.details;
    LValue offset = genGuestArrayOffset(puti->descr,
        puti->ix, puti->bias);
    IRType ty = typeOfIRExpr(m_bb->tyenv, puti->data);
    LValue val = translateExpr(puti->data);
    LValue pointer = m_output.buildArgBytePointer();
    LValue afterOffset = m_output.buildAdd(pointer, offset);
    ensureType(val, ty);
    switch (ty) {
    case Ity_F64: {
        LValue casted = m_output.buildCast(LLVMBitCast, afterOffset, m_output.repo().refDouble);
        m_output.buildStore(val, casted);
    } break;
    case Ity_I8: {
        LValue casted = m_output.buildCast(LLVMBitCast, afterOffset, m_output.repo().ref8);
        m_output.buildStore(val, casted);
    } break;
    case Ity_I64: {
        LValue casted = m_output.buildCast(LLVMBitCast, afterOffset, m_output.repo().ref64);
        m_output.buildStore(val, casted);
    } break;
    default:
        EMASSERT("not implement type yet" && false);
        break;
    }
    return true;
}

bool VexTranslatorImpl::translateWrTmp(IRStmt* stmt)
{
    IRTemp tmp = stmt->Ist.WrTmp.tmp;
    LValue val = translateExpr(stmt->Ist.WrTmp.data);
    ensureType(val, typeOfIRExpr(stmt->Ist.WrTmp.data));
    ensureType(val, typeOfIRTemp(m_bb->tyenv, tmp));
    m_tmpValMap[tmp] = val;
    return true;
}

void VexTranslatorImpl::_ensureType(LValue val, IRType type)
{
    switch (type) {
    case Ity_I1:
        EMASSERT(m_output.typeOf(val) == m_output.repo().int1);
    case Ity_I8:
        EMASSERT(m_output.typeOf(val) == m_output.repo().int8);
    case Ity_I16:
        EMASSERT(m_output.typeOf(val) == m_output.repo().int16);
    case Ity_I32:
        EMASSERT(m_output.typeOf(val) == m_output.repo().int32);
    case Ity_I64:
        EMASSERT(m_output.typeOf(val) == m_output.repo().int64);
    case Ity_I128:
        EMASSERT(m_output.typeOf(val) == m_output.repo().int128);
    case Ity_D32:
    case Ity_F32:
        EMASSERT(m_output.typeOf(val) == m_output.repo().floatType);
    case Ity_D64:
    case Ity_F64:
        EMASSERT(m_output.typeOf(val) == m_output.repo().doubleType);
    case Ity_D128:
    case Ity_F128:
        EMASSERT("AMD64 not support F128 D128." && false);
    case Ity_V128:
        EMASSERT(m_output.typeOf(val) == m_output.repo().v128Type);
    case Ity_V256:
        EMASSERT(m_output.typeOf(val) == m_output.repo().v256Type);
    default:
        EMASSERT("unknow type" && false);
    }
}

bool VexTranslatorImpl::translateNext()
{
    IRExpr* next = m_bb->next;
    IRJumpKind jk = m_bb->jumpkind;
    /* Case: boring transfer to known address */
    LValue val = translateExpr(next);
    if (next->tag == Iex_Const) {
        IRConst* cdst = next->Iex.Const.con;
        if (env().m_chainingAllow)
            m_output.buildDirectPatch(val);
        else
            m_output.buildAssistPatch(val);
        return true;
    }

    /* Case: call/return (==boring) transfer to any address */
    switch (jk) {
    case Ijk_Boring:
    case Ijk_Ret:
    case Ijk_Call: {
        if (env->chainingAllowed) {
            m_output.buildIndirectPatch(val);
        }
        else {
            m_output.buildAssistPatch(val);
        }
        return true;
    }
    default:
        break;
    }

    /* Case: assisted transfer to arbitrary address */
    switch (jk) {
    /* Keep this list in sync with that for Ist_Exit above */
    case Ijk_ClientReq:
    case Ijk_EmWarn:
    case Ijk_NoDecode:
    case Ijk_NoRedir:
    case Ijk_SigSEGV:
    case Ijk_SigTRAP:
    case Ijk_Sys_syscall:
    case Ijk_InvalICache:
    case Ijk_Yield: {
        m_output.buildAssistPatch(val);
        return true;
    }
    default:
        break;
    }

    vex_printf("\n-- PUT(%d) = ", offsIP);
    ppIRExpr(next);
    vex_printf("; exit-");
    ppIRJumpKind(jk);
    vex_printf("\n");
    vassert(0); // are we expecting any other kind?
}

LValue VexTranslatorImpl::translateExpr(IRExpr* expr)
{
    switch (expr->tag) {
    case Iex_RdTmp: {
        return translateRdTmp(expr);
    }
    case Iex_Const: {
        return translateConst(expr);
    }
    }
}

LValue VexTranslatorImpl::translateRdTmp(IRExpr* expr)
{
    auto found = m_tmpValMap.find(expr->Iex.RdTmp.tmp);
    EMASSERT(found != m_tmpValMap.end());
    return found->second;
}

LValue VexTranslatorImpl::translateConst(IRExpr* expr)
{
    IRConst* myconst = expr->Iex.Const.con;
    switch (myconst->tag) {
    case Ico_U1:
        return m_output.constInt1(myconst->U1);
    case Ico_U8:
        return m_output.constInt8(myconst->U8);
    case Ico_U16:
        return m_output.constInt16(myconst->U16);
    case Ico_U32:
        return m_output.constInt32(myconst->U32);
    case Ico_U64:
        return m_output.constInt64(myconst->U64);
    case Ico_F32:
        return m_output.constFloat(myconst->F32);
    case Ico_F64:
        return m_output.constFloat(myconst->F64);
    case Ico_F32i: {
        LValue val = m_output.constInt32(myconst->F32i);
        return m_output.buildCast(LLVMBitCast, val, m_output.repo().floatType);
    }
    case Ico_F64i: {
        LValue val = m_output.constInt64(myconst->F64i);
        return m_output.buildCast(LLVMBitCast, val, m_output.repo().doubleType);
    }
    case Ico_V128:
        return m_output.constV128(myconst->V128);

    case Ico_V256:
        return m_output.constV256(myconst->V256);
    }
    EMASSERT("not supported constant" && false);
}
}
