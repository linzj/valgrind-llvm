#ifndef VEXTRANSLATOR_H
#define VEXTRANSLATOR_H
#include "VexHeaders.h"

class VexTranslator
{
public:
    VexTranslator();
    virtual ~VexTranslator();

    VexTranslator(const VexTranslator&) = delete;
    const VexTranslator& operator= (const VexTranslator&) = delete;

    virtual bool translate(IRSB*) = 0;
    inline const void* code() const { return m_code; }
    inline size_t code_size() const { return m_codeSize; }
    static VexTranslator* create();
private:
    const void* m_code;
    size_t m_codeSize;
};

#endif /* VEXTRANSLATOR_H */
