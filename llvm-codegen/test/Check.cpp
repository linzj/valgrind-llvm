#include "Check.h"
#include "RegisterOperation.h"
#include <sstream>
#include <iomanip>
Check::Check() {}
Check::~Check() {}
namespace {
class CheckRegisterEqConst : public Check {
public:
    CheckRegisterEqConst(const char* name, unsigned long long val)
        : m_name(name)
        , m_val(val)
    {
    }

private:
    virtual bool check(const VexGuestState* state, const uintptr_t*, std::string& info) const override
    {
        RegisterOperation op;
        const uintptr_t* p = op.getRegisterPointer(state, m_name);
        std::ostringstream oss;
        oss << "CheckRegisterEqConst " << ((*p == m_val) ? "PASSED" : "FAILED")
            << "; m_name = " << m_name
            << "; m_val = " << std::hex
            << m_val;
        info = oss.str();
        return *p == m_val;
    }
    std::string m_name;
    unsigned long long m_val;
};

class CheckRegisterEq : public Check {
public:
    CheckRegisterEq(const char* name1, const char* name2)
        : m_name1(name1)
        , m_name2(name2)
    {
    }

private:
    virtual bool check(const VexGuestState* state, const uintptr_t*, std::string& info) const override
    {
        RegisterOperation op;
        const uintptr_t* p1 = op.getRegisterPointer(state, m_name1);
        const uintptr_t* p2 = op.getRegisterPointer(state, m_name2);
        std::ostringstream oss;
        oss << "CheckRegisterEq " << ((*p1 == *p2) ? "PASSED" : "FAILED")
            << "; m_name1 = " << m_name1
            << "; m_name2 = " << m_name2;
        info = oss.str();
        return *p1 == *p2;
    }
    std::string m_name1;
    std::string m_name2;
};

class CheckState : public Check {
public:
    CheckState(unsigned long val1, unsigned long val2)
        : m_val1(val1)
        , m_val2(val2)
    {
    }

private:
    virtual bool check(const VexGuestState*, const uintptr_t* w, std::string& info) const override
    {
        std::ostringstream oss;
        oss << "CheckState " << ((w[0] == m_val1 && w[1] == m_val2) ? "PASSED" : "FAILED")
            << "; m_val1 = " << std::hex << m_val1
            << "; m_val2 = " << std::hex << m_val2
            << "; w[0] = " << std::hex << w[0]
            << "; w[1] = " << std::hex << w[1];
        info = oss.str();
        return (w[0] == m_val1 && w[1] == m_val2);
    }
    unsigned long long m_val1;
    unsigned long long m_val2;
};
}

std::unique_ptr<Check> Check::createCheckRegisterEqConst(const char* name, unsigned long long val)
{
    return std::unique_ptr<Check>(new CheckRegisterEqConst(name, val));
}

std::unique_ptr<Check> Check::createCheckRegisterEq(const char* name1, const char* name2)
{
    return std::unique_ptr<Check>(new CheckRegisterEq(name1, name2));
}

std::unique_ptr<Check> Check::createCheckState(unsigned long long val1, unsigned long long val2)
{
    return std::unique_ptr<Check>(new CheckState(val1, val2));
}
