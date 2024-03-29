#include <common/parser/CapricaUserFlagsParser.h>

namespace caprica { namespace parser {

void CapricaUserFlagsParser::parseUserFlags(CapricaUserFlagsDefinition& def) {
  while (cur.type != TokenType::END) {
    expectConsume(TokenType::kFlag);
    CapricaUserFlagsDefinition::UserFlag flag(cur.location);
    expect(TokenType::Identifier);
    flag.name = std::move(cur.sValue);
    consume();
    if (cur.type == TokenType::Identifier) {
      auto& childFlag = def.findFlag(reportingContext, cur.location, cur.sValue);
      flag.flagData = childFlag.getData();
      flag.validLocations = childFlag.validLocations;
      consume();

      while (maybeConsume(TokenType::And)) {
        expect(TokenType::Identifier);
        auto& otherFlag = def.findFlag(reportingContext, cur.location, cur.sValue);
        flag.flagData |= otherFlag.getData();
        flag.validLocations &= otherFlag.validLocations;
        consume();
      }

      if (flag.validLocations == CapricaUserFlagsDefinition::ValidLocations::None)
        reportingContext.error(flag.location, "The child flags have no valid locations in common.");
    } else {
      expect(TokenType::Integer);
      if (cur.iValue < 0 || cur.iValue > 31)
        reportingContext.error(cur.location, "The bit index must be greater than zero and less than 31.");
      flag.bitIndex = uint8_t(cur.iValue);
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
            case TokenType::kGroup:
              flag.validLocations |= CapricaUserFlagsDefinition::ValidLocations::PropertyGroup;
              consume();
              break;
            case TokenType::kScript:
              flag.validLocations |= CapricaUserFlagsDefinition::ValidLocations::Script;
              consume();
              break;
            case TokenType::kStructVar:
              flag.validLocations |= CapricaUserFlagsDefinition::ValidLocations::StructMember;
              consume();
              break;
            case TokenType::kVariable:
              flag.validLocations |= CapricaUserFlagsDefinition::ValidLocations::Variable;
              consume();
              break;
            default:
              consume();
              reportingContext.error(cur.location, "Unexpected token '{}'!", cur.prettyString());
              break;
          }
        }
      } else {
        flag.validLocations = CapricaUserFlagsDefinition::ValidLocations::AllLocations;
      }
    }

    def.registerUserFlag(reportingContext, flag);
  }
}

}}
