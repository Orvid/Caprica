#include <papyrus/PapyrusIdentifier.h>

#include <common/CapricaConfig.h>

#include <papyrus/PapyrusFunctionParameter.h>
#include <papyrus/PapyrusObject.h>
#include <papyrus/PapyrusProperty.h>
#include <papyrus/PapyrusStructMember.h>
#include <papyrus/PapyrusVariable.h>
#include <papyrus/statements/PapyrusDeclareStatement.h>

namespace caprica { namespace papyrus {

PapyrusIdentifier PapyrusIdentifier::Property(CapricaFileLocation loc, const PapyrusProperty* p) {
  auto id = PapyrusIdentifier(PapyrusIdentifierType::Property, loc);
  id.prop = p;
  return id;
}
PapyrusIdentifier PapyrusIdentifier::Variable(CapricaFileLocation loc, const PapyrusVariable* v) {
  auto id = PapyrusIdentifier(PapyrusIdentifierType::Variable, loc);
  id.var = v;
  return id;
}
PapyrusIdentifier PapyrusIdentifier::FunctionParameter(CapricaFileLocation loc, const PapyrusFunctionParameter* p) {
  auto id = PapyrusIdentifier(PapyrusIdentifierType::Parameter, loc);
  id.param = p;
  return id;
}
PapyrusIdentifier PapyrusIdentifier::DeclStatement(CapricaFileLocation loc, const statements::PapyrusDeclareStatement* s) {
  auto id = PapyrusIdentifier(PapyrusIdentifierType::DeclareStatement, loc);
  id.declStatement = s;
  return id;
}
PapyrusIdentifier PapyrusIdentifier::StructMember(CapricaFileLocation loc, const PapyrusStructMember* m) {
  auto id = PapyrusIdentifier(PapyrusIdentifierType::StructMember, loc);
  id.structMember = m;
  return id;
}
PapyrusIdentifier PapyrusIdentifier::Function(CapricaFileLocation loc, const PapyrusFunction* f) {
  auto id = PapyrusIdentifier(PapyrusIdentifierType::Function, loc);
  id.func = f;
  return id;
}
PapyrusIdentifier PapyrusIdentifier::ArrayFunction(CapricaFileLocation loc, PapyrusBuiltinArrayFunctionKind fk, const PapyrusType& elemType) {
  auto id = PapyrusIdentifier(PapyrusIdentifierType::BuiltinArrayFunction, loc);
  id.arrayFuncKind = fk;
  id.arrayFuncElementType = std::make_shared<PapyrusType>(elemType);
  return id;
}

pex::PexValue PapyrusIdentifier::generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr, pex::PexValue::Identifier base) const {
  namespace op = caprica::pex::op;
  switch (type) {
    case PapyrusIdentifierType::Property:
      if (conf::CodeGeneration::enableCKOptimizations && prop->isAuto() && !prop->isAutoReadOnly() && !base.tmpVar && file->getStringValue(base.name) == "self") {
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
      CapricaReportingContext::logicalFatal("Invalid PapyrusIdentifierType!");
    case PapyrusIdentifierType::Unresolved:
      bldr.reportingContext.fatal(location, "Attempted to generate a load of an unresolved identifier '%s'!", name.to_string().c_str());
  }
  CapricaReportingContext::logicalFatal("Unknown PapyrusIdentifierType!");
}

void PapyrusIdentifier::generateStore(pex::PexFile* file, pex::PexFunctionBuilder& bldr, pex::PexValue::Identifier base, pex::PexValue val) const {
  namespace op = caprica::pex::op;
  switch (type) {
    case PapyrusIdentifierType::Property:
      if (prop->isAutoReadOnly())
        bldr.reportingContext.fatal(location, "Attempted to generate a store to a read-only property!");
      if (conf::CodeGeneration::enableCKOptimizations && prop->isAuto() && !prop->isAutoReadOnly() && !base.tmpVar && file->getStringValue(base.name) == "self") {
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
      CapricaReportingContext::logicalFatal("Invalid PapyrusIdentifierType!");
    case PapyrusIdentifierType::Unresolved:
      bldr.reportingContext.fatal(location, "Attempted to generate a store to an unresolved identifier '%s'!", name.to_string().c_str());
  }
  CapricaReportingContext::logicalFatal("Unknown PapyrusIdentifierType!");
}

void PapyrusIdentifier::ensureAssignable(CapricaReportingContext& repCtx) const {
  switch (type) {
    case PapyrusIdentifierType::Property:
      if (prop->isAutoReadOnly())
        return repCtx.error(location, "You cannot assign to the read-only property '%s'.", prop->name.to_string().c_str());
      if (prop->isConst())
        return repCtx.error(location, "You cannot assign to the const property '%s'.", prop->name.to_string().c_str());
      if (prop->parent->isConst())
        return repCtx.error(location, "You cannot assign to the '%s' property because the parent script '%s' is marked as const.", prop->name.to_string().c_str(), prop->parent->name.to_string().c_str());
      return;
    case PapyrusIdentifierType::Variable:
      if (var->isConst())
        return repCtx.error(location, "You cannot assign to the const variable '%s'.", var->name.to_string().c_str());
      if (var->parent->isConst())
        return repCtx.error(location, "You cannot assign to the variable '%s' because the parent script '%s' is marked as const.", var->name.to_string().c_str(), var->parent->name.to_string().c_str());
      return;
    case PapyrusIdentifierType::StructMember:
      if (structMember->isConst())
        return repCtx.error(location, "You cannot assign to the '%s' member of a '%s' struct because it is marked as const.", structMember->name.to_string().c_str(), structMember->parent->name.to_string().c_str());
      return;

    case PapyrusIdentifierType::DeclareStatement:
      if (declStatement->isConst)
        return repCtx.error(location, "You cannot assign to the const local variable '%s'.", declStatement->name.to_string().c_str());
      return;

    case PapyrusIdentifierType::Parameter:
    case PapyrusIdentifierType::BuiltinStateField:
      return;

    // This being unresolved has already been reported,
    // so don't give more errors.
    case PapyrusIdentifierType::Unresolved:
      return;

    case PapyrusIdentifierType::Function:
    case PapyrusIdentifierType::BuiltinArrayFunction:
      CapricaReportingContext::logicalFatal("Invalid PapyrusIdentifierType!");
  }
  CapricaReportingContext::logicalFatal("Unknown PapyrusIdentifierType!");
}

void PapyrusIdentifier::markRead() {
  switch (type) {
    case PapyrusIdentifierType::Variable:
      const_cast<PapyrusVariable*>(var)->referenceState.isRead = true;
      return;

    case PapyrusIdentifierType::Property:
    case PapyrusIdentifierType::Parameter:
    case PapyrusIdentifierType::DeclareStatement:
    case PapyrusIdentifierType::StructMember:
    case PapyrusIdentifierType::BuiltinStateField:
      return;

    // This being unresolved has already been reported,
    // so don't give more errors.
    case PapyrusIdentifierType::Unresolved:
      return;
    case PapyrusIdentifierType::Function:
    case PapyrusIdentifierType::BuiltinArrayFunction:
      CapricaReportingContext::logicalFatal("Invalid PapyrusIdentifierType!");
  }
  CapricaReportingContext::logicalFatal("Unknown PapyrusIdentifierType!");
}

void PapyrusIdentifier::markWritten() {
  switch (type) {
    case PapyrusIdentifierType::Variable:
      const_cast<PapyrusVariable*>(var)->referenceState.isWritten = true;
      return;

    case PapyrusIdentifierType::Property:
    case PapyrusIdentifierType::Parameter:
    case PapyrusIdentifierType::DeclareStatement:
    case PapyrusIdentifierType::StructMember:
    case PapyrusIdentifierType::BuiltinStateField:
      return;

    // This being unresolved has already been reported,
    // so don't give more errors.
    case PapyrusIdentifierType::Unresolved:
      return;

    case PapyrusIdentifierType::Function:
    case PapyrusIdentifierType::BuiltinArrayFunction:
      CapricaReportingContext::logicalFatal("Invalid PapyrusIdentifierType!");
  }
  CapricaReportingContext::logicalFatal("Unknown PapyrusIdentifierType!");
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

    // This being unresolved has already been reported,
    // so let errors chain off of the result being None.
    case PapyrusIdentifierType::Unresolved:
      return PapyrusType::None(location);

    case PapyrusIdentifierType::Function:
    case PapyrusIdentifierType::BuiltinArrayFunction:
      CapricaReportingContext::logicalFatal("Invalid PapyrusIdentifierType!");
  }
  CapricaReportingContext::logicalFatal("Unknown PapyrusIdentifierType!");
}

}}
