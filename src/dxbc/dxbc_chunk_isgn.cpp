#include "dxbc_chunk_isgn.h"

namespace dxvk {
  
  DxbcIsgn::DxbcIsgn(DxbcReader reader, DxbcTag tag) {
    uint32_t elementCount = reader.readu32();
    reader.skip(sizeof(uint32_t));
    
    std::array<DxbcScalarType, 4> componentTypes = {
      DxbcScalarType::Uint32, DxbcScalarType::Uint32,
      DxbcScalarType::Sint32, DxbcScalarType::Float32,
    };

    // https://github.com/DarkStarSword/3d-fixes/blob/master/dx11shaderanalyse.py#L101
    bool hasStream    = (tag == "ISG1") || (tag == "OSG1") || (tag == "PSG1") || (tag == "OSG5");
    bool hasPrecision = (tag == "ISG1") || (tag == "OSG1") || (tag == "PSG1");
    
    for (uint32_t i = 0; i < elementCount; i++) {
      DxbcSgnEntry entry;
      entry.streamId        = hasStream ? reader.readu32() : 0;
      entry.semanticName    = reader.clone(reader.readu32()).readString();
      entry.semanticIndex   = reader.readu32();
      entry.systemValue     = static_cast<DxbcSystemValue>(reader.readu32());
      entry.componentType   = componentTypes.at(reader.readu32());
      entry.registerId      = reader.readu32();
      entry.componentMask   = bit::extract(reader.readu32(), 0, 3);

      if (hasPrecision)
        reader.readu32();

      m_entries.push_back(entry);
    }
  }
  
  
  DxbcIsgn::~DxbcIsgn() {
    
  }
  
  
  const DxbcSgnEntry* DxbcIsgn::findByRegister(uint32_t registerId) const {
    for (const auto& e : *this) {
      if (e.registerId == registerId)
        return &e;
    }
    
    return nullptr;
  }
  
  
  const DxbcSgnEntry* DxbcIsgn::find(
    const std::string& semanticName,
          uint32_t     semanticIndex,
          uint32_t     streamId) const {
    for (const auto& e : *this) {
      if (e.semanticIndex == semanticIndex
       && e.streamId      == streamId
       && compareSemanticNames(semanticName, e.semanticName))
        return &e;
    }
    
    return nullptr;
  }
  
  
  DxbcRegMask DxbcIsgn::regMask(
          uint32_t     registerId) const {
    DxbcRegMask mask;

    for (const auto& e : *this) {
      if (e.registerId == registerId)
        mask |= e.componentMask;
    }

    return mask;
  }


  uint32_t DxbcIsgn::maxRegisterCount() const {
    uint32_t result = 0;
    for (const auto& e : *this)
      result = std::max(result, e.registerId + 1);
    return result;
  }

  void DxbcIsgn::printEntries() const {
    for (const auto& entry : *this) {
          Logger::debug(str::format("SGN Entry:\n\t",
            "semanticName: ",  entry.semanticName, "\n\t",
            "semanticIndex: ", entry.semanticIndex, "\n\t",
            "registerId: ",    entry.registerId, "\n\t",
            "componentMask: ", entry.componentMask.maskString(), "\n\t",
            "componentType: ", entry.componentType, "\n\t",
            "systemValue: ",   entry.systemValue, "\n\t",
            "streamId: ",      entry.streamId, "\n",
            "\n"));
    }
  }


  bool DxbcIsgn::compareSemanticNames(
    const std::string& a, const std::string& b) const {
    return a.size() == b.size() && _stricmp(a.c_str(), b.c_str()) == 0;
  }
  
}
