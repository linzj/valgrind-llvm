#ifndef REGISTEROPERATION_H
#define REGISTEROPERATION_H
#include <unordered_map>
#include <string>
#include "VexHeaders.h"

class RegisterOperation {
public:
    RegisterOperation();
    uintptr_t* getRegisterPointer(VexGuestState* state, const std::string& registerName);
    const uintptr_t* getRegisterPointer(const VexGuestState* state, const std::string& registerName);

private:
    std::unordered_map<std::string, size_t> m_map;
};
#endif /* REGISTEROPERATION_H */
