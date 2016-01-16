#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <common/CapricaBinaryReader.h>
#include <common/CapricaBinaryWriter.h>

namespace caprica { namespace vmad {

enum class VMADPropertyType : uint8_t
{
  Unknown,
  Object = 1,
  String = 2,
  Integer = 3,
  Float = 4,
  Bool = 5,

  ArrayOfObjects = 11,
  ArrayOfStrings = 12,
  ArrayOfIntegers = 13,
  ArrayOfFloats = 14,
  ArrayOfBools = 15,
};

enum class VMADPropertyStatus : uint8_t
{
  Unknown,
  Edited = 1,
  Removed = 3,
};

struct VMADPropertyValue final
{
  std::string stringValue;
  union
  {
    int32_t intVal;
    float floatVal;
    bool boolVal;
    struct
    {
      uint32_t formID;
      uint16_t alias;
    } objectVal;
  };

  static VMADPropertyValue read(VMADPropertyType tp, CapricaBinaryReader& rdr) {
    VMADPropertyValue val;
    
    // We read array types here because it keeps the switch in the property itself sane.
    switch (tp) {
      case VMADPropertyType::Object:
      case VMADPropertyType::ArrayOfObjects:
      {
        auto unused = rdr.read<uint16_t>();
        val.objectVal.alias = rdr.read<uint16_t>();
        val.objectVal.formID = rdr.read<uint32_t>();
        break;
      }

      case VMADPropertyType::String:
      case VMADPropertyType::ArrayOfStrings:
        val.stringValue = rdr.read<std::string>();
        break;
      case VMADPropertyType::Integer:
      case VMADPropertyType::ArrayOfIntegers:
        val.intVal = rdr.read<int32_t>();
        break;
      case VMADPropertyType::Float:
      case VMADPropertyType::ArrayOfFloats:
        val.floatVal = rdr.read<float>();
        break;
      case VMADPropertyType::Bool:
      case VMADPropertyType::ArrayOfBools:
        val.boolVal = rdr.read<uint8_t>() != 0;
        break;

      default:
        CapricaError::logicalFatal("Unknown VMADPropertyType %i!", (int)tp);
    }

    return val;
  }
};

struct VMADProperty final
{
  std::string name{ "" };
  VMADPropertyType type{ VMADPropertyType::Unknown };
  VMADPropertyStatus status{ VMADPropertyStatus::Unknown };
  VMADPropertyValue value{ };
  std::vector<VMADPropertyValue> arrayValue{ };

  static VMADProperty* read(CapricaBinaryReader& rdr) {
    auto prop = new VMADProperty();
    prop->name = rdr.read<std::string>();
    prop->type = (VMADPropertyType)rdr.read<uint8_t>();
    prop->status = (VMADPropertyStatus)rdr.read<uint8_t>();

    switch (prop->type) {
      case VMADPropertyType::Object:
      case VMADPropertyType::String:
      case VMADPropertyType::Integer:
      case VMADPropertyType::Float:
      case VMADPropertyType::Bool:
        prop->value = VMADPropertyValue::read(prop->type, rdr);
        break;

      case VMADPropertyType::ArrayOfObjects:
      case VMADPropertyType::ArrayOfStrings:
      case VMADPropertyType::ArrayOfIntegers:
      case VMADPropertyType::ArrayOfFloats:
      case VMADPropertyType::ArrayOfBools:
      {
        auto valCount = rdr.read<uint32_t>();
        for (size_t i = 0; i < valCount; i++)
          prop->arrayValue.push_back(VMADPropertyValue::read(prop->type, rdr));
        break;
      }

      default:
        CapricaError::logicalFatal("Unknown VMADPropertyType %i!", (int)prop->type);
    }
    
    return prop;
  }

  void write(CapricaBinaryWriter& wtr) const {
    
  }
};

}}
