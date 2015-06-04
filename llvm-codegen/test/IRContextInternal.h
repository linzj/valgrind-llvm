#ifndef IRCONTEXTINTERNAL_H
#define IRCONTEXTINTERNAL_H
#include <vector>
#include <memory>
#include <stdint.h>
#include "IRContext.h"
#include "RegisterInit.h"
#include "VexHeaders.h"

struct IRContextInternal : IRContext {
    typedef std::vector<IRStmt*> StatmentVector;
    typedef std::vector<RegisterInit> RegisterInitVector;
    typedef std::vector<std::unique_ptr<class Check> > CheckVector;
    StatmentVector m_statments;
    RegisterInitVector m_registerInit;
    CheckVector m_checks;
};

#endif /* IRCONTEXTINTERNAL_H */
