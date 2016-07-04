#include <pex/PexReflector.h>

#include <boost/utility/string_ref.hpp>

#include <pex/PexReader.h>

namespace caprica { namespace pex {

using namespace caprica::papyrus;

static PapyrusType reflectType(CapricaFileLocation loc, boost::string_ref name) {
  if (name.size() > 2 && name[name.size() - 2] == '[' && name[name.size() - 1] == ']')
    return PapyrusType::Array(loc, std::make_shared<PapyrusType>(reflectType(loc, name.substr(0, name.size() - 2))));

  if (idEq(name, "bool"))
    return PapyrusType::Bool(loc);
  if (idEq(name, "none"))
    return PapyrusType::None(loc);
  if (idEq(name, "int"))
    return PapyrusType::Int(loc);
  if (idEq(name, "float"))
    return PapyrusType::Float(loc);
  if (idEq(name, "string"))
    return PapyrusType::String(loc);
  if (idEq(name, "var"))
    return PapyrusType::Var(loc);

  return PapyrusType::Unresolved(loc, name);
}

static PapyrusType reflectType(CapricaFileLocation loc, PexFile* pex, PexString pexName) {
  return reflectType(loc, pex->getStringValue(pexName));
}

static PapyrusFunction* reflectFunction(CapricaFileLocation loc, PexFile* pex, PapyrusObject* obj, PexFunction* pFunc, boost::string_ref funcName) {
  auto func = new PapyrusFunction(loc, reflectType(loc, pex, pFunc->returnTypeName));
  func->parentObject = obj;
  func->name = funcName;
  func->userFlags.isGlobal = pFunc->isGlobal;
  func->userFlags.isNative = pFunc->isNative;

  for (auto pp : pFunc->parameters) {
    auto param = new PapyrusFunctionParameter(loc, reflectType(loc, pex, pp->type));
    param->name = pex->getStringValue(pp->name);
    func->parameters.emplace_back(param);
  }

  return func;
}

PapyrusScript* PexReflector::reflectScript(PexFile* pex) {
  CapricaFileLocation loc{ 0 };

  auto script = new PapyrusScript();
  script->sourceFileName = pex->sourceFileName;
  
  for (auto po : pex->objects) {
    PapyrusType baseTp = PapyrusType::None(loc);
    if (pex->getStringValue(po->parentClassName) != "")
      baseTp = reflectType(loc, pex, po->parentClassName);
    auto obj = new PapyrusObject(loc, baseTp);
    obj->name = pex->getStringValue(po->name);

    for (auto ps : po->structs) {
      auto struc = new PapyrusStruct(loc);
      struc->parentObject = obj;
      struc->name = pex->getStringValue(ps->name);
      for (auto pm : ps->members) {
        auto mem = new PapyrusStructMember(loc, reflectType(loc, pex, pm->typeName), struc);
        mem->userFlags.isConst = pm->isConst;
        mem->name = pex->getStringValue(pm->name);
        struc->members.push_back(mem);
      }
      obj->structs.push_back(struc);
    }

    for (auto pp : po->properties) {
      auto prop = new PapyrusProperty(loc, reflectType(loc, pex, pp->typeName), obj);
      prop->name = pex->getStringValue(pp->name);
      if (pp->isAuto) {
        prop->userFlags.isAuto = true;
      } else {
        if (pp->isReadable) {
          prop->readFunction = reflectFunction(loc, pex, obj, pp->readFunction, "get");
          prop->readFunction->functionType = PapyrusFunctionType::Getter;
        }
        if (pp->isWritable) {
          prop->writeFunction = reflectFunction(loc, pex, obj, pp->writeFunction, "set");
          prop->writeFunction->functionType = PapyrusFunctionType::Setter;
        }
      }
      obj->getRootPropertyGroup()->properties.emplace_back(prop);
    }

    for (auto ps : po->states) {
      bool pushState = pex->getStringValue(ps->name) != "";
      PapyrusState* state{ nullptr };
      if (pushState) {
        state = new PapyrusState(loc);
        state->name = pex->getStringValue(ps->name);
      } else {
        state = obj->getRootState();
      }

      for (auto pf : ps->functions) {
        auto f = reflectFunction(loc, pex, obj, pf, pex->getStringValue(pf->name));
        f->functionType = PapyrusFunctionType::Function;
        if (f->name.size() > 2 && idEq(f->name.substr(0, 2), "on"))
          f->functionType = PapyrusFunctionType::Event;
        state->functions.emplace_back(f);
      }
      
      if (pushState)
        obj->states.emplace_back(state);
    }

    script->objects.emplace_back(obj);
  }

  return script;
}

}}
