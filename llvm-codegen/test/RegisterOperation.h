#ifndef REGISTEROPERATION_H
#define REGISTEROPERATION_H
#include <unordered_map>
#include <string>
#include "VexHeaders.h"

class RegisterOperation {
public:
    RegisterOperation();
    uintptr_t* getRegisterPointer(VexGuestAMD64State* state, const char* registerName);

private:
    std::unordered_map<std::string, size_t> m_map;
};
#endif /* REGISTEROPERATION_H */
