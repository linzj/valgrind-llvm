#ifndef REGISTERASSIGN_H
#define REGISTERASSIGN_H
#include <memory>
#include <string>
class RegisterOperation;

class RegisterAssign {
public:
    RegisterAssign();
    ~RegisterAssign();
    void assign(VexGuestState* state, const std::string& registerName, unsigned long long val);

protected:
    std::unique_ptr<RegisterOperation> m_op;
};
#endif /* REGISTERASSIGN_H */
