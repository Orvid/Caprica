#include <papyrus/PapyrusIdentifier.h>

#include <papyrus/PapyrusFunctionParameter.h>
#include <papyrus/PapyrusProperty.h>
#include <papyrus/PapyrusVariable.h>
#include <papyrus/statements/PapyrusDeclareStatement.h>

namespace caprica { namespace papyrus {

pex::PexValue PapyrusIdentifier::generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr, pex::PexValue base) const {
  namespace op = caprica::pex::op;
  switch (type) {
    case PapyrusIdentifierType::Property:
      // TODO: Support properties on external objects.
      if (base.type != pex::PexValueType::None)
        throw std::runtime_error("Not yet implemented!");
      if (prop->isAuto) {
        return pex::PexValue::Identifier(file->getString(prop->getAutoVarName()));
      } else {
        auto ret = bldr.allocTemp(file, resultType());
        bldr << op::propget{ file->getString(prop->name), pex::PexValue::Identifier(file->getString("self")), ret };
        return ret;
      }
    case PapyrusIdentifierType::Variable:
      return pex::PexValue::Identifier(file->getString(var->name));
    case PapyrusIdentifierType::Parameter:
      return pex::PexValue::Identifier(file->getString(param->name));
    case PapyrusIdentifierType::DeclareStatement:
      return pex::PexValue::Identifier(file->getString(declStatement->name));

    case PapyrusIdentifierType::Unresolved:
      throw std::runtime_error("Attempted to generate a load of an unresolved identifier '" + name + "'!");
    default:
      throw std::runtime_error("Unknown PapyrusIdentifierType!");
  }
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

    case PapyrusIdentifierType::Unresolved:
      throw std::runtime_error("Attempted to get the result type of an unresolved identifier '" + name + "'!");
    default:
      throw std::runtime_error("Unknown PapyrusIdentifierType!");
  }
}

}}
