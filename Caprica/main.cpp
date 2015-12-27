
#include <ostream>

#include <papyrus/parser/PapyrusParser.h>
#include <papyrus/PapyrusScript.h>
#include <pex/PexWriter.h>

int main(int argc, char* argv[])
{
  /*auto a = new caprica::papyrus::PapyrusScript();
  auto obj = new caprica::papyrus::PapyrusObject();
  obj->name = "Test";
  obj->parentClass = caprica::papyrus::PapyrusType("Quest");
  a->objects.push_back(obj);
  auto state = new caprica::papyrus::PapyrusState();
  obj->states.push_back(state);
  auto func = new caprica::papyrus::PapyrusFunction();
  func->name = "TestFunc";
  func->returnType = caprica::papyrus::PapyrusType("none");
  state->functions.push_back(func);

  auto prop = new caprica::papyrus::PapyrusProperty();
  prop->name = "SomeProperty";
  prop->isConst = true;
  prop->type = caprica::papyrus::PapyrusType("Int");
  prop->defaultValue.type = caprica::papyrus::PapyrusValueType::Integer;
  prop->defaultValue.i = 5;
  obj->properties.push_back(prop);

  auto prop2 = new caprica::papyrus::PapyrusProperty();
  prop2->name = "SomeOtherProperty";
  prop2->type = caprica::papyrus::PapyrusType("Int");
  prop2->defaultValue.type = caprica::papyrus::PapyrusValueType::Integer;
  prop2->defaultValue.i = 4;
  obj->properties.push_back(prop2);*/

  auto parser = new caprica::papyrus::parser::PapyrusParser("test.psc");
  auto a = parser->parseScript();
  auto pex = a->buildPex();
  std::ofstream strm("test.pex", std::ofstream::binary);
  caprica::pex::PexWriter wtr(strm);
  pex->write(wtr);

  return 0;
}

