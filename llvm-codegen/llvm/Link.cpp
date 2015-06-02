#include <assert.h>
#include "StackMaps.h"
#include "CompilerState.h"
#include "Abbreviations.h"
#include "Link.h"

namespace jit {

void link(CompilerState& state)
{
    StackMaps sm;
    DataView dv(state.m_stackMapsSection->data());
    sm.parse(&dv);
    auto rm = sm.computeRecordMap();
    assert(state.m_codeSectionList.size() == 1);
    uint8_t* prologue = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(state.m_codeSectionList.front().data()));
    uint8_t* body = static_cast<uint8_t*>(state.m_entryPoint);
    PlatformDesc& platformDesc = state.m_platformDesc;
    state.m_platformDesc.m_patchPrologue(platformDesc.m_opaque, prologue, body);
    for (auto& record : rm) {
        assert(record.second.size() == 1);
        auto found = state.m_patchMap.find(record.first);
        assert(found != state.m_patchMap.end());
        PatchDesc& patchDesc = found->second;
        switch (patchDesc.m_type) {
        case PatchType::Direct: {
            platformDesc.m_patchDirect(platformDesc.m_opaque, body + record.second[0].instructionOffset);
        } break;
        case PatchType::Indirect: {
            platformDesc.m_patchIndirect(platformDesc.m_opaque, body + record.second[0].instructionOffset);
        } break;
        default:
            __builtin_unreachable();
        }
    }
}
}
