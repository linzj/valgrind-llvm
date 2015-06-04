#ifndef IRCONTEXTINTERNAL_H
#define IRCONTEXTINTERNAL_H
#include <vector>
#include <stdint.h>
#include "IRContext.h"
#include "RegisterInit.h"
#include "VexHeaders.h"

struct IRContextInternal : IRContext {
    typedef std::vector<IRStmt*> StatmentVector;
    typedef std::vector<RegisterInit> RegisterInitVector;
    StatmentVector m_statments;
    RegisterInitVector m_registerInit;
};

#endif /* IRCONTEXTINTERNAL_H */
