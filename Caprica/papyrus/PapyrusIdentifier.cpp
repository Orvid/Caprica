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
  id.res.prop = p;
  return id;
}

PapyrusIdentifier PapyrusIdentifier::Guard(CapricaFileLocation loc, const PapyrusGuard* g) {
  auto id = PapyrusIdentifier(PapyrusIdentifierType::Guard, loc);
  id.res.guard = g;
  return id;
}

PapyrusIdentifier PapyrusIdentifier::Variable(CapricaFileLocation loc, const PapyrusVariable* v) {
  auto id = PapyrusIdentifier(PapyrusIdentifierType::Variable, loc);
  id.res.var = v;
  return id;
}
PapyrusIdentifier PapyrusIdentifier::FunctionParameter(CapricaFileLocation loc, const PapyrusFunctionParameter* p) {
  auto id = PapyrusIdentifier(PapyrusIdentifierType::Parameter, loc);
  id.res.param = p;
  return id;
}
PapyrusIdentifier PapyrusIdentifier::DeclStatement(CapricaFileLocation loc,
                                                   const statements::PapyrusDeclareStatement* s) {
  auto id = PapyrusIdentifier(PapyrusIdentifierType::DeclareStatement, loc);
  id.res.declStatement = s;
  return id;
}
PapyrusIdentifier PapyrusIdentifier::StructMember(CapricaFileLocation loc, const PapyrusStructMember* m) {
  auto id = PapyrusIdentifier(PapyrusIdentifierType::StructMember, loc);
  id.res.structMember = m;
  return id;
}
PapyrusIdentifier PapyrusIdentifier::Function(CapricaFileLocation loc, const PapyrusFunction* f) {
  auto id = PapyrusIdentifier(PapyrusIdentifierType::Function, loc);
  id.res.func = f;
  return id;
}
PapyrusIdentifier
PapyrusIdentifier::ArrayFunction(CapricaFileLocation loc, PapyrusBuiltinArrayFunctionKind fk, PapyrusType* elemType) {
  auto id = PapyrusIdentifier(PapyrusIdentifierType::BuiltinArrayFunction, loc);
  id.arrayFuncKind = fk;
  id.res.arrayFuncElementType = elemType;
  return id;
}

pex::PexValue PapyrusIdentifier::generateLoad(pex::PexFile* file,
                                              pex::PexFunctionBuilder& bldr,
                                              pex::PexValue::Identifier base) const {
  namespace op = caprica::pex::op;
  switch (type) {
    case PapyrusIdentifierType::Property:
      if (conf::CodeGeneration::enableCKOptimizations && res.prop->isAuto() && !res.prop->isAutoReadOnly() &&
          !base.tmpVar && file->getStringValue(base.name) == "self") {
        // We can only do this for properties on ourselves. (CK does this even on parents)
        return pex::PexValue::Identifier(file->getString(res.prop->autoVarName));
      } else {
        auto ret = bldr.allocTemp(resultType());
        bldr << op::propget { file->getString(res.prop->name), base, ret };
        return ret;
      }
    case PapyrusIdentifierType::Variable:
      return pex::PexValue::Identifier(file->getString(res.var->name));
    case PapyrusIdentifierType::Parameter:
      return pex::PexValue::Identifier(file->getString(res.param->name));
    case PapyrusIdentifierType::DeclareStatement:
      return pex::PexValue::Identifier(file->getString(res.declStatement->name));
    case PapyrusIdentifierType::StructMember: {
      auto ret = bldr.allocTemp(resultType());
      bldr << op::structget { ret, base, file->getString(res.structMember->name) };
      return ret;
    }
    case PapyrusIdentifierType::BuiltinStateField:
      return pex::PexValue::Identifier(file->getString("::State"));

    case PapyrusIdentifierType::Function:
    case PapyrusIdentifierType::BuiltinArrayFunction:
      CapricaReportingContext::logicalFatal("Invalid PapyrusIdentifierType!");
    case PapyrusIdentifierType::Unresolved:
      bldr.reportingContext.fatal(location,
                                  "Attempted to generate a load of an unresolved identifier '{}'!",
                                  res.name);
  }
  CapricaReportingContext::logicalFatal("Unknown PapyrusIdentifierType!");
}

void PapyrusIdentifier::generateStore(pex::PexFile* file,
                                      pex::PexFunctionBuilder& bldr,
                                      pex::PexValue::Identifier base,
                                      pex::PexValue val) const {
  namespace op = caprica::pex::op;
  switch (type) {
    case PapyrusIdentifierType::Property:
      if (res.prop->isAutoReadOnly())
        bldr.reportingContext.fatal(location, "Attempted to generate a store to a read-only property!");
      if (conf::CodeGeneration::enableCKOptimizations && res.prop->isAuto() && !res.prop->isAutoReadOnly() &&
          !base.tmpVar && file->getStringValue(base.name) == "self") {
        // We can only do this for properties on ourselves. (CK does this even on parents)
        bldr << op::assign { pex::PexValue::Identifier(file->getString(res.prop->autoVarName)), val };
      } else {
        bldr << op::propset { file->getString(res.prop->name), base, val };
      }
      return;
    case PapyrusIdentifierType::Variable:
      bldr << op::assign { pex::PexValue::Identifier(file->getString(res.var->name)), val };
      return;
    case PapyrusIdentifierType::Parameter:
      bldr << op::assign { pex::PexValue::Identifier(file->getString(res.prop->name)), val };
      return;
    case PapyrusIdentifierType::DeclareStatement:
      bldr << op::assign { pex::PexValue::Identifier(file->getString(res.declStatement->name)), val };
      return;
    case PapyrusIdentifierType::StructMember:
      bldr << op::structset { base, file->getString(res.structMember->name), val };
      return;
    case PapyrusIdentifierType::BuiltinStateField:
      bldr << op::assign { pex::PexValue::Identifier(file->getString("::State")), val };
      return;

    case PapyrusIdentifierType::Function:
    case PapyrusIdentifierType::BuiltinArrayFunction:
      CapricaReportingContext::logicalFatal("Invalid PapyrusIdentifierType!");
    case PapyrusIdentifierType::Unresolved:
      bldr.reportingContext.fatal(location,
                                  "Attempted to generate a store to an unresolved identifier '{}'!",
                                  res.name);
  }
  CapricaReportingContext::logicalFatal("Unknown PapyrusIdentifierType!");
}

void PapyrusIdentifier::ensureAssignable(CapricaReportingContext& repCtx) const {
  switch (type) {
    case PapyrusIdentifierType::Property:
      if (res.prop->isAutoReadOnly()) {
        return repCtx.error(location,
                            "You cannot assign to the read-only property '{}'.",
                            res.prop->name);
      }
      if (res.prop->isConst()) {
        return repCtx.error(location,
                            "You cannot assign to the const property '{}'.",
                            res.prop->name);
      }
      if (res.prop->parent->isConst()) {
        return repCtx.error(location,
                            "You cannot assign to the '{}' property because the parent script '{}' is marked as const.",
                            res.prop->name,
                            res.prop->parent->name);
      }
      return;
    case PapyrusIdentifierType::Variable:
      if (res.var->isConst()) {
        return repCtx.error(location,
                            "You cannot assign to the const variable '{}'.",
                            res.var->name);
      }
      if (res.var->parent->isConst()) {
        return repCtx.error(location,
                            "You cannot assign to the variable '{}' because the parent script '{}' is marked as const.",
                            res.var->name,
                            res.var->parent->name);
      }
      return;
    case PapyrusIdentifierType::StructMember:
      if (res.structMember->isConst()) {
        return repCtx.error(location,
                            "You cannot assign to the '{}' member of a '{}' struct because it is marked as const.",
                            res.structMember->name,
                            res.structMember->parent->name);
      }
      return;

    case PapyrusIdentifierType::DeclareStatement:
      if (res.declStatement->isConst) {
        return repCtx.error(location,
                            "You cannot assign to the const local variable '{}'.",
                            res.declStatement->name);
      }
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
      const_cast<PapyrusVariable*>(res.var)->referenceState.isRead = true;
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
      const_cast<PapyrusVariable*>(res.var)->referenceState.isWritten = true;
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
      return res.prop->type;
    case PapyrusIdentifierType::Variable:
      return res.var->type;
    case PapyrusIdentifierType::Parameter:
      return res.param->type;
    case PapyrusIdentifierType::DeclareStatement:
      return res.declStatement->type;
    case PapyrusIdentifierType::StructMember:
      return res.structMember->type;
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
