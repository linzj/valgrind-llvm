#ifndef PLATFORMDESC_H
#define PLATFORMDESC_H

struct PlatformDesc {
    size_t m_contextSize;
    size_t m_pcFieldOffset;
    size_t m_prologueSize;
    size_t m_directSize;
    size_t m_indirectSize;
    size_t m_assistSize;
    void* m_opaque;
    void (*m_patchPrologue)(void* opaque, uint8_t* start, uint8_t* end);
    void (*m_patchDirect)(void* opaque, uint8_t* toFill);
    void (*m_patchIndirect)(void* opaque, uint8_t* toFill);
    void (*m_patchAssist)(void* opaque, uint8_t* toFill);
};

#endif /* PLATFORMDESC_H */
