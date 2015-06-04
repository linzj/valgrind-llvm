#ifndef REGISTERASSIGN_H
#define REGISTERASSIGN_H
#include <memory>
class RegisterOperation;

class RegisterAssign {
public:
    RegisterAssign();
    ~RegisterAssign();
    void assign(VexGuestAMD64State* state, const char* registerName, unsigned long long val);

protected:
    uintptr_t* getRegisterPointer(VexGuestAMD64State* state, const char* registerName);
    std::unique_ptr<RegisterOperation> m_op;
};
#endif /* REGISTERASSIGN_H */
