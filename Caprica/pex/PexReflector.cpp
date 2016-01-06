#include <pex/PexReflector.h>

#include <pex/PexFile.h>
#include <pex/PexReader.h>

namespace caprica { namespace pex {

using namespace caprica::papyrus;

static PapyrusType reflectType(const CapricaFileLocation& loc, const std::string& name) {
  if (name.size() > 2 && name[name.size() - 2] == '[' && name[name.size() - 1] == ']')
    return PapyrusType::Array(loc, std::make_shared<PapyrusType>(reflectType(loc, name.substr(0, name.size() - 2))));

  if (!_stricmp(name.c_str(), "bool"))
    return PapyrusType::Bool(loc);
  if (!_stricmp(name.c_str(), "none"))
    return PapyrusType::None(loc);
  if (!_stricmp(name.c_str(), "int"))
    return PapyrusType::Int(loc);
  if (!_stricmp(name.c_str(), "float"))
    return PapyrusType::Float(loc);
  if (!_stricmp(name.c_str(), "string"))
    return PapyrusType::String(loc);
  if (!_stricmp(name.c_str(), "var"))
    return PapyrusType::Var(loc);

  return PapyrusType::Unresolved(loc, name);
}

static PapyrusType reflectType(const CapricaFileLocation& loc, PexFile* pex, const PexString& pexName) {
  return reflectType(loc, pex->getStringValue(pexName));
}

static PapyrusFunction* reflectFunction(const CapricaFileLocation& loc, PexFile* pex, PapyrusObject* obj, PexFunction* pFunc, std::string funcName) {
  auto func = new PapyrusFunction(loc, reflectType(loc, pex, pFunc->returnTypeName));
  func->parentObject = obj;
  func->name = funcName;
  func->isGlobal = pFunc->isGlobal;
  func->isNative = pFunc->isNative;

  for (auto pp : pFunc->parameters) {
    auto param = new PapyrusFunctionParameter(loc, reflectType(loc, pex, pp->type));
    param->name = pex->getStringValue(pp->name);
    func->parameters.push_back(param);
  }

  return func;
}

PapyrusScript* PexReflector::reflectScript(const std::string& filename) {
  const CapricaFileLocation loc{ filename, 0, 0 };

  PexReader rdr(filename);
  auto pex = PexFile::read(rdr);
  auto script = new PapyrusScript();
  script->sourceFileName = pex->sourceFileName;
  
  for (auto po : pex->objects) {
    PapyrusType baseTp = PapyrusType::None(loc);
    if (pex->getStringValue(po->parentClassName) != "")
      baseTp = reflectType(loc, pex, po->parentClassName);
    auto obj = new PapyrusObject(loc, baseTp);
    obj->name = pex->getStringValue(po->name);

    for (auto ps : po->structs) {
      auto struc = new PapyrusStruct();
      struc->parentObject = obj;
      struc->name = pex->getStringValue(ps->name);
      for (auto pm : ps->members) {
        auto mem = new PapyrusStructMember(loc, reflectType(loc, pex, pm->typeName));
        mem->isConst = pm->isConst;
        mem->name = pex->getStringValue(pm->name);
        struc->members.push_back(mem);
      }
      obj->structs.push_back(struc);
    }

    for (auto pp : po->properties) {
      auto prop = new PapyrusProperty(loc, reflectType(loc, pex, pp->typeName));
      prop->name = pex->getStringValue(pp->name);
      if (pp->isAuto) {
        prop->isAuto = true;
      } else {
        if (pp->isReadable)
          prop->readFunction = reflectFunction(loc, pex, obj, pp->readFunction, "get");
        if (pp->isWritable)
          prop->writeFunction = reflectFunction(loc, pex, obj, pp->writeFunction, "set");
      }
      obj->getRootPropertyGroup()->properties.push_back(prop);
    }

    for (auto ps : po->states) {
      bool pushState = pex->getStringValue(ps->name) != "";
      PapyrusState* state{ nullptr };
      if (pushState) {
        state = new PapyrusState();
        state->name = pex->getStringValue(ps->name);
      } else {
        state = obj->getRootState();
      }

      for (auto pf : ps->functions)
        state->functions.push_back(reflectFunction(loc, pex, obj, pf, pex->getStringValue(pf->name)));
      
      if (pushState)
        obj->states.push_back(state);
    }

    script->objects.push_back(obj);
  }

  delete pex;
  return script;
}

}}
