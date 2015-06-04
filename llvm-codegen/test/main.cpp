#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <string>
extern "C" {
#include <libvex.h>
#include <libvex_ir.h>
#include <VEX/priv/main_util.h>
#include <VEX/priv/host_generic_regs.h>
#include <libvex_trc_values.h>
#include <libvex_guest_amd64.h>
}
#include "IRContextInternal.h"
#include "log.h"

extern "C" {
void yyparse(IRContext*);
typedef void* yyscan_t;
int yylex_init(yyscan_t* scanner);

int yylex_init_extra(struct IRContext* user_defined, yyscan_t* scanner);

int yylex_destroy(yyscan_t yyscanner);
void vexSetAllocModeTEMP_and_clear(void);
}
static void failure_exit(void) __attribute__((noreturn));

static void log_bytes(HChar*, Int nbytes);

void failure_exit(void)
{
    LOGE("failure and exit.\n");
    exit(1);
}

void log_bytes(HChar* bytes, Int nbytes)
{
    static std::string s;
    s.append(bytes, nbytes);
    size_t found;
    while ((found = s.find('\n')) != std::string::npos) {
        LOGE("%s.\n", s.substr(0, found).c_str());
        s = s.substr(found + 1);
    }
}

int main()
{
    VexControl clo_vex_control;
    LibVEX_default_VexControl(&clo_vex_control);
    clo_vex_control.iropt_unroll_thresh = 400;
    clo_vex_control.guest_max_insns = 100;
    clo_vex_control.guest_chase_thresh = 99;
    clo_vex_control.iropt_register_updates = VexRegUpdSpAtMemAccess;

    // use define control to init vex.
    LibVEX_Init(&failure_exit, &log_bytes,
        1, /* debug_paranoia */
        False,
        &clo_vex_control);
    IRContextInternal context;
    vexSetAllocModeTEMP_and_clear();
    yylex_init_extra(&context, &context.m_scanner);
    yyparse(&context);
    yylex_destroy(context.m_scanner);
    Bool (*isMove)(HInstr*, HReg*, HReg*);
    void (*getRegUsage)(HRegUsage*, HInstr*, Bool);
    void (*mapRegs)(HRegRemap*, HInstr*, Bool);
    void (*genSpill)(HInstr**, HInstr**, HReg, Int, Bool);
    void (*genReload)(HInstr**, HInstr**, HReg, Int, Bool);
    HInstr* (*directReload)(HInstr*, HReg, Short);
    void (*ppInstr)(HInstr*, Bool);
    void (*ppReg)(HReg);
    HInstrArray* (*iselSB)(IRSB*, VexArch, VexArchInfo*, VexAbiInfo*,
        Int, Int, Bool, Bool, Addr64);
    Int (*emit)(/*MB_MOD*/ Bool*,
        UChar*, Int, HInstr*, Bool, VexEndness,
        void*, void*, void*, void*);
    IRExpr* (*specHelper)(const HChar*, IRExpr**, IRStmt**, Int);
    Bool (*preciseMemExnsFn)(Int, Int);
    return 0;
}
