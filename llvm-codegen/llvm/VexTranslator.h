#ifndef VEXTRANSLATOR_H
#define VEXTRANSLATOR_H
#include "VexHeaders.h"

struct VexTranslatorEnv {
    bool m_chainingAllow;
};

class VexTranslator {
public:
    explicit VexTranslator(const VexTranslatorEnv&);
    virtual ~VexTranslator();

    VexTranslator(const VexTranslator&) = delete;
    const VexTranslator& operator=(const VexTranslator&) = delete;

    virtual bool translate(IRSB*) = 0;
    inline const void* code() const { return m_code; }
    inline size_t code_size() const { return m_codeSize; }
    inline const VexTranslator& env() const { return m_env; }
    static VexTranslator* create(const VexTranslatorEnv&);

private:
    const void* m_code;
    size_t m_codeSize;
    VexTranslatorEnv m_env;
};

#endif /* VEXTRANSLATOR_H */
