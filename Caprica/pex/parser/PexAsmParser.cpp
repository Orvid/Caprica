#include <pex/parser/PexAsmParser.h>

#include <vector>

#include <common/CaselessStringComparer.h>

namespace caprica { namespace pex { namespace parser {

PexFile* PexAsmParser::parseFile() {
  auto file = new PexFile();

  while (cur.type != TokenType::END) {
    switch (cur.type) {
      case TokenType::kInfo:
      {
        consume();
        expectConsumeEOL();

        while (!maybeConsumeTokEOL(TokenType::kEndInfo)) {
          switch (cur.type) {
            case TokenType::kSource:
              consume();
              file->sourceFileName = expectConsumeStringEOL();
              break;
            case TokenType::kModifyTime:
              consume();
              file->ensureDebugInfo();
              file->debugInfo->modificationTime = (time_t)expectConsumeLongIntegerEOL();
              break;
            case TokenType::kCompileTime:
              consume();
              file->compilationTime = (time_t)expectConsumeLongIntegerEOL();
              break;
            case TokenType::kUser:
              consume();
              file->userName = expectConsumeStringEOL();
              break;
            case TokenType::kComputer:
              consume();
              file->computerName = expectConsumeStringEOL();
              break;
            default:
              reportingContext.fatal(cur.location, "Unknown child of .info '%s'!", cur.prettyString().c_str());
          }
        }
        break;
      }
      case TokenType::kUserFlagsRef:
      {
        consume();
        expectConsumeEOL();

        while (!maybeConsumeTokEOL(TokenType::kEndUserFlagsRef)) {
          if (cur.type != TokenType::kFlag)
            reportingContext.fatal(cur.location, "Expected '.flag' got '%s'!", cur.prettyString().c_str());
          consume();

          auto nam = expectConsumePexIdent(file);
          auto bit = expectConsumeIntegerEOL();
          if (bit < 0 || bit > std::numeric_limits<uint8_t>::max())
            reportingContext.fatal(cur.location, "Bit index '%i' out of range!", (int)bit);
          file->getUserFlag(nam, (uint8_t)bit);
        }
        break;
      }
      case TokenType::kObjectTable:
      {
        consume();
        expectConsumeEOL();
        while (!maybeConsumeTokEOL(TokenType::kEndObjectTable)) {
          if (cur.type != TokenType::kObject)
            reportingContext.fatal(cur.location, "Expected '.object' got '%s'!", cur.prettyString().c_str());
          consume();

          auto obj = new PexObject();
          obj->name = expectConsumePexIdent(file);
          maybeExpectConsumeConstFlag(&obj->isConst);
          if (cur.type != TokenType::EOL)
            obj->parentClassName = expectConsumePexIdent(file);
          else
            obj->parentClassName = file->getString("");
          expectConsumeEOL();

          while (!maybeConsumeTokEOL(TokenType::kEndObject)) {
            switch (cur.type) {
              case TokenType::kUserFlags:
                consume();
                obj->userFlags = expectConsumeUserFlags();
                break;
              case TokenType::kDocString:
                consume();
                obj->documentationString = expectConsumePexStringEOL(file);
                break;
              case TokenType::kAutoState:
                consume();
                obj->autoStateName = maybeConsumePexIdentEOL(file);
                break;
              case TokenType::kStructTable:
              {
                consume();
                expectConsumeEOL();

                while (!maybeConsumeTokEOL(TokenType::kEndStructTable)) {
                  if (cur.type != TokenType::kStruct)
                    reportingContext.fatal(cur.location, "Expected '.struct' got '%s'!", cur.prettyString().c_str());
                  consume();

                  auto struc = new PexStruct();
                  struc->name = expectConsumePexIdentEOL(file);

                  while (!maybeConsumeTokEOL(TokenType::kEndStruct)) {
                    if (cur.type != TokenType::kVariable)
                      reportingContext.fatal(cur.location, "Expected '.variable' got '%s'!", cur.prettyString().c_str());
                    consume();

                    auto mem = new PexStructMember();
                    mem->name = expectConsumePexIdent(file);
                    mem->typeName = expectConsumePexIdent(file);
                    maybeExpectConsumeConstFlag(&mem->isConst);
                    expectConsumeEOL();

                    while (!maybeConsumeTokEOL(TokenType::kEndVariable)) {
                      switch (cur.type) {
                        case TokenType::kUserFlags:
                          consume();
                          mem->userFlags = expectConsumeUserFlags();
                          break;
                        case TokenType::kDocString:
                          consume();
                          mem->documentationString = expectConsumePexStringEOL(file);
                          break;
                        case TokenType::kInitialValue:
                          consume();
                          mem->defaultValue = expectConsumeValueEOL(file);
                          break;
                        default:
                          reportingContext.fatal(cur.location, "Unknown child of .variable '%s'!", cur.prettyString().c_str());
                      }
                    }
                    struc->members.push_back(mem);
                  }
                  obj->structs.push_back(struc);
                }
                break;
              }
              case TokenType::kVariableTable:
              {
                consume();
                expectConsumeEOL();

                while (!maybeConsumeTokEOL(TokenType::kEndVariableTable)) {
                  if (cur.type != TokenType::kVariable)
                    reportingContext.fatal(cur.location, "Expected '.variable' got '%s'!", cur.prettyString().c_str());
                  consume();

                  auto var = new PexVariable();
                  var->name = expectConsumePexIdent(file);
                  var->typeName = expectConsumePexIdent(file);
                  maybeExpectConsumeConstFlag(&var->isConst);
                  expectConsumeEOL();

                  while (!maybeConsumeTokEOL(TokenType::kEndVariable)) {
                    switch (cur.type) {
                      case TokenType::kUserFlags:
                        consume();
                        var->userFlags = expectConsumeUserFlags();
                        break;
                      case TokenType::kInitialValue:
                        consume();
                        var->defaultValue = expectConsumeValueEOL(file);
                        break;
                      default:
                        reportingContext.fatal(cur.location, "Unknown child of .variable '%s'!", cur.prettyString().c_str());
                    }
                  }
                  obj->variables.push_back(var);
                }
                break;
              }
              case TokenType::kPropertyTable:
              {
                consume();
                expectConsumeEOL();

                while (!maybeConsumeTokEOL(TokenType::kEndPropertyTable)) {
                  if (cur.type != TokenType::kProperty)
                    reportingContext.fatal(cur.location, "Expected '.property' got '%s'!", cur.prettyString().c_str());
                  consume();

                  auto prop = new PexProperty();
                  prop->name = expectConsumePexIdent(file);
                  prop->typeName = expectConsumePexIdent(file);
                  if (cur.type != TokenType::EOL) {
                    auto str = expectConsumeIdent();
                    if (!idEq(str, "auto"))
                      reportingContext.fatal(cur.location, "Expected 'auto' got '%s'!", str.c_str());
                    prop->isAuto = true;
                    prop->isReadable = true;
                    prop->isWritable = true;
                  }
                  expectConsumeEOL();

                  while (!maybeConsumeTokEOL(TokenType::kEndProperty)) {
                    switch (cur.type) {
                      case TokenType::kUserFlags:
                        consume();
                        prop->userFlags = expectConsumeUserFlags();
                        break;
                      case TokenType::kDocString:
                        consume();
                        prop->documentationString = expectConsumePexStringEOL(file);
                        break;
                      case TokenType::kAutoVar:
                        consume();
                        prop->autoVar = expectConsumePexIdentEOL(file);
                        break;
                      case TokenType::kFunction:
                      {
                        consume();

                        PexDebugFunctionInfo* debInf{ nullptr };
                        if (file->debugInfo) {
                          debInf = new PexDebugFunctionInfo();
                          debInf->stateName = file->getString("");
                          debInf->objectName = obj->name;
                          debInf->functionName = prop->name;
                        }
                        std::string funcName;
                        auto func = parseFunction(file, debInf, funcName);
                        if (idEq(funcName, "get")) {
                          prop->isReadable = true;
                          prop->readFunction = func;
                          if (debInf)
                            debInf->functionType = PexDebugFunctionType::Getter;
                        } else if (idEq(funcName, "set")) {
                          prop->isWritable = true;
                          prop->writeFunction = func;
                          if (debInf)
                            debInf->functionType = PexDebugFunctionType::Setter;
                        } else {
                          reportingContext.fatal(cur.location, "Unknown function definition in .property '%s'! Expected either 'get' or 'set'!", funcName.c_str());
                        }
                        if (debInf)
                          file->debugInfo->functions.push_back(debInf);
                        break;
                      }
                      default:
                        reportingContext.fatal(cur.location, "Unknown child of .property '%s'!", cur.prettyString().c_str());
                    }
                  }

                  obj->properties.push_back(prop);
                }
                break;
              }
              case TokenType::kPropertyGroupTable:
              {
                consume();
                expectConsumeEOL();

                while (!maybeConsumeTokEOL(TokenType::kEndPropertyGroupTable)) {
                  if (cur.type != TokenType::kPropertyGroup)
                    reportingContext.fatal(cur.location, "Expected '.propertyGroup' got '%s'!", cur.prettyString().c_str());
                  consume();

                  auto group = new PexDebugPropertyGroup();
                  group->objectName = obj->name;
                  if (cur.type != TokenType::EOL)
                    group->groupName = expectConsumePexIdent(file);
                  else
                    group->groupName = file->getString("");
                  expectConsumeEOL();

                  while (!maybeConsumeTokEOL(TokenType::kEndPropertyGroup)) {
                    switch (cur.type) {
                      case TokenType::kUserFlags:
                        consume();
                        group->userFlags = expectConsumeUserFlags();
                        break;
                      case TokenType::kDocString:
                        consume();
                        group->documentationString = expectConsumePexStringEOL(file);
                        break;
                      case TokenType::kProperty:
                        consume();
                        group->properties.push_back(expectConsumePexIdentEOL(file));
                        break;
                      default:
                        reportingContext.fatal(cur.location, "Unknown child of .propertyGroup '%s'!", cur.prettyString().c_str());
                    }
                  }

                  file->ensureDebugInfo();
                  file->debugInfo->propertyGroups.push_back(group);
                }
                break;
              }
              case TokenType::kStateTable:
              {
                consume();
                expectConsumeEOL();
                while (!maybeConsumeTokEOL(TokenType::kEndStateTable)) {
                  if (cur.type != TokenType::kState)
                    reportingContext.fatal(cur.location, "Expected '.state' got '%s'!", cur.prettyString().c_str());
                  consume();

                  auto state = new PexState();
                  if (cur.type != TokenType::EOL)
                    state->name = expectConsumePexIdent(file);
                  else
                    state->name = file->getString("");
                  expectConsumeEOL();

                  while (!maybeConsumeTokEOL(TokenType::kEndState)) {
                    if (cur.type != TokenType::kFunction)
                      reportingContext.fatal(cur.location, "Expected '.function' got '%s'!", cur.prettyString().c_str());
                    consume();

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
                  }

                  obj->states.push_back(state);
                }
                break;
              }
              default:
                reportingContext.fatal(cur.location, "Unknown child of .object '%s'!", cur.prettyString().c_str());
            }
          }
          file->objects.push_back(obj);
        }
        break;
      }
      default:
        reportingContext.fatal(cur.location, "Unknown root declaration '%s'!", cur.prettyString().c_str());
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

  while (!maybeConsumeTokEOL(TokenType::kEndFunction)) {
    switch (cur.type) {
      case TokenType::kUserFlags:
        consume();
        func->userFlags = expectConsumeUserFlags();
        break;
      case TokenType::kDocString:
        consume();
        func->documentationString = expectConsumePexStringEOL(file);
        break;
      case TokenType::kReturn:
        consume();
        func->returnTypeName = expectConsumePexIdentEOL(file);
        break;
      case TokenType::kParamTable:
        consume();
        expectConsumeEOL();
        while (!maybeConsumeTokEOL(TokenType::kEndParamTable)) {
          if (cur.type != TokenType::kParam)
            reportingContext.fatal(cur.location, "Expected '.param' got '%s'!", cur.prettyString().c_str());
          consume();

          auto param = new PexFunctionParameter();
          param->name = expectConsumePexIdent(file);
          param->type = expectConsumePexIdentEOL(file);
          func->parameters.push_back(param);
        }
        break;
      case TokenType::kLocalTable:
        if (func->isNative)
          reportingContext.fatal(cur.location, "A native function cannot have locals!");
        consume();
        expectConsumeEOL();
        while (!maybeConsumeTokEOL(TokenType::kEndLocalTable)) {
          if (cur.type != TokenType::kLocal)
            reportingContext.fatal(cur.location, "Expected '.local' got '%s'!", cur.prettyString().c_str());
          consume();

          auto loc = new PexLocalVariable();
          loc->name = expectConsumePexIdent(file);
          loc->type = expectConsumePexIdentEOL(file);
          func->locals.push_back(loc);
        }
        break;
      case TokenType::kCode:
      {
        if (func->isNative)
          reportingContext.fatal(cur.location, "A native function cannot have a body!");
        consume();
        expectConsumeEOL();

        std::map<std::string, PexLabel*, CaselessStringComparer> labels{ };
        while (!maybeConsumeTokEOL(TokenType::kEndCode)) {
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
            if (idEq(id, "jump")) {
              PexLabel* target{ nullptr };
              auto labName = expectConsumeIdent();
              auto f = labels.find(labName);
              if (f != labels.end())
                target = f->second;
              else
                labels.insert({ labName, target = new PexLabel() });
              func->instructions.push_back(new PexInstruction(PexOpCode::Jmp, { target }));
            } else if (idEq(id, "jumpf")) {
              auto val = expectConsumeValue(file);
              PexLabel* target{ nullptr };
              auto labName = expectConsumeIdent();
              auto f = labels.find(labName);
              if (f != labels.end())
                target = f->second;
              else
                labels.insert({ labName, target = new PexLabel() });
              func->instructions.push_back(new PexInstruction(PexOpCode::JmpF, { val, target }));
            } else if (idEq(id, "jumpt")) {
              auto val = expectConsumeValue(file);
              PexLabel* target{ nullptr };
              auto labName = expectConsumeIdent();
              auto f = labels.find(labName);
              if (f != labels.end())
                target = f->second;
              else
                labels.insert({ labName, target = new PexLabel() });
              func->instructions.push_back(new PexInstruction(PexOpCode::JmpT, { val, target }));
            } else if (idEq(id, "callmethod")) {
              auto valA = expectConsumeValue(file);
              auto valB = expectConsumeValue(file);
              auto valC = expectConsumeValue(file);
              std::vector<PexValue> params;
              while (cur.type != TokenType::LineNumer && cur.type != TokenType::EOL && cur.type != TokenType::END) {
                params.push_back(expectConsumeValue(file));
              }
              func->instructions.push_back(new PexInstruction(PexOpCode::CallMethod, { valA, valB, valC }, std::move(params)));
            } else if (idEq(id, "callparent")) {
              auto valA = expectConsumeValue(file);
              auto valB = expectConsumeValue(file);
              std::vector<PexValue> params;
              while (cur.type != TokenType::LineNumer && cur.type != TokenType::EOL && cur.type != TokenType::END) {
                params.push_back(expectConsumeValue(file));
              }
              func->instructions.push_back(new PexInstruction(PexOpCode::CallParent, { valA, valB }, std::move(params)));
            } else if (idEq(id, "callstatic")) {
              auto valA = expectConsumeValue(file);
              auto valB = expectConsumeValue(file);
              auto valC = expectConsumeValue(file);
              std::vector<PexValue> params;
              while (cur.type != TokenType::LineNumer && cur.type != TokenType::EOL && cur.type != TokenType::END) {
                params.push_back(expectConsumeValue(file));
              }
              func->instructions.push_back(new PexInstruction(PexOpCode::CallStatic, { valA, valB, valC }, std::move(params)));
            } else {
              auto op = PexInstruction::tryParseOpCode(id);
              if (op == PexOpCode::Invalid)
                reportingContext.fatal(cur.location, "Unknown op-code '%s'!", id.c_str());

              std::vector<PexValue> params;
              while (cur.type != TokenType::LineNumer && cur.type != TokenType::EOL && cur.type != TokenType::END) {
                params.push_back(expectConsumeValue(file));
              }
              func->instructions.push_back(new PexInstruction(op, std::move(params)));
            }

            if (maybeConsume(TokenType::LineNumer)) {
              auto num = expectConsumeIntegerEOL();
              if (num < 0 || num > std::numeric_limits<uint16_t>::max())
                reportingContext.fatal(cur.location, "Line number '%i' out of the range of a uint16_t!", (int)num);
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
                CapricaReportingContext::logicalFatal("Unresolved label!");
              auto newVal = arg.l->targetIdx - i;
              arg.type = PexValueType::Integer;
              arg.i = (int32_t)newVal;
            }
          }
        }

        for (auto l : labels) {
          if (l.second->targetIdx == (size_t)-1)
            CapricaReportingContext::logicalFatal("Unused unresolved label!");
          delete l.second;
        }
        break;
      }
      default:
        reportingContext.fatal(cur.location, "Unknown child of .function '%s'!", cur.prettyString().c_str());
    }
  }

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
      if (idEq(str, "none")) {
        val.type = PexValueType::None;
        return val;
      } else if (idEq(str, "true")) {
        val.type = PexValueType::Bool;
        val.b = true;
        return val;
      } else if (idEq(str, "false")) {
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
      reportingContext.fatal(cur.location, "Expected a default value, got '%s'!", cur.prettyString().c_str());
  }
}

}}}
