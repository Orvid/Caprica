#include <pex/parser/PexAsmParser.h>

#include <vector>

// For the CaselessStringComparer
#include <papyrus/parser/PapyrusLexer.h>

namespace caprica { namespace pex { namespace parser {

PexFile* PexAsmParser::parseFile() {
  auto file = new PexFile();

  while (cur.type != TokenType::END) {
    auto id = expectConsumeDotIdentEOL();
    if (id == "info") {
      auto id2 = expectConsumeDotIdent();
      while (id2 != "endInfo") {
        if (id2 == "source") {
          file->sourceFileName = expectConsumeStringEOL();
        } else if (id2 == "modifyTime") {
          file->ensureDebugInfo();
          file->debugInfo->modificationTime = (time_t)expectConsumeLongIntegerEOL();
        } else if (id2 == "compileTime") {
          file->compilationTime = (time_t)expectConsumeLongIntegerEOL();
        } else if (id2 == "user") {
          file->userName = expectConsumeStringEOL();
        } else if (id2 == "computer") {
          file->computerName = expectConsumeStringEOL();
        } else {
          CapricaError::fatal(cur.location, "Unknown child of .info '.%s'!", id2.c_str());
        }
        id2 = expectConsumeDotIdent();
      }
      expectConsumeEOL();
    } else if (id == "userFlagsRef") {
      auto id2 = expectConsumeDotIdent();
      while (id2 != "endUserFlagsRef") {
        if (id2 != "flag")
          CapricaError::fatal(cur.location, "Expected '.flag' got '.%s'!", id2.c_str());

        auto nam = expectConsumePexIdent(file);
        auto bit = expectConsumeIntegerEOL();
        if (bit < 0 || bit > std::numeric_limits<uint8_t>::max())
          CapricaError::fatal(cur.location, "Bit index '%i' out of range!", (int)bit);
        file->getUserFlag(nam, (uint8_t)bit);

        id2 = expectConsumeDotIdent();
      }
      expectConsumeEOL();
    } else if (id == "objectTable") {
      auto id2 = expectConsumeDotIdent();
      while (id2 != "endObjectTable") {
        if (id2 != "object")
          CapricaError::fatal(cur.location, "Expected '.object' got '.%s'!", id2.c_str());

        auto obj = new PexObject();
        obj->name = expectConsumePexIdent(file);
        if (cur.type != TokenType::EOL)
          obj->parentClassName = expectConsumePexIdent(file);
        else
          obj->parentClassName = file->getString("");
        expectConsumeEOL();

        auto id3 = expectConsumeDotIdent();
        while (id3 != "endObject") {
          if (id3 == "userFlags") {
            obj->userFlags = expectConsumeUserFlags();
          } else if (id3 == "docString") {
            obj->documentationString = expectConsumePexStringEOL(file);
          } else if (id3 == "autoState") {
            obj->autoStateName = maybeConsumePexStringEOL(file);
          } else if (id3 == "variableTable") {
            expectConsumeEOL();

            auto id4 = expectConsumeDotIdent();
            while (id4 != "endVariableTable") {
              if (id4 != "variable")
                CapricaError::fatal(cur.location, "Expected '.variable' got '.%s'!", id4.c_str());

              auto var = new PexVariable();
              var->name = expectConsumePexIdent(file);
              var->typeName = expectConsumePexIdentEOL(file);

              auto id5 = expectConsumeDotIdent();
              while (id5 != "endVariable") {
                if (id5 == "userFlags") {
                  var->userFlags = expectConsumeUserFlags();
                } else if (id5 == "initialValue") {
                  var->defaultValue = expectConsumeValueEOL(file);
                } else {
                  CapricaError::fatal(cur.location, "Unknown child of .variable '.%s'!", id5.c_str());
                }
                id5 = expectConsumeDotIdent();
              }
              expectConsumeEOL();

              obj->variables.push_back(var);
              id4 = expectConsumeDotIdent();
            }
            expectConsumeEOL();
          } else if (id3 == "propertyTable") {
            expectConsumeEOL();

            auto id4 = expectConsumeDotIdent();
            while (id4 != "endPropertyTable") {
              if (id4 != "property")
                CapricaError::fatal(cur.location, "Expected '.property' got '.%s'!", id4.c_str());

              auto prop = new PexProperty();
              prop->name = expectConsumePexIdent(file);
              prop->typeName = expectConsumePexIdent(file);
              if (cur.type != TokenType::EOL) {
                auto str = expectConsumeIdent();
                if (_stricmp(str.c_str(), "auto"))
                  CapricaError::fatal(cur.location, "Expected 'auto' got '%s'!", str.c_str());
                prop->isAuto = true;
                prop->isReadable = true;
                prop->isWritable = true;
              }
              expectConsumeEOL();

              auto id5 = expectConsumeDotIdent();
              while (id5 != "endProperty") {
                if (id5 == "userFlags") {
                  prop->userFlags = expectConsumeUserFlags();
                } else if (id5 == "docString") {
                  prop->documentationString = expectConsumePexStringEOL(file);
                } else if (id5 == "autoVar") {
                  prop->autoVar = expectConsumePexIdentEOL(file);
                } else if (id5 == "function") {
                  PexDebugFunctionInfo* debInf{ nullptr };
                  if (file->debugInfo) {
                    debInf = new PexDebugFunctionInfo();
                    debInf->stateName = file->getString("");
                    debInf->objectName = obj->name;
                    debInf->functionName = prop->name;
                  }
                  std::string funcName;
                  auto func = parseFunction(file, debInf, funcName);
                  if (!_stricmp(funcName.c_str(), "get")) {
                    prop->isReadable = true;
                    prop->readFunction = func;
                    if (debInf)
                      debInf->functionType = PexDebugFunctionType::Getter;
                  } else if (!_stricmp(funcName.c_str(), "set")) {
                    prop->isWritable = true;
                    prop->writeFunction = func;
                    if (debInf)
                      debInf->functionType = PexDebugFunctionType::Setter;
                  } else {
                    CapricaError::fatal(cur.location, "Unknown function definition in .property '%s'! Expected either 'get' or 'set'!", funcName.c_str());
                  }
                  if (debInf)
                    file->debugInfo->functions.push_back(debInf);
                } else {
                  CapricaError::fatal(cur.location, "Unknown child of .property '.%s'!", id5.c_str());
                }
                id5 = expectConsumeDotIdent();
              }
              expectConsumeEOL();

              obj->properties.push_back(prop);
              id4 = expectConsumeDotIdent();
            }
            expectConsumeEOL();
          } else if (id3 == "stateTable") {
            expectConsumeEOL();

            auto id4 = expectConsumeDotIdent();
            while (id4 != "endStateTable") {
              if (id4 != "state")
                CapricaError::fatal(cur.location, "Expected '.state' got '.%s'!", id4.c_str());

              auto state = new PexState();
              if (cur.type != TokenType::EOL)
                state->name = expectConsumePexIdent(file);
              else
                state->name = file->getString("");
              expectConsumeEOL();

              auto id5 = expectConsumeDotIdent();
              while (id5 != "endState") {
                if (id5 != "function")
                  CapricaError::fatal(cur.location, "Expected '.function' got '.%s'!", id4.c_str());

                PexDebugFunctionInfo* debInf{ nullptr };
                if (file->debugInfo) {
                  debInf = new PexDebugFunctionInfo();
                  debInf->functionType = PexDebugFunctionType::Normal;
                  debInf->stateName = state->name;
                  debInf->objectName = obj->name;
                }
                std::string funcName;
                auto func = parseFunction(file, debInf, funcName);
                func->name = file->getString(funcName);
                if (debInf) {
                  debInf->functionName = func->name;
                  file->debugInfo->functions.push_back(debInf);
                }
                state->functions.push_back(func);
                id5 = expectConsumeDotIdent();
              }
              expectConsumeEOL();

              obj->states.push_back(state);
              id4 = expectConsumeDotIdent();
            }
            expectConsumeEOL();
          } else {
            CapricaError::fatal(cur.location, "Unknown child of .object '.%s'!", id3.c_str());
          }
          id3 = expectConsumeDotIdent();
        }
        expectConsumeEOL();

        file->objects.push_back(obj);
        id2 = expectConsumeDotIdent();
      }
    } else {
      CapricaError::fatal(cur.location, "Unknown root declaration '.%s'!", id.c_str());
    }
  }

  return file;
}

PexFunction* PexAsmParser::parseFunction(PexFile* file, PexDebugFunctionInfo* debInfo, std::string& funcNameOut) {
  auto func = new PexFunction();
  funcNameOut = expectConsumeIdent();
  if (cur.type != TokenType::EOL) {
    auto id = expectConsumeIdent();
    if (id == "native") {
      func->isNative = true;
      if (cur.type != TokenType::EOL)
        id = expectConsumeIdent();
    }
    if (id == "static")
      func->isGlobal = true;
  }
  expectConsumeEOL();

  auto id = expectConsumeDotIdent();
  while (id != "endFunction") {
    if (id == "userFlags") {
      func->userFlags = expectConsumeUserFlags();
    } else if (id == "docString") {
      func->documentationString = expectConsumePexStringEOL(file);
    } else if (id == "return") {
      func->returnTypeName = expectConsumePexIdentEOL(file);
    } else if (id == "paramTable") {
      expectConsumeEOL();

      auto id2 = expectConsumeDotIdent();
      while (id2 != "endParamTable") {
        if (id2 != "param")
          CapricaError::fatal(cur.location, "Expected '.param' got '.%s'!", id2.c_str());

        auto param = new PexFunctionParameter();
        param->name = expectConsumePexIdent(file);
        param->type = expectConsumePexIdentEOL(file);
        func->parameters.push_back(param);

        id2 = expectConsumeDotIdent();
      }
      expectConsumeEOL();
    } else if (id == "localTable") {
      expectConsumeEOL();
      if (func->isNative)
        CapricaError::fatal(cur.location, "A native function cannot have locals!");

      auto id2 = expectConsumeDotIdent();
      while (id2 != "endLocalTable") {
        if (id2 != "local")
          CapricaError::fatal(cur.location, "Expected '.local' got '.%s'!", id2.c_str());

        auto loc = new PexLocalVariable();
        loc->name = expectConsumePexIdent(file);
        loc->type = expectConsumePexIdentEOL(file);
        func->locals.push_back(loc);

        id2 = expectConsumeDotIdent();
      }
      expectConsumeEOL();
    } else if (id == "code") {
      expectConsumeEOL();
      if (func->isNative)
        CapricaError::fatal(cur.location, "A native function cannot have a body!");

      std::map<std::string, PexLabel*, papyrus::parser::CaselessStringComparer> labels{ };
      while (cur.type != TokenType::Dot) {
        auto id = expectConsumeIdent();
        if (id[id.size() - 1] == ':') {
          // It's a label.
          auto f = labels.find(id.substr(0, id.size() - 1));
          if (f != labels.end()) {
            f->second->targetIdx = func->instructions.size();
          } else {
            auto lab = new PexLabel();
            lab->targetIdx = func->instructions.size();
            labels.insert({ id.substr(0, id.size() - 1), lab });
          }
          expectConsumeEOL();
        } else {
          if (!_stricmp(id.c_str(), "jump")) {
            PexLabel* target{ nullptr };
            auto labName = expectConsumeIdent();
            auto f = labels.find(labName);
            if (f != labels.end())
              target = f->second;
            else
              labels.insert({ labName, target = new PexLabel() });
            func->instructions.push_back(new PexInstruction(PexOpCode::Jmp, { target }));
          } else if (!_stricmp(id.c_str(), "jumpf")) {
            auto val = expectConsumeValue(file);
            PexLabel* target{ nullptr };
            auto labName = expectConsumeIdent();
            auto f = labels.find(labName);
            if (f != labels.end())
              target = f->second;
            else
              labels.insert({ labName, target = new PexLabel() });
            func->instructions.push_back(new PexInstruction(PexOpCode::JmpF, { val, target }));
          } else if (!_stricmp(id.c_str(), "jumpt")) {
            auto val = expectConsumeValue(file);
            PexLabel* target{ nullptr };
            auto labName = expectConsumeIdent();
            auto f = labels.find(labName);
            if (f != labels.end())
              target = f->second;
            else
              labels.insert({ labName, target = new PexLabel() });
            func->instructions.push_back(new PexInstruction(PexOpCode::JmpT, { val, target }));
          } else if (!_stricmp(id.c_str(), "callmethod")) {
            auto valA = expectConsumeValue(file);
            auto valB = expectConsumeValue(file);
            auto valC = expectConsumeValue(file);
            std::vector<PexValue> params;
            while (cur.type != TokenType::LineNumer && cur.type != TokenType::EOL && cur.type != TokenType::END) {
              params.push_back(expectConsumeValue(file));
            }
            func->instructions.push_back(new PexInstruction(PexOpCode::CallMethod , { valA, valB, valC }, params));
          } else if (!_stricmp(id.c_str(), "callparent")) {
            auto valA = expectConsumeValue(file);
            auto valB = expectConsumeValue(file);
            std::vector<PexValue> params;
            while (cur.type != TokenType::LineNumer && cur.type != TokenType::EOL && cur.type != TokenType::END) {
              params.push_back(expectConsumeValue(file));
            }
            func->instructions.push_back(new PexInstruction(PexOpCode::CallParent, { valA, valB }, params));
          } else if (!_stricmp(id.c_str(), "callstatic")) {
            auto valA = expectConsumeValue(file);
            auto valB = expectConsumeValue(file);
            auto valC = expectConsumeValue(file);
            std::vector<PexValue> params;
            while (cur.type != TokenType::LineNumer && cur.type != TokenType::EOL && cur.type != TokenType::END) {
              params.push_back(expectConsumeValue(file));
            }
            func->instructions.push_back(new PexInstruction(PexOpCode::CallStatic, { valA, valB, valC }, params));
          } else {
            auto op = PexInstruction::tryParseOpCode(id);
            if (op == PexOpCode::Invalid)
              CapricaError::fatal(cur.location, "Unknown op-code '%s'!", id.c_str());

            std::vector<PexValue> params;
            while (cur.type != TokenType::LineNumer && cur.type != TokenType::EOL && cur.type != TokenType::END) {
              params.push_back(expectConsumeValue(file));
            }
            func->instructions.push_back(new PexInstruction(op, params));
          }

          if (maybeConsume(TokenType::LineNumer)) {
            auto num = expectConsumeIntegerEOL();
            if (num < 0 || num > std::numeric_limits<uint16_t>::max())
              CapricaError::fatal(cur.location, "Line number '%i' out of the range of a uint16_t!", (int)num);
            if (debInfo)
              debInfo->instructionLineMap.push_back((uint16_t)num);
          } else {
            if (debInfo && debInfo->instructionLineMap.size() != 0 && debInfo->instructionLineMap.size() != func->instructions.size()) {
              debInfo->instructionLineMap.push_back(debInfo->instructionLineMap[debInfo->instructionLineMap.size() - 1]);
            }
            expectConsumeEOL();
          }
        }
      }

      for (size_t i = 0; i < func->instructions.size(); i++) {
        for (auto& arg : func->instructions[i]->args) {
          if (arg.type == PexValueType::Label) {
            if (arg.l->targetIdx == (size_t)-1)
              CapricaError::logicalFatal("Unresolved label!");
            auto newVal = arg.l->targetIdx - i;
            arg.type = PexValueType::Integer;
            arg.i = (int32_t)newVal;
          }
        }
      }

      for (auto l : labels) {
        if (l.second->targetIdx == (size_t)-1)
          CapricaError::logicalFatal("Unused unresolved label!");
        delete l.second;
      }

      if (expectConsumeDotIdentEOL() != "endCode")
        CapricaError::fatal(cur.location, "Expected '.endCode' but got something else!");
    } else {
      CapricaError::fatal(cur.location, "Unknown child of .function '.%s'!", id.c_str());
    }
    id = expectConsumeDotIdent();
  }
  expectConsumeEOL();

  return func;
}

PexValue PexAsmParser::expectConsumeValue(PexFile* file) {
  PexValue val;

  switch (cur.type) {
    case TokenType::Float:
      val.type = PexValueType::Float;
      val.f = cur.fValue;
      consume();
      return val;
    case TokenType::Integer:
      val.type = PexValueType::Integer;
      val.i = expectConsumeInteger();
      return val;
    case TokenType::String:
      val.type = PexValueType::String;
      val.s = file->getString(cur.sValue);
      consume();
      return val;
    case TokenType::Identifier:
    {
      auto str = expectConsumeIdent();
      if (!_stricmp(str.c_str(), "none")) {
        val.type = PexValueType::None;
        return val;
      } else if (!_stricmp(str.c_str(), "true")) {
        val.type = PexValueType::Bool;
        val.b = true;
        return val;
      } else if (!_stricmp(str.c_str(), "false")) {
        val.type = PexValueType::Bool;
        val.b = false;
        return val;
      } else {
        val.type = PexValueType::Identifier;
        val.s = file->getString(str);
        return val;
      }
    }

    default:
      CapricaError::fatal(cur.location, "Expected a default value, got '%s'!", cur.prettyString().c_str());
  }
}

}}}
