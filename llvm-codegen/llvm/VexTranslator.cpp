#include <unordered_map>
#include "VexTranslator.h"
#include "Output.h"
#include "log.h"

VexTranslator::VexTranslator()
    : m_code(nullptr)
    , m_codeSize(0)
{
}

VexTranslator::~VexTranslator()
{
}

namespace {
class VexTranslatorImpl : public VexTranslator {
    using namespace jit;

public:
    VexTranslatorImpl();

private:
    virtual bool translate(IRSB*) override;
    void translateStmt(IRStmt* stmt);

    LValue genGuestArrayOffset(IRRegArray* descr,
        IRExpr* off, Int bias);
    void translatePut(IRStmt* stmt);
    void translatePutI(IRStmt* stmt);
    void translateWrTmp(IRStmt* stmt);
    inline void ensureType(LValue val, IRType type) __attribute__((pure));

    typedef std::unordered_map<IRTemp, LValue> TmpValMap;
    Output m_output;
    IRSB* m_bb;
    TmpValMap m_tmpValMap;
};

VexTranslatorImpl::VexTranslatorImpl()
    : m_bb(nullptr)
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
    return true;
}

void VexTranslatorImpl::translateStmt(IRStmt* stmt)
{
    switch (stmt->tag) {
    case Ist_Store: {
        EMASSERT("not yet implement" && false);
    } break;
    case Ist_Put: {
        translatePut(stmt);
    } break;
    case Ist_PutI: {
        translatePutI(stmt);
    } break;
    case Ist_WrTmp: {
        translateWrTmp(stmt);
    } break;
    }
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

void VexTranslatorImpl::translatePut(IRStmt* stmt)
{
    LValue expr = translateExpr(stmt->Ist.Put.data);
    m_output.buildStoreArgIndex(expr, stmt->Ist.Put.offset / sizeof(intptr_t));
}

void VexTranslatorImpl::translatePutI(IRStmt* stmt)
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
}

void VexTranslatorImpl::translateWrTmp(IRStmt* stmt)
{
    IRTemp tmp = stmt->Ist.WrTmp.tmp;
    LValue val = translateExpr(stmt->Ist.WrTmp.data);
    ensureType(val, typeOfIRExpr(stmt->Ist.WrTmp.data));
    ensureType(val, typeOfIRTemp(m_bb->tyenv, tmp));
    m_tmpValMap[tmp] = val;
}

void VexTranslatorImpl::ensureType(LValue val, IRType type)
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
}
