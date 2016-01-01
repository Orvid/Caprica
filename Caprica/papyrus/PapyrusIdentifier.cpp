#include <papyrus/PapyrusIdentifier.h>

#include <CapricaConfig.h>

#include <papyrus/PapyrusFunctionParameter.h>
#include <papyrus/PapyrusProperty.h>
#include <papyrus/PapyrusStructMember.h>
#include <papyrus/PapyrusVariable.h>
#include <papyrus/statements/PapyrusDeclareStatement.h>

namespace caprica { namespace papyrus {

pex::PexValue PapyrusIdentifier::generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr, pex::PexValue::Identifier base) const {
  namespace op = caprica::pex::op;
  switch (type) {
    case PapyrusIdentifierType::Property:
      if (CapricaConfig::enableCKOptimizations && prop->isAuto && !prop->isReadOnly && file->getStringValue(base.name) == "self") {
        // We can only do this for properties on ourselves. (CK does this even on parents)
        return pex::PexValue::Identifier(file->getString(prop->getAutoVarName()));
      } else {
        auto ret = bldr.allocTemp(file, resultType());
        bldr << op::propget{ file->getString(prop->name), base, ret };
        bldr.freeIfTemp(base);
        return ret;
      }
    case PapyrusIdentifierType::Variable:
      return pex::PexValue::Identifier(file->getString(var->name));
    case PapyrusIdentifierType::Parameter:
      return pex::PexValue::Identifier(file->getString(param->name));
    case PapyrusIdentifierType::DeclareStatement:
      return pex::PexValue::Identifier(file->getString(declStatement->name));
    case PapyrusIdentifierType::StructMember:
    {
      auto ret = bldr.allocTemp(file, resultType());
      bldr << op::structget{ ret, base, file->getString(structMember->name) };
      bldr.freeIfTemp(base);
      return ret;
    }

    case PapyrusIdentifierType::Unresolved:
      throw std::runtime_error("Attempted to generate a load of an unresolved identifier '" + name + "'!");
    default:
      throw std::runtime_error("Unknown PapyrusIdentifierType!");
  }
}

void PapyrusIdentifier::generateStore(pex::PexFile* file, pex::PexFunctionBuilder& bldr, pex::PexValue::Identifier base, pex::PexValue val) const {
  namespace op = caprica::pex::op;
  switch (type) {
    case PapyrusIdentifierType::Property:
      if (prop->isReadOnly)
        throw std::runtime_error("Attempted to generate a store to a read-only property!");
      if (CapricaConfig::enableCKOptimizations && prop->isAuto && !prop->isReadOnly && file->getStringValue(base.name) == "self") {
        // We can only do this for properties on ourselves. (CK does this even on parents)
        bldr << op::assign{ pex::PexValue::Identifier(file->getString(prop->getAutoVarName())), val };
      } else {
        bldr << op::propset{ file->getString(prop->name), base, val };
        bldr.freeIfTemp(base);
      }
      break;
    case PapyrusIdentifierType::Variable:
      bldr << op::assign{ pex::PexValue::Identifier(file->getString(var->name)), val };
      break;
    case PapyrusIdentifierType::Parameter:
      bldr << op::assign{ pex::PexValue::Identifier(file->getString(prop->name)), val };
      break;
    case PapyrusIdentifierType::DeclareStatement:
      bldr << op::assign{ pex::PexValue::Identifier(file->getString(declStatement->name)), val };
      break;
    case PapyrusIdentifierType::StructMember:
      bldr << op::structset{ base, file->getString(structMember->name), val };
      bldr.freeIfTemp(base);
      break;

    case PapyrusIdentifierType::Unresolved:
      throw std::runtime_error("Attempted to generate a store to an unresolved identifier '" + name + "'!");
    default:
      throw std::runtime_error("Unknown PapyrusIdentifierType!");
  }
  bldr.freeIfTemp(val);
}

PapyrusType PapyrusIdentifier::resultType() const {
  switch (type) {
    case PapyrusIdentifierType::Property:
      return prop->type;
    case PapyrusIdentifierType::Variable:
      return var->type;
    case PapyrusIdentifierType::Parameter:
      return param->type;
    case PapyrusIdentifierType::DeclareStatement:
      return declStatement->type;
    case PapyrusIdentifierType::StructMember:
      return structMember->type;

    case PapyrusIdentifierType::Unresolved:
      throw std::runtime_error("Attempted to get the result type of an unresolved identifier '" + name + "'!");
    default:
      throw std::runtime_error("Unknown PapyrusIdentifierType!");
  }
}

}}
