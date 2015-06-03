#include <assert.h>
#include "StackMaps.h"
#include "CompilerState.h"
#include "Abbreviations.h"
#include "Link.h"

namespace jit {

void link(CompilerState& state, const LinkDesc& desc)
{
    StackMaps sm;
    DataView dv(state.m_stackMapsSection->data());
    sm.parse(&dv);
    auto rm = sm.computeRecordMap();
    assert(state.m_codeSectionList.size() == 1);
    uint8_t* prologue = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(state.m_codeSectionList.front().data()));
    uint8_t* body = static_cast<uint8_t*>(state.m_entryPoint);
    desc.m_patchPrologue(desc.m_opaque, prologue, body);
    for (auto& record : rm) {
        assert(record.second.size() == 1);
        auto found = state.m_patchMap.find(record.first);
        assert(found != state.m_patchMap.end());
        PatchDesc& patchDesc = found->second;
        switch (patchDesc.m_type) {
        case PatchType::Direct: {
            desc.m_patchDirect(desc.m_opaque, body + record.second[0].instructionOffset, desc.m_dispDirect);
        } break;
        case PatchType::DirectSlow: {
            desc.m_patchDirect(desc.m_opaque, body + record.second[0].instructionOffset, desc.m_dispDirectSlow);
        } break;
        case PatchType::Indirect: {
            desc.m_patchIndirect(desc.m_opaque, body + record.second[0].instructionOffset, desc.m_dispIndirect);
        } break;
        case PatchType::Assist: {
            desc.m_patchAssist(desc.m_opaque, body + record.second[0].instructionOffset, desc.m_dispAssist);
        } break;
        default:
            __builtin_unreachable();
        }
    }
}
}
