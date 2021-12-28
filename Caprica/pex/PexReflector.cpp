#include <pex/PexReflector.h>

#include <common/allocators/ChainedPool.h>

#include <pex/PexReader.h>

namespace caprica { namespace pex {

using namespace caprica::papyrus;

static PapyrusType reflectType(CapricaFileLocation loc, allocators::ChainedPool* alloc, const identifier_ref& name) {
  if (name.size() > 2 && name[name.size() - 2] == '[' && name[name.size() - 1] == ']')
    return PapyrusType::Array(loc, alloc->make<PapyrusType>(reflectType(loc, alloc, name.substr(0, name.size() - 2))));

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

  return PapyrusType::Unresolved(loc, alloc->allocateIdentifier(name));
}

static PapyrusType reflectType(CapricaFileLocation loc, allocators::ChainedPool* alloc, PexFile* pex, PexString pexName) {
  return reflectType(loc, alloc, alloc->allocateIdentifier(pex->getStringValue(pexName)));
}

static PapyrusFunction* reflectFunction(CapricaFileLocation loc, allocators::ChainedPool* alloc, PexFile* pex, PapyrusObject* obj, PexFunction* pFunc, const identifier_ref& funcName) {
  auto func = alloc->make<PapyrusFunction>(loc, reflectType(loc, alloc, pex, pFunc->returnTypeName));
  func->parentObject = obj;
  func->name = funcName;
  func->userFlags.isGlobal = pFunc->isGlobal;
  func->userFlags.isNative = pFunc->isNative;

  for (auto pp : pFunc->parameters) {
    auto param = alloc->make<PapyrusFunctionParameter>(loc, func->parameters.size(), reflectType(loc, alloc, pex, pp->type));
    param->name = alloc->allocateIdentifier(pex->getStringValue(pp->name));
    func->parameters.push_back(param);
  }

  return func;
}

PapyrusScript* PexReflector::reflectScript(PexFile* pex) {
  auto alloc = new allocators::ChainedPool(1024 * 4);
  CapricaFileLocation loc{ 0 };

  auto script = alloc->make<PapyrusScript>();
  script->allocator = alloc;
  script->sourceFileName = pex->sourceFileName;
  
  for (auto po : pex->objects) {
    PapyrusType baseTp = PapyrusType::None(loc);
    if (pex->getStringValue(po->parentClassName) != "")
      baseTp = reflectType(loc, alloc, pex, po->parentClassName);
    auto obj = alloc->make<PapyrusObject>(loc, alloc, baseTp);
    obj->setName(alloc->allocateIdentifier(pex->getStringValue(po->name)));

    for (auto ps : po->structs) {
      auto struc = alloc->make<PapyrusStruct>(loc);
      struc->parentObject = obj;
      struc->name = alloc->allocateIdentifier(pex->getStringValue(ps->name));
      for (auto pm : ps->members) {
        auto mem = alloc->make<PapyrusStructMember>(loc, reflectType(loc, alloc, pex, pm->typeName), struc);
        mem->userFlags.isConst = pm->isConst;
        mem->name = alloc->allocateIdentifier(pex->getStringValue(pm->name));
        struc->members.push_back(mem);
      }
      obj->structs.push_back(struc);
    }

    for (auto pp : po->properties) {
      auto prop = alloc->make<PapyrusProperty>(loc, reflectType(loc, alloc, pex, pp->typeName), obj);
      prop->name = alloc->allocateIdentifier(pex->getStringValue(pp->name));
      if (pp->isAuto) {
        prop->userFlags.isAuto = true;
      } else {
        if (pp->isReadable) {
          prop->readFunction = reflectFunction(loc, alloc, pex, obj, pp->readFunction, "get");
          prop->readFunction->functionType = PapyrusFunctionType::Getter;
        }
        if (pp->isWritable) {
          prop->writeFunction = reflectFunction(loc, alloc, pex, obj, pp->writeFunction, "set");
          prop->writeFunction->functionType = PapyrusFunctionType::Setter;
        }
      }
      obj->getRootPropertyGroup()->properties.push_back(prop);
    }

    for (auto ps : po->states) {
      bool pushState = pex->getStringValue(ps->name) != "";
      PapyrusState* state{ nullptr };
      if (pushState) {
        state = alloc->make<PapyrusState>(loc);
        state->name = alloc->allocateIdentifier(pex->getStringValue(ps->name));
      } else {
        state = obj->getRootState();
      }

      for (auto pf : ps->functions) {
        auto f = reflectFunction(loc, alloc, pex, obj, pf, alloc->allocateIdentifier(pex->getStringValue(pf->name)));
        f->functionType = PapyrusFunctionType::Function;
        if (f->name.size() > 2 && idEq(f->name.substr(0, 2), "on"))
          f->functionType = PapyrusFunctionType::Event;
        state->functions.emplace(f->name, f);
      }
      
      if (pushState)
        obj->states.push_back(state);
    }

    script->objects.push_back(obj);
  }

  return script;
}

}}
