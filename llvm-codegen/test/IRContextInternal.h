#ifndef IRCONTEXTINTERNAL_H
#define IRCONTEXTINTERNAL_H
extern "C" {
#include <libvex_ir.h>
}
#include <vector>
#include <stdint.h>
#include "IRContext.h"

struct IRContextInternal : IRContext {
    typedef std::vector<IRStmt*> StatmentVector;
    StatmentVector m_statments;
};

#endif /* IRCONTEXTINTERNAL_H */
