#include <papyrus/PapyrusIdentifier.h>

#include <common/CapricaConfig.h>

#include <papyrus/PapyrusFunctionParameter.h>
#include <papyrus/PapyrusObject.h>
#include <papyrus/PapyrusProperty.h>
#include <papyrus/PapyrusStructMember.h>
#include <papyrus/PapyrusVariable.h>
#include <papyrus/statements/PapyrusDeclareStatement.h>

namespace caprica { namespace papyrus {

PapyrusIdentifier::PapyrusIdentifier(const Property& other)
  : type(PapyrusIdentifierType::Property),
    location(other.location),
    prop(other.prop),
    name(other.prop->name) { }
PapyrusIdentifier::PapyrusIdentifier(const Variable& other)
  : type(PapyrusIdentifierType::Variable),
    location(other.location),
    var(other.var),
    name(other.var->name) { }
PapyrusIdentifier::PapyrusIdentifier(const FunctionParameter& other)
  : type(PapyrusIdentifierType::Parameter),
    location(other.location),
    param(other.param),
    name(other.param->name) { }
PapyrusIdentifier::PapyrusIdentifier(const DeclStatement& other)
  : type(PapyrusIdentifierType::DeclareStatement),
    location(other.location),
    declStatement(other.declStatement),
    name(other.declStatement->name) { }
PapyrusIdentifier::PapyrusIdentifier(const StructMember& other)
  : type(PapyrusIdentifierType::StructMember),
    location(other.location),
    structMember(other.member),
    name(other.member->name) { }
PapyrusIdentifier::PapyrusIdentifier(const Function& other)
  : type(PapyrusIdentifierType::Function),
    location(other.location),
    func(other.function),
    name(other.function->name) { }
PapyrusIdentifier::PapyrusIdentifier(const ArrayFunction& other)
  : type(PapyrusIdentifierType::BuiltinArrayFunction),
    location(other.location),
    arrayFuncKind(other.arrayFuncKind),
    arrayFuncElementType(std::make_shared<PapyrusType>(other.arrayFuncElementType)) { }

pex::PexValue PapyrusIdentifier::generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr, pex::PexValue::Identifier base) const {
  namespace op = caprica::pex::op;
  switch (type) {
    case PapyrusIdentifierType::Property:
      if (CapricaConfig::enableCKOptimizations && prop->isAuto() && !prop->isReadOnly() && !base.tmpVar && file->getStringValue(base.name) == "self") {
        // We can only do this for properties on ourselves. (CK does this even on parents)
        return pex::PexValue::Identifier(file->getString(prop->getAutoVarName()));
      } else {
        auto ret = bldr.allocTemp(resultType());
        bldr << op::propget{ file->getString(prop->name), base, ret };
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
      auto ret = bldr.allocTemp(resultType());
      bldr << op::structget{ ret, base, file->getString(structMember->name) };
      return ret;
    }

    case PapyrusIdentifierType::Unresolved:
      CapricaError::fatal(location, "Attempted to generate a load of an unresolved identifier '%s'!", name.c_str());
  }
  CapricaError::logicalFatal("Unknown PapyrusIdentifierType!");
}

void PapyrusIdentifier::generateStore(pex::PexFile* file, pex::PexFunctionBuilder& bldr, pex::PexValue::Identifier base, pex::PexValue val) const {
  namespace op = caprica::pex::op;
  switch (type) {
    case PapyrusIdentifierType::Property:
      if (prop->isReadOnly())
        CapricaError::fatal(location, "Attempted to generate a store to a read-only property!");
      if (CapricaConfig::enableCKOptimizations && prop->isAuto() && !prop->isReadOnly() && !base.tmpVar && file->getStringValue(base.name) == "self") {
        // We can only do this for properties on ourselves. (CK does this even on parents)
        bldr << op::assign{ pex::PexValue::Identifier(file->getString(prop->getAutoVarName())), val };
      } else {
        bldr << op::propset{ file->getString(prop->name), base, val };
      }
      return;
    case PapyrusIdentifierType::Variable:
      bldr << op::assign{ pex::PexValue::Identifier(file->getString(var->name)), val };
      return;
    case PapyrusIdentifierType::Parameter:
      bldr << op::assign{ pex::PexValue::Identifier(file->getString(prop->name)), val };
      return;
    case PapyrusIdentifierType::DeclareStatement:
      bldr << op::assign{ pex::PexValue::Identifier(file->getString(declStatement->name)), val };
      return;
    case PapyrusIdentifierType::StructMember:
      bldr << op::structset{ base, file->getString(structMember->name), val };
      return;

    case PapyrusIdentifierType::Unresolved:
      CapricaError::fatal(location, "Attempted to generate a store to an unresolved identifier '%s'!", name.c_str());
  }
  CapricaError::logicalFatal("Unknown PapyrusIdentifierType!");
}

void PapyrusIdentifier::ensureAssignable() const {
  switch (type) {
    case PapyrusIdentifierType::Property:
      if (prop->isReadOnly())
        return CapricaError::error(location, "You cannot assign to the read-only property '%s'.", prop->name.c_str());
      if (prop->isConst())
        return CapricaError::error(location, "You cannot assign to the const property '%s'.", prop->name.c_str());
      if (prop->parent->isConst())
        return CapricaError::error(location, "You cannot assign to the '%s' property because the parent script '%s' is marked as const.", prop->name.c_str(), prop->parent->name.c_str());
      return;
    case PapyrusIdentifierType::Variable:
      if (var->isConst())
        return CapricaError::error(location, "You cannot assign to the const variable '%s'.", var->name.c_str());
      if (var->parent->isConst())
        return CapricaError::error(location, "You cannot assign to the variable '%s' because the parent script '%s' is marked as const.", var->name.c_str(), var->parent->name.c_str());
      return;
    case PapyrusIdentifierType::StructMember:
      if (structMember->isConst())
        return CapricaError::error(location, "You cannot assign to the '%s' member of a '%s' struct because it is marked as const.", structMember->name.c_str(), structMember->parent->name.c_str());
      return;

    case PapyrusIdentifierType::Parameter:
    case PapyrusIdentifierType::DeclareStatement:
      return;

    case PapyrusIdentifierType::Unresolved:
      return;
  }
  CapricaError::logicalFatal("Unknown PapyrusIdentifierType!");
}

void PapyrusIdentifier::markRead() {
  switch (type) {
    case PapyrusIdentifierType::Variable:
      var->referenceState.isRead = true;
      return;

    case PapyrusIdentifierType::Property:
    case PapyrusIdentifierType::Parameter:
    case PapyrusIdentifierType::DeclareStatement:
    case PapyrusIdentifierType::StructMember:
      return;

    case PapyrusIdentifierType::Unresolved:
      CapricaError::fatal(location, "Attempted to assign to an unresolved identifier '%s'!", name.c_str());
  }
  CapricaError::logicalFatal("Unknown PapyrusIdentifierType!");
}

void PapyrusIdentifier::markWritten() {
  switch (type) {
    case PapyrusIdentifierType::Variable:
      var->referenceState.isWritten = true;
      return;

    case PapyrusIdentifierType::Property:
    case PapyrusIdentifierType::Parameter:
    case PapyrusIdentifierType::DeclareStatement:
    case PapyrusIdentifierType::StructMember:
      return;

    case PapyrusIdentifierType::Unresolved:
      CapricaError::fatal(location, "Attempted to assign to an unresolved identifier '%s'!", name.c_str());
  }
  CapricaError::logicalFatal("Unknown PapyrusIdentifierType!");
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
      CapricaError::fatal(location, "Attempted to get the result type of an unresolved identifier '%s'!", name.c_str());
  }
  CapricaError::logicalFatal("Unknown PapyrusIdentifierType!");
}

}}
