#include <common/parser/CapricaUserFlagsParser.h>

namespace caprica { namespace parser {

void CapricaUserFlagsParser::parseUserFlags(CapricaUserFlagsDefinition& def) {
  while (cur.type != TokenType::END) {
    expectConsume(TokenType::kFlag);
    CapricaUserFlagsDefinition::UserFlag flag(cur.location);
    expect(TokenType::Identifier);
    flag.name = cur.sValue;
    consume();
    expect(TokenType::Integer);
    flag.bitIndex = cur.iValue;
    consume();

    if (maybeConsume(TokenType::LCurly)) {
      while (!maybeConsume(TokenType::RCurly)) {
        switch (cur.type) {
          case TokenType::kFunction:
            flag.validLocations |= CapricaUserFlagsDefinition::ValidLocations::Function;
            consume();
            break;
          case TokenType::kProperty:
            flag.validLocations |= CapricaUserFlagsDefinition::ValidLocations::Property;
            consume();
            break;
          case TokenType::kPropertyGroup:
            flag.validLocations |= CapricaUserFlagsDefinition::ValidLocations::PropertyGroup;
            consume();
            break;
          case TokenType::kScript:
            flag.validLocations |= CapricaUserFlagsDefinition::ValidLocations::Script;
            consume();
            break;
          case TokenType::kStructMember:
            flag.validLocations |= CapricaUserFlagsDefinition::ValidLocations::StructMember;
            consume();
            break;
          case TokenType::kVariable:
            flag.validLocations |= CapricaUserFlagsDefinition::ValidLocations::Variable;
            consume();
            break;
          default:
            reportingContext.fatal(cur.location, "Unexpected token '%s'!", cur.prettyString().c_str());
        }
      }
    } else {
      flag.validLocations = CapricaUserFlagsDefinition::ValidLocations::AllLocations;
    }

    def.registerUserFlag(reportingContext, flag);
  }
}

}}
