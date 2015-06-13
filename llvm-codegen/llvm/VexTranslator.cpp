#include <unordered_map>
#include <memory>
#include "InitializeLLVM.h"
#include "CompilerState.h"
#include "Compile.h"
#include "Link.h"
#include "VexTranslator.h"
#include "Registers.h"
#include "Output.h"
#include "log.h"

namespace jit {

VexTranslator::VexTranslator()
    : m_code(nullptr)
    , m_codeSize(0)
{
}

VexTranslator::~VexTranslator()
{
}

bool VexTranslator::init()
{
    initLLVM();
    return true;
}
}

namespace {
using namespace jit;
static inline UChar clearWBit(UChar rex)
{
    return toUChar(rex & ~(1 << 3));
}

inline static uint8_t rexAMode_R__wrk(unsigned gregEnc3210, unsigned eregEnc3210)
{
    uint8_t W = 1; /* we want 64-bit mode */
    uint8_t R = (gregEnc3210 >> 3) & 1;
    uint8_t X = 0; /* not relevant */
    uint8_t B = (eregEnc3210 >> 3) & 1;
    return 0x40 + ((W << 3) | (R << 2) | (X << 1) | (B << 0));
}

static inline unsigned iregEnc3210(unsigned in)
{
    return in;
}

static uint8_t rexAMode_R(unsigned greg, unsigned ereg)
{
    return rexAMode_R__wrk(iregEnc3210(greg), iregEnc3210(ereg));
}

inline static uint8_t mkModRegRM(unsigned mod, unsigned reg, unsigned regmem)
{
    return (uint8_t)(((mod & 3) << 6) | ((reg & 7) << 3) | (regmem & 7));
}

inline static uint8_t* doAMode_R__wrk(uint8_t* p, unsigned gregEnc3210, unsigned eregEnc3210)
{
    *p++ = mkModRegRM(3, gregEnc3210 & 7, eregEnc3210 & 7);
    return p;
}

static uint8_t* doAMode_R(uint8_t* p, unsigned greg, unsigned ereg)
{
    return doAMode_R__wrk(p, iregEnc3210(greg), iregEnc3210(ereg));
}

static uint8_t* emit64(uint8_t* p, uint64_t w64)
{
    *reinterpret_cast<uint64_t*>(p) = w64;
    return p + sizeof(w64);
}

typedef jit::CompilerState State;

class VexTranslatorImpl : public jit::VexTranslator {
public:
    VexTranslatorImpl();

private:
    virtual bool translate(IRSB*, const VexTranslatorEnv& env) override;
    bool translateStmt(IRStmt* stmt);
    bool translateNext();

    jit::LValue genGuestArrayOffset(IRRegArray* descr,
        IRExpr* off, Int bias);
    bool translatePut(IRStmt* stmt);
    bool translatePutI(IRStmt* stmt);
    bool translateWrTmp(IRStmt* stmt);
    bool translateExit(IRStmt* stmt);
    bool translateStore(IRStmt* stmt);
    bool translateStoreG(IRStmt* stmt);
    bool translateLoadG(IRStmt* stmt);

    jit::LValue translateExpr(IRExpr* expr);
    jit::LValue translateRdTmp(IRExpr* expr);
    jit::LValue translateConst(IRExpr* expr);
    jit::LValue translateGet(IRExpr* expr);
    jit::LValue translateLoad(IRExpr* expr);
    inline void _ensureType(jit::LValue val, IRType type) __attribute__((pure));
#define ensureType(val, type) \
    _ensureType(val, type)

    LValue castToPointer(IRType type, LValue p);
    static void patchProloge(void*, uint8_t* start);
    static void patchDirect(void*, uint8_t* p, void*);
    static void patchIndirect(void*, uint8_t* p, void*);
    static void patchAssist(void*, uint8_t* p, void* entry);
    typedef std::unordered_map<IRTemp, jit::LValue> TmpValMap;
    jit::Output* m_output;
    IRSB* m_bb;
    const VexTranslatorEnv* m_env;
    TmpValMap m_tmpValMap;
    jit::ByteBuffer m_code;
    std::unique_ptr<State> m_state;
    bool m_chainingAllow;
};

void VexTranslatorImpl::patchProloge(void*, uint8_t* start)
{
    uint8_t* p = start;
    static const uint8_t evCheck[] = {
        0xff,
        0x4d,
        0x08,
        0x79,
        0x03,
        0xff,
        0x65,
        0x00,
    };
    // 4:	ff 4d 08             	decl   0x8(%rbp)
    // 7:	79 03                	jns    c <nofail>
    // 9:	ff 65 00             	jmpq   *0x0(%rbp)
    memcpy(p, evCheck, sizeof(evCheck));
    p += sizeof(evCheck);
    *p++ = rexAMode_R(jit::RBP,
        jit::RDI);
    *p++ = 0x89;
    p = doAMode_R(p, jit::RBP,
        jit::RDI);
}

void VexTranslatorImpl::patchDirect(void*, uint8_t* p, void* entry)
{
    // epilogue

    // 3 bytes
    *p++ = rexAMode_R(jit::RBP,
        jit::RDI);
    *p++ = 0x89;
    p = doAMode_R(p, jit::RBP,
        jit::RSP);
    // 1 bytes pop rbp
    *p++ = 0x5d;

    /* 10 bytes: movabsq $target, %r11 */
    *p++ = 0x49;
    *p++ = 0xBB;
    p = emit64(p, reinterpret_cast<uintptr_t>(entry));
    /* movq %r11, RIP(%rbp) */

    /* 3 bytes: call*%r11 */
    *p++ = 0x41;
    *p++ = 0xFF;
    *p++ = 0xD3;
}

void VexTranslatorImpl::patchIndirect(void*, uint8_t* p, void* entry)
{
    // epilogue
    // 3 bytes
    *p++ = rexAMode_R(jit::RBP,
        jit::RDI);
    *p++ = 0x89;
    p = doAMode_R(p, jit::RBP,
        jit::RSP);
    // 1 bytes pop rbp
    *p++ = 0x5d;

    /* 10 bytes: movabsq $target, %r11 */
    *p++ = 0x49;
    *p++ = 0xBB;
    p = emit64(p, reinterpret_cast<uintptr_t>(entry));
    /* movq %r11, RIP(%rbp) */

    /* 3 bytes: jmp *%r11 */
    *p++ = 0x41;
    *p++ = 0xFF;
    *p++ = 0xE3;
}

void VexTranslatorImpl::patchAssist(void*, uint8_t* p, void* entry)
{
    // epilogue

    // 3 bytes
    *p++ = rexAMode_R(jit::RBP,
        jit::RDI);
    *p++ = 0x89;
    p = doAMode_R(p, jit::RBP,
        jit::RSP);
    // 1 bytes pop rbp
    *p++ = 0x5d;

    /* 10 bytes: movabsq $target, %r11 */
    *p++ = 0x49;
    *p++ = 0xBB;
    p = emit64(p, reinterpret_cast<uintptr_t>(entry));
    /* movq %r11, RIP(%rbp) */

    /* 3 bytes: jmp *%r11 */
    *p++ = 0x41;
    *p++ = 0xFF;
    *p++ = 0xE3;
}

VexTranslatorImpl::VexTranslatorImpl()
    : m_output(nullptr)
    , m_bb(nullptr)
    , m_env(nullptr)
    , m_chainingAllow(false)
{
}

bool VexTranslatorImpl::translate(IRSB* bb, const VexTranslatorEnv& env)
{
    PlatformDesc desc = {
        env.m_contextSize,
        static_cast<size_t>(bb->offsIP), /* offset of pc */
        11, /* prologue size */
        17, /* direct size */
        17, /* indirect size */
        17, /* assist size */
    };
    m_bb = bb;

    m_state.reset(new State("llvm", desc));
    State& state = *m_state;
    Output output(state);
    m_output = &output;
    m_env = &env;
    if (env.m_dispDirect && env.m_dispDirectSlow && env.m_dispIndirect) {
        m_chainingAllow = true;
    }

    for (int i = 0; i < bb->stmts_used; i++)
        if (bb->stmts[i]) {
            bool r = translateStmt(bb->stmts[i]);
            if (!r) {
                return false;
            }
        }
    if (!translateNext()) {
        return false;
    }

    dumpModule(state.m_module);
    compile(state);
    void (*m_patchPrologue)(void* opaque, uint8_t* start);
    void (*m_patchDirect)(void* opaque, uint8_t* toFill, void*);
    void (*m_patchIndirect)(void* opaque, uint8_t* toFill, void*);
    void (*m_patchAssist)(void* opaque, uint8_t* toFill, void*);
    LinkDesc linkDesc = {
        this,
        env.m_dispDirect,
        env.m_dispDirectSlow,
        env.m_dispIndirect,
        env.m_dispAssist,
        patchProloge,
        patchDirect,
        patchIndirect,
        patchAssist
    };
    link(state, linkDesc);

    m_bb = nullptr;
    m_output = nullptr;
    m_env = nullptr;
    setCode(state.m_codeSectionList.front().data());
    setCodeSize(state.m_codeSectionList.front().size());
    return true;
}

bool VexTranslatorImpl::translateStmt(IRStmt* stmt)
{
    switch (stmt->tag) {
    case Ist_Put: {
        return translatePut(stmt);
    } break;
    case Ist_PutI: {
        return translatePutI(stmt);
    } break;
    case Ist_WrTmp: {
        return translateWrTmp(stmt);
    } break;
    case Ist_Exit: {
        return translateExit(stmt);
    }
    case Ist_Store: {
        return translateStore(stmt);
    }
    case Ist_StoreG: {
        return translateStoreG(stmt);
    }
    case Ist_LoadG: {
        return translateLoadG(stmt);
    }
    default:
        EMASSERT("not yet implement" && false);
        return false;
    }
    return false;
}

jit::LValue VexTranslatorImpl::genGuestArrayOffset(IRRegArray* descr,
    IRExpr* off, Int bias)
{
    LValue roff = translateExpr(off);
    LValue tmp = roff;
    Int elemSz = sizeofIRType(descr->elemTy);
    if (bias != 0) {
        /* Make sure the bias is sane, in the sense that there are
           no significant bits above bit 30 in it. */
        EMASSERT(-10000 < bias && bias < 10000);
        tmp = m_output->buildAdd(m_output->constInt32(bias), tmp);
    }
    tmp = m_output->buildAnd(m_output->constInt32(7), tmp);
    EMASSERT(elemSz == 1 || elemSz == 8);
    tmp = m_output->buildShl(tmp, m_output->constInt32(elemSz == 8 ? 3 : 0));
    tmp = m_output->buildAdd(tmp, m_output->constInt32(descr->base));
    return tmp;
}

bool VexTranslatorImpl::translatePut(IRStmt* stmt)
{
    LValue expr = translateExpr(stmt->Ist.Put.data);
    m_output->buildStoreArgIndex(expr, stmt->Ist.Put.offset / sizeof(intptr_t));
    return true;
}

bool VexTranslatorImpl::translatePutI(IRStmt* stmt)
{
    IRPutI* puti = stmt->Ist.PutI.details;
    LValue offset = genGuestArrayOffset(puti->descr,
        puti->ix, puti->bias);
    IRType ty = typeOfIRExpr(m_bb->tyenv, puti->data);
    LValue val = translateExpr(puti->data);
    LValue pointer = m_output->buildArgBytePointer();
    LValue afterOffset = m_output->buildAdd(pointer, offset);
    ensureType(val, ty);
    switch (ty) {
    case Ity_F64: {
        LValue casted = m_output->buildCast(LLVMBitCast, afterOffset, m_output->repo().refDouble);
        m_output->buildStore(val, casted);
    } break;
    case Ity_I8: {
        LValue casted = m_output->buildCast(LLVMBitCast, afterOffset, m_output->repo().ref8);
        m_output->buildStore(val, casted);
    } break;
    case Ity_I64: {
        LValue casted = m_output->buildCast(LLVMBitCast, afterOffset, m_output->repo().ref64);
        m_output->buildStore(val, casted);
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
    ensureType(val, typeOfIRExpr(m_bb->tyenv, stmt->Ist.WrTmp.data));
    ensureType(val, typeOfIRTemp(m_bb->tyenv, tmp));
    m_tmpValMap[tmp] = val;
    return true;
}

bool VexTranslatorImpl::translateExit(IRStmt* stmt)
{
    LValue guard = translateExpr(stmt->Ist.Exit.guard);
    LBasicBlock bbt = m_output->appendBasicBlock("taken");
    LBasicBlock bbnt = m_output->appendBasicBlock("not_taken");
    m_output->buildCondBr(guard, bbt, bbnt);
    m_output->positionToBBEnd(bbt);
    IRConst* cdst = stmt->Ist.Exit.dst;
    LValue val = m_output->constIntPtr(static_cast<uint64_t>(cdst->Ico.U64));

    if (stmt->Ist.Exit.jk == Ijk_Boring) {
        if (m_chainingAllow) {
            bool toFastEP
                = ((Addr64)cdst->Ico.U64) > m_env->m_maxga;
            if (toFastEP)
                m_output->buildDirectPatch(val);
            else
                m_output->buildDirectSlowPatch(val);
        }
        else
            m_output->buildAssistPatch(val);
        goto end;
    }

    /* Case: assisted transfer to arbitrary address */
    switch (stmt->Ist.Exit.jk) {
    /* Keep this list in sync with that in iselNext below */
    case Ijk_ClientReq:
    case Ijk_EmWarn:
    case Ijk_NoDecode:
    case Ijk_NoRedir:
    case Ijk_SigSEGV:
    case Ijk_SigTRAP:
    case Ijk_Sys_syscall:
    case Ijk_InvalICache:
    case Ijk_Yield: {
        m_output->buildAssistPatch(val);
        goto end;
    }
    default:
        break;
    }
    EMASSERT("unsurpported IREXIT" && false);
end:
    m_output->positionToBBEnd(bbnt);
    return true;
}

bool VexTranslatorImpl::translateStore(IRStmt* stmt)
{
    auto&& store = stmt->Ist.Store;
    EMASSERT(store.end == Iend_LE);
    LValue addr = translateExpr(store.addr);
    LValue data = translateExpr(store.data);
    // EMASSERT(jit::getElementType(jit::typeOf(addr)) == jit::typeOf(data));
    LValue castAddr = m_output->buildCast(LLVMBitCast, addr, jit::pointerType(jit::typeOf(data)));
    m_output->buildStore(data, castAddr);
    return true;
}

bool VexTranslatorImpl::translateStoreG(IRStmt* stmt)
{
    IRStoreG* details = stmt->Ist.StoreG.details;
    EMASSERT(details->end == Iend_LE);
    LValue addr = translateExpr(details->addr);
    LValue data = translateExpr(details->data);
    EMASSERT(jit::getElementType(jit::typeOf(addr)) == jit::typeOf(data));
    LValue guard = translateExpr(details->guard);
    LBasicBlock bbt = m_output->appendBasicBlock("taken");
    LBasicBlock bbnt = m_output->appendBasicBlock("not_taken");
    m_output->buildCondBr(guard, bbt, bbnt);
    m_output->positionToBBEnd(bbt);
    m_output->buildStore(data, addr);
    m_output->buildBr(bbnt);
    m_output->positionToBBEnd(bbnt);
}

bool VexTranslatorImpl::translateLoadG(IRStmt* stmt)
{
    IRLoadG* details = stmt->Ist.LoadG.details;
    EMASSERT(details->end == Iend_LE);
    LValue addr = translateExpr(details->addr);
    LValue alt = translateExpr(details->alt);
    LValue guard = translateExpr(details->guard);
    LBasicBlock bbt = m_output->appendBasicBlock("taken");
    LBasicBlock bbnt = m_output->appendBasicBlock("not_taken");
    m_output->buildCondBr(guard, bbt, bbnt);
    LBasicBlock original = m_output->current();
    m_output->positionToBBEnd(bbt);
    LValue dataBeforCast = m_output->buildLoad(m_output->buildPointerCast(addr, m_output->repo().ref32));
    LValue dataAfterCast;
    // do casting
    switch (details->cvt) {
    case ILGop_Ident32:
        EMASSERT(typeOf(dataBeforCast) == m_output->repo().int32);
        dataAfterCast = dataAfterCast;
        break;
    case ILGop_16Sto32:
    case ILGop_16Uto32: {
        EMASSERT(typeOf(dataBeforCast) == m_output->repo().int16);
        dataAfterCast = m_output->buildCast(details->cvt == ILGop_16Uto32 ? LLVMZExt : LLVMSExt, dataBeforCast, m_output->repo().int32);
        break;
    case ILGop_8Uto32:
    case ILGop_8Sto32:
        EMASSERT(typeOf(dataBeforCast) == m_output->repo().int8);
        dataAfterCast = m_output->buildCast(details->cvt == ILGop_8Uto32 ? LLVMZExt : LLVMSExt, dataBeforCast, m_output->repo().int32);
        break;
    }
    }
    m_output->buildBr(bbnt);
    m_output->positionToBBEnd(bbnt);
    LValue phi = m_output->buildPhi(m_output->repo().int32);
    jit::addIncoming(phi, &dataAfterCast, &bbt, 1);
    jit::addIncoming(phi, &alt, &original, 1);
    m_tmpValMap[details->dst] = phi;
}

void VexTranslatorImpl::_ensureType(jit::LValue val, IRType type)
{
    switch (type) {
    case Ity_I1:
        EMASSERT(m_output->typeOf(val) == m_output->repo().int1);
    case Ity_I8:
        EMASSERT(m_output->typeOf(val) == m_output->repo().int8);
    case Ity_I16:
        EMASSERT(m_output->typeOf(val) == m_output->repo().int16);
    case Ity_I32:
        EMASSERT(m_output->typeOf(val) == m_output->repo().int32);
    case Ity_I64:
        EMASSERT(m_output->typeOf(val) == m_output->repo().int64);
    case Ity_I128:
        EMASSERT(m_output->typeOf(val) == m_output->repo().int128);
    case Ity_D32:
    case Ity_F32:
        EMASSERT(m_output->typeOf(val) == m_output->repo().floatType);
    case Ity_D64:
    case Ity_F64:
        EMASSERT(m_output->typeOf(val) == m_output->repo().doubleType);
    case Ity_D128:
    case Ity_F128:
        EMASSERT("AMD64 not support F128 D128." && false);
    case Ity_V128:
        EMASSERT(m_output->typeOf(val) == m_output->repo().v128Type);
    case Ity_V256:
        EMASSERT(m_output->typeOf(val) == m_output->repo().v256Type);
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
        if (m_chainingAllow) {

            bool toFastEP
                = ((Addr64)cdst->Ico.U64) > m_env->m_maxga;
            if (toFastEP)
                m_output->buildDirectPatch(val);
            else
                m_output->buildDirectSlowPatch(val);
        }
        else
            m_output->buildAssistPatch(val);
        return true;
    }

    /* Case: call/return (==boring) transfer to any address */
    switch (jk) {
    case Ijk_Boring:
    case Ijk_Ret:
    case Ijk_Call: {
        if (m_chainingAllow) {
            m_output->buildIndirectPatch(val);
        }
        else {
            m_output->buildAssistPatch(val);
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
        m_output->buildAssistPatch(val);
        return true;
    }
    default:
        break;
    }

    vex_printf("\n-- PUT(%d) = ", m_bb->offsIP);
    ppIRExpr(next);
    vex_printf("; exit-");
    ppIRJumpKind(jk);
    vex_printf("\n");
    vassert(0); // are we expecting any other kind?
}

jit::LValue VexTranslatorImpl::translateExpr(IRExpr* expr)
{
    switch (expr->tag) {
    case Iex_RdTmp: {
        return translateRdTmp(expr);
    }
    case Iex_Const: {
        return translateConst(expr);
    }
    case Iex_Get: {
        return translateGet(expr);
    }
    case Iex_Load: {
        return translateLoad(expr);
    }
    }
    EMASSERT("not supported expr" && false);
}

jit::LValue VexTranslatorImpl::translateRdTmp(IRExpr* expr)
{
    auto found = m_tmpValMap.find(expr->Iex.RdTmp.tmp);
    EMASSERT(found != m_tmpValMap.end());
    return found->second;
}

jit::LValue VexTranslatorImpl::translateConst(IRExpr* expr)
{
    IRConst* myconst = expr->Iex.Const.con;
    switch (myconst->tag) {
    case Ico_U1:
        return m_output->constInt1(myconst->Ico.U1);
    case Ico_U8:
        return m_output->constInt8(myconst->Ico.U8);
    case Ico_U16:
        return m_output->constInt16(myconst->Ico.U16);
    case Ico_U32:
        return m_output->constInt32(myconst->Ico.U32);
    case Ico_U64:
        return m_output->constInt64(myconst->Ico.U64);
    case Ico_F32:
        return m_output->constFloat(myconst->Ico.F32);
    case Ico_F64:
        return m_output->constFloat(myconst->Ico.F64);
    case Ico_F32i: {
        jit::LValue val = m_output->constInt32(myconst->Ico.F32i);
        return m_output->buildCast(LLVMBitCast, val, m_output->repo().floatType);
    }
    case Ico_F64i: {
        jit::LValue val = m_output->constInt64(myconst->Ico.F64i);
        return m_output->buildCast(LLVMBitCast, val, m_output->repo().doubleType);
    }
    case Ico_V128:
        return m_output->constV128(myconst->Ico.V128);

    case Ico_V256:
        return m_output->constV256(myconst->Ico.V256);
    }
    EMASSERT("not supported constant" && false);
}

jit::LValue VexTranslatorImpl::translateGet(IRExpr* expr)
{
    EMASSERT(expr->Iex.Get.ty == Ity_I64);
    LValue beforeCast = m_output->buildLoadArgIndex(expr->Iex.Get.offset / sizeof(intptr_t));
    return beforeCast;
}

jit::LValue VexTranslatorImpl::translateLoad(IRExpr* expr)
{
    auto&& load = expr->Iex.Load;
    LValue addr = translateExpr(load.addr);
    LValue pointer = castToPointer(load.ty, addr);
    return m_output->buildLoad(pointer);
}

LValue VexTranslatorImpl::castToPointer(IRType irtype, LValue p)
{
    LType type;
    switch (irtype) {
    case Ity_I8:
        type = m_output->repo().ref8;
        break;
    case Ity_I16:
        type = m_output->repo().ref16;
        break;
    case Ity_I32:
        type = m_output->repo().ref32;
        break;
    case Ity_I64:
        type = m_output->repo().ref64;
        break;
    default:
        EMASSERT("unsupported type.");
        EMUNREACHABLE();
    }
    return m_output->buildCast(LLVMBitCast, p, type);
}
}

namespace jit {
VexTranslator* VexTranslator::create()
{
    return new VexTranslatorImpl;
}
}
