#include <papyrus/PapyrusIdentifier.h>

#include <common/CapricaConfig.h>

#include <papyrus/PapyrusFunctionParameter.h>
#include <papyrus/PapyrusObject.h>
#include <papyrus/PapyrusProperty.h>
#include <papyrus/PapyrusStructMember.h>
#include <papyrus/PapyrusVariable.h>
#include <papyrus/statements/PapyrusDeclareStatement.h>

namespace caprica { namespace papyrus {

PapyrusIdentifier PapyrusIdentifier::Property(const CapricaFileLocation& loc, PapyrusProperty* p) {
  auto id = PapyrusIdentifier(PapyrusIdentifierType::Property, loc);
  id.prop = p;
  id.name = p->name;
  return id;
}
PapyrusIdentifier PapyrusIdentifier::Variable(const CapricaFileLocation& loc, PapyrusVariable* v) {
  auto id = PapyrusIdentifier(PapyrusIdentifierType::Variable, loc);
  id.var = v;
  id.name = v->name;
  return id;
}
PapyrusIdentifier PapyrusIdentifier::FunctionParameter(const CapricaFileLocation& loc, PapyrusFunctionParameter* p) {
  auto id = PapyrusIdentifier(PapyrusIdentifierType::Parameter, loc);
  id.param = p;
  id.name = p->name;
  return id;
}
PapyrusIdentifier PapyrusIdentifier::DeclStatement(const CapricaFileLocation& loc, statements::PapyrusDeclareStatement* s) {
  auto id = PapyrusIdentifier(PapyrusIdentifierType::DeclareStatement, loc);
  id.declStatement = s;
  id.name = s->name;
  return id;
}
PapyrusIdentifier PapyrusIdentifier::StructMember(const CapricaFileLocation& loc, PapyrusStructMember* m) {
  auto id = PapyrusIdentifier(PapyrusIdentifierType::StructMember, loc);
  id.structMember = m;
  id.name = m->name;
  return id;
}
PapyrusIdentifier PapyrusIdentifier::Function(const CapricaFileLocation& loc, PapyrusFunction* f) {
  auto id = PapyrusIdentifier(PapyrusIdentifierType::Function, loc);
  id.func = f;
  id.name = f->name;
  return id;
}
PapyrusIdentifier PapyrusIdentifier::ArrayFunction(const CapricaFileLocation& loc, PapyrusBuiltinArrayFunctionKind fk, const PapyrusType& elemType) {
  auto id = PapyrusIdentifier(PapyrusIdentifierType::BuiltinArrayFunction, loc);
  id.arrayFuncKind = fk;
  id.arrayFuncElementType = std::make_shared<PapyrusType>(elemType);
  return id;
}

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
    case PapyrusIdentifierType::BuiltinStateField:
      return pex::PexValue::Identifier(file->getString("::State"));

    case PapyrusIdentifierType::Function:
    case PapyrusIdentifierType::BuiltinArrayFunction:
      CapricaError::logicalFatal("Invalid PapyrusIdentifierType!");
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
    case PapyrusIdentifierType::BuiltinStateField:
      bldr << op::assign{ pex::PexValue::Identifier(file->getString("::State")), val };
      return;

    case PapyrusIdentifierType::Function:
    case PapyrusIdentifierType::BuiltinArrayFunction:
      CapricaError::logicalFatal("Invalid PapyrusIdentifierType!");
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
    case PapyrusIdentifierType::BuiltinStateField:
      return;

    case PapyrusIdentifierType::Function:
    case PapyrusIdentifierType::BuiltinArrayFunction:
      CapricaError::logicalFatal("Invalid PapyrusIdentifierType!");
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
    case PapyrusIdentifierType::BuiltinStateField:
      return;

    case PapyrusIdentifierType::Function:
    case PapyrusIdentifierType::BuiltinArrayFunction:
      CapricaError::logicalFatal("Invalid PapyrusIdentifierType!");
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
    case PapyrusIdentifierType::BuiltinStateField:
      return;

    case PapyrusIdentifierType::Function:
    case PapyrusIdentifierType::BuiltinArrayFunction:
      CapricaError::logicalFatal("Invalid PapyrusIdentifierType!");
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
    case PapyrusIdentifierType::BuiltinStateField:
      return PapyrusType::String(location);

    case PapyrusIdentifierType::Function:
    case PapyrusIdentifierType::BuiltinArrayFunction:
      CapricaError::logicalFatal("Invalid PapyrusIdentifierType!");
    case PapyrusIdentifierType::Unresolved:
      CapricaError::fatal(location, "Attempted to get the result type of an unresolved identifier '%s'!", name.c_str());
  }
  CapricaError::logicalFatal("Unknown PapyrusIdentifierType!");
}

}}
