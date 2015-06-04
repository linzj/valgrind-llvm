#include "log.h"
#include "RegisterOperation.h"
#include "RegisterAssign.h"

RegisterAssign::RegisterAssign()
    : m_op(new RegisterOperation)
{
}
RegisterAssign::~RegisterAssign() {}
void RegisterAssign::assign(VexGuestState* state, const std::string& registerName, unsigned long long val)
{
    uintptr_t* p = m_op->getRegisterPointer(state, registerName);
    EMASSERT(p != nullptr);
    *p = val;
}
