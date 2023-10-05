#include "CapricaPPJParser.h"
#include "common/CaselessStringComparer.h"
#include <boost/algorithm/string/case_conv.hpp>
#include <common/identifier_ref.h>
#include <common/parser/PapyrusProject.h>
#include <iostream>
#include <map>
#include <pugixml.hpp>
namespace caprica {
/**
 * PapyrusProject.xsd:
 ```xml
 <?xml version="1.0" encoding="UTF-8"?>
<xs:schema
    elementFormDefault="qualified"
    id="PapyrusProject"
    targetNamespace="PapyrusProject.xsd"
    xmlns="PapyrusProject.xsd"
    xmlns:pyro="PapyrusProject.xsd"
    xmlns:xs="http://www.w3.org/2001/XMLSchema">

<!-- Elements -->
    <xs:element name="PapyrusProject">
    <xs:complexType>
    <xs:sequence>
    <xs:choice maxOccurs="unbounded">
    <xs:element minOccurs="0" name="Variables" type="pyro:variableList"/>
    <xs:element minOccurs="0" name="Imports" type="pyro:importList"/>
    <xs:element minOccurs="0" name="Folders" type="pyro:folderList"/>
    <xs:element minOccurs="0" name="Scripts" type="pyro:scriptList"/>
    <xs:element minOccurs="0" name="Packages" type="pyro:packageList"/>
    <xs:element minOccurs="0" name="ZipFiles" type="pyro:zipList"/>
    <xs:element minOccurs="0" name="PreBuildEvent" type="pyro:commandList"/>
    <xs:element minOccurs="0" name="PostBuildEvent" type="pyro:commandList"/>
    <xs:element minOccurs="0" name="PreImportEvent" type="pyro:commandList"/>
    <xs:element minOccurs="0" name="PostImportEvent" type="pyro:commandList"/>
    <xs:element minOccurs="0" name="PreCompileEvent" type="pyro:commandList"/>
    <xs:element minOccurs="0" name="PostCompileEvent" type="pyro:commandList"/>
    <xs:element minOccurs="0" name="PreAnonymizeEvent" type="pyro:commandList"/>
    <xs:element minOccurs="0" name="PostAnonymizeEvent" type="pyro:commandList"/>
    <xs:element minOccurs="0" name="PrePackageEvent" type="pyro:commandList"/>
    <xs:element minOccurs="0" name="PostPackageEvent" type="pyro:commandList"/>
    <xs:element minOccurs="0" name="PreZipEvent" type="pyro:commandList"/>
    <xs:element minOccurs="0" name="PostZipEvent" type="pyro:commandList"/>
    </xs:choice>
    </xs:sequence>
    <xs:attribute name="Game" type="pyro:gameType"/>
    <xs:attribute name="Output" type="xs:string"/>
    <xs:attribute name="Flags" type="xs:string"/>
    <xs:attribute name="Asm" type="pyro:asmType" default="none"/>
    <xs:attribute name="Optimize" type="pyro:bool" default="false"/>
    <xs:attribute name="Release" type="pyro:bool" default="false"/>
    <xs:attribute name="Final" type="pyro:bool" default="false"/>
    <xs:attribute name="Anonymize" type="pyro:bool" default="false"/>
    <xs:attribute name="Package" type="pyro:bool" default="false"/>
    <xs:attribute name="Zip" type="pyro:bool" default="false"/>
    </xs:complexType>
    </xs:element>
    <xs:element name="Folder" type="pyro:recursablePath"/>
    <xs:element name="Include" type="pyro:includePattern"/>
    <xs:element name="Match" type="pyro:matchPattern"/>
    <xs:element name="Package" type="pyro:includeBase"/>
    <xs:element name="ZipFile" type="pyro:includeZip"/>

    <!-- Complex Types -->
    <xs:complexType name="variableList">
    <xs:sequence>
    <xs:element maxOccurs="unbounded" name="Variable" type="pyro:nameValuePair"/>
    </xs:sequence>
    </xs:complexType>
    <xs:complexType name="importList">
    <xs:sequence>
    <xs:element maxOccurs="unbounded" name="Import" type="xs:string"/>
    </xs:sequence>
    </xs:complexType>
    <xs:complexType name="folderList">
    <xs:sequence>
    <xs:element maxOccurs="unbounded" ref="pyro:Folder"/>
    </xs:sequence>
    </xs:complexType>
    <xs:complexType name="scriptList">
    <xs:sequence>
    <xs:element maxOccurs="unbounded" name="Script" type="xs:string"/>
    </xs:sequence>
    </xs:complexType>
    <xs:complexType name="packageList">
    <xs:sequence>
    <xs:element maxOccurs="unbounded" ref="pyro:Package"/>
    </xs:sequence>
    <xs:attribute name="Output" type="xs:string"/>
    </xs:complexType>
    <xs:complexType name="zipList">
    <xs:sequence>
    <xs:element maxOccurs="unbounded" ref="pyro:ZipFile"/>
    </xs:sequence>
    <xs:attribute name="Output" type="xs:string"/>
    </xs:complexType>
    <xs:complexType name="commandList">
    <xs:sequence>
    <xs:element maxOccurs="unbounded" name="Command" type="xs:string"/>
    </xs:sequence>
    <xs:attribute name="Description" type="xs:string"/>
    <xs:attribute name="UseInBuild" type="pyro:bool"/>
    </xs:complexType>

    <!-- Reusable Complex Types -->
    <xs:complexType name="nameValuePair">
    <xs:attribute name="Name" type="xs:string" use="required"/>
    <xs:attribute name="Value" type="xs:string" use="required"/>
    </xs:complexType>
    <xs:complexType name="recursablePath" mixed="true">
    <xs:attribute name="NoRecurse" type="pyro:bool" default="false"/>
    </xs:complexType>
    <xs:complexType name="includePattern" mixed="true">
    <xs:complexContent>
    <xs:extension base="recursablePath">
    <xs:attribute name="Path" type="xs:string" default=""/>
    </xs:extension>
    </xs:complexContent>
    </xs:complexType>
    <xs:complexType name="matchPattern" mixed="true">
    <xs:complexContent>
    <xs:extension base="recursablePath">
    <xs:attribute name="In" type="xs:string" default=""/>
    <xs:attribute name="Exclude" type="xs:string" default=""/>
    <xs:attribute name="Path" type="xs:string" default=""/>
    </xs:extension>
    </xs:complexContent>
    </xs:complexType>
    <xs:complexType name="includeBase">
    <xs:sequence>
    <xs:choice minOccurs="0" maxOccurs="unbounded">
    <xs:element maxOccurs="unbounded" ref="pyro:Include"/>
    <xs:element maxOccurs="unbounded" ref="pyro:Match"/>
    </xs:choice>
    </xs:sequence>
    <xs:attribute name="Name" type="xs:string"/>
    <xs:attribute name="RootDir" type="xs:string" use="required"/>
    </xs:complexType>
    <xs:complexType name="includeZip">
    <xs:complexContent>
    <xs:extension base="includeBase">
    <xs:attribute name="Compression" type="pyro:compressionType" default="deflate"/>
    </xs:extension>
    </xs:complexContent>
    </xs:complexType>

    <!-- Simple Types -->
    <xs:simpleType name="asmType">
    <xs:restriction base="xs:string">
    <xs:pattern value="[nN][oO][nN][eE]"/>
    <xs:pattern value="[kK][eE][eE][pP]"/>
    <xs:pattern value="[oO][nN][lL][yY]"/>
    <xs:pattern value="[dD][iI][sS][cC][aA][rR][dD]"/>
    </xs:restriction>
    </xs:simpleType>
    <xs:simpleType name="bool">
    <xs:restriction base="xs:string">
    <xs:pattern value="[tT][rR][uU][eE]"/>
    <xs:pattern value="[fF][aA][lL][sS][eE]"/>
    <xs:pattern value="[01]"/>
    </xs:restriction>
    </xs:simpleType>
    <xs:simpleType name="gameType">
    <xs:restriction base="xs:string">
    <xs:pattern value="[sS][fF]1"/>
    <xs:pattern value="[sS][sS][eE]"/>
    <xs:pattern value="[tT][eE][sS][5vV]"/>
    <xs:pattern value="[fF][oO]4"/>
    </xs:restriction>
    </xs:simpleType>
    <xs:simpleType name="compressionType">
    <xs:restriction base="xs:string">
    <xs:pattern value="[sS][tT][oO][rR][eE]"/>
    <xs:pattern value="[dD][eE][fF][lL][aA][tT][eE]"/>
    </xs:restriction>
    </xs:simpleType>
    </xs:schema>
```
        */

/**
 *  Parses a PPJ file and returns a PapyrusProject object.
 *  The PPJ file is an XML file that describes a Papyrus project.
 *  See PapyrusProject.xsd for the schema.
 */

static caseless_unordered_identifier_map<PapyrusProject::PapyrusGame> GAME_MAP = {
  {"sf1",   PapyrusProject::PapyrusGame::Starfield           },
  { "sse",  PapyrusProject::PapyrusGame::SkyrimSpecialEdition},
  { "tes5", PapyrusProject::PapyrusGame::Skyrim              },
  { "fo4",  PapyrusProject::PapyrusGame::Fallout4            },
  { "f76",  PapyrusProject::PapyrusGame::Fallout76           },
};
static caseless_unordered_identifier_map<PapyrusProject::AsmType> ASM_MAP = {
  {"none",     PapyrusProject::AsmType::None   },
  { "keep",    PapyrusProject::AsmType::Keep   },
  { "only",    PapyrusProject::AsmType::Only   },
  { "discard", PapyrusProject::AsmType::Discard},
};

static caseless_unordered_identifier_map<bool> BOOL_MAP = {
  {"true",   true },
  { "false", false},
  { "1",     true },
  { "0",     false},
};

static std::vector<std::string> rootAttributes {
  "Game", "Output", "Flags", "Asm", "Optimize", "Release", "Final", "Anonymize", "Package", "Zip",
  "xmlns" // ignore xmlns attribute
};

// variables are case insensitive and ascii-only
std::string CapricaPPJParser::substituteString(const char* text) {
  if (strlen(text) == 0)
    return text;

  std::string str = text;
  if (str.find('@') == std::string::npos)
    return str;

  std::string strLower = boost::to_lower_copy(str);
  for (auto& varPair : variables) {
    size_t pos = strLower.find(varPair.first);
    while (pos != std::string::npos) {
      str.replace(pos, varPair.first.size(), varPair.second);
      strLower.replace(pos, varPair.first.size(), varPair.second);
      pos = strLower.find(varPair.first, pos + 1);
    }
  }
  return str;
}

std::string CapricaPPJParser::ParseString(const pugi::xml_node& node) {
  if (variables.size() == 0)
    return node.text().as_string();
  return substituteString(node.text().as_string());
}
std::string CapricaPPJParser::ParseString(const pugi::xml_attribute& attr) {
  if (variables.size() == 0)
    return attr.as_string();
  return substituteString(attr.as_string());
}

IncludeBase CapricaPPJParser::ParseIncludeBase(const pugi::xml_node& node) {
  IncludeBase base;
  base.name = ParseString(node.attribute("Name"));
  base.rootDir = ParseString(node.attribute("RootDir"));
  if (base.rootDir.empty())
    throw std::runtime_error("IncludeBase must have a RootDir attribute!");
  for (pugi::xml_node& child : node.children()) {
    if (strcmp(child.name(), "Include") == 0) {
      auto includePattern = Include { .path = ParseString(child) };
      includePattern.noRecurse = child.attribute("NoRecurse").as_bool();
      base.includes.emplace_back(includePattern);
    } else if (strcmp(child.name(), "Match") == 0) {
      auto match = Match {
        .in = ParseString(child.attribute("In")),
        .exclude = ParseString(child.attribute("Exclude")),
      };
      match.path = ParseString(child.attribute("Path"));
      match.noRecurse = child.attribute("NoRecurse").as_bool();
      base.matches.emplace_back(match);
    }
  }
  return base;
}

inline CommandList CapricaPPJParser::ParseCommandList(const pugi::xml_node& node) {
  CommandList list;
  list.description = ParseString(node.attribute("Description"));
  list.useInBuild = node.attribute("UseInBuild").as_bool();
  for (pugi::xml_node& child : node.children())
    if (strcmp(child.name(), "Command") == 0)
      list.commands.emplace_back(ParseString(child));
  return list;
}

inline PapyrusProject::PapyrusGame ParseGameAttribute(const pugi::xml_node& root) {
  std::string game = root.attribute("Game").as_string();
  if (game.empty())
    throw std::runtime_error("PapyrusProject must have a Game attribute!");
  auto it = GAME_MAP.find(game);
  if (it == GAME_MAP.end())
    throw std::runtime_error("PapyrusProject has an invalid Game attribute!");
  return it->second;
}
inline PapyrusProject::AsmType ParseAsmAttribute(const pugi::xml_node& root) {
  std::string asmAttr = root.attribute("Asm").as_string();
  if (asmAttr.empty())
    return PapyrusProject::AsmType::None;
  auto it = ASM_MAP.find(asmAttr);
  if (it == ASM_MAP.end())
    throw std::runtime_error("PapyrusProject has an invalid Asm attribute!");
  return it->second;
}
inline bool ParseBoolTypeAttribute(const pugi::xml_node& node, const char* name, bool defaultValue = false) {
  std::string attr = node.attribute(name).as_string();
  if (attr.empty())
    return defaultValue;
  auto it = BOOL_MAP.find(attr);
  if (it == BOOL_MAP.end())
    throw std::runtime_error("PapyrusProject has an invalid " + std::string(name) + " attribute!");
  return it->second;
}

bool CapricaPPJParser::ParseVariables(const pugi::xml_node& node) {
  for (pugi::xml_node& child : node.children()) {
    if (strcmp(child.name(), "Variable") == 0) {
      std::string name = child.attribute("Name").as_string();
      boost::to_lower(name);
      variables.emplace_back(std::pair { "@" + name, child.attribute("Value").as_string() });
    }
  }
  // now, sort the variables by length, so that we can replace the longest ones first
  std::sort(variables.begin(), variables.end(), [](const auto& a, const auto& b) {
    return a.first.size() > b.first.size();
  });
  return true;
}

PapyrusProject CapricaPPJParser::Parse(const std::string& path) {
  pugi::xml_document doc;
  pugi::xml_parse_result result = doc.load_file(path.c_str());
  if (!result) {
    std::cerr << "Error parsing PPJ file: " << result.description() << std::endl;
    return {};
  }
  pugi::xml_node root = doc.child("PapyrusProject");
  if (!root) {
    std::cerr << "Error parsing PPJ file: No PapyrusProject node found!" << std::endl;
    return {};
  }
  PapyrusProject project;
  // check if it has any other attributes besides the ones in the xsd
  for (pugi::xml_attribute attr : root.attributes())
    if (std::find(rootAttributes.begin(), rootAttributes.end(), attr.name()) == rootAttributes.end())
      std::cerr << "Warning: PapyrusProject has an unknown attribute: " << attr.name() << std::endl;

  // convert to enum
  project.game = ParseGameAttribute(root);
  project.asmAttr = ParseAsmAttribute(root);
  project.optimize = ParseBoolTypeAttribute(root, "Optimize");
  project.release = ParseBoolTypeAttribute(root, "Release");
  project.finalAttr = ParseBoolTypeAttribute(root, "Final");
  project.anonymize = ParseBoolTypeAttribute(root, "Anonymize");
  project.package = ParseBoolTypeAttribute(root, "Package");
  project.zip = ParseBoolTypeAttribute(root, "Zip");

  // we have to do variables first
  for (pugi::xml_node& child : root.children())
    if (strcmp(child.name(), "Variables") == 0)
      ParseVariables(child);

  project.output = ParseString(root.attribute("Output"));
  project.flags = ParseString(root.attribute("Flags"));
  if (project.output.empty())
    throw std::runtime_error("PapyrusProject must have an Output attribute!");
  if (project.flags.empty())
    throw std::runtime_error("PapyrusProject must have a Flags attribute!");

  for (pugi::xml_node& child : root.children()) {
    if (strcmp(child.name(), "Imports") == 0) {
      for (pugi::xml_node& import : child.children())
        if (strcmp(import.name(), "Import") == 0)
          project.imports.emplace_back();
    } else if (strcmp(child.name(), "Folders") == 0) {
      for (pugi::xml_node& folderElement : child.children()) {
        if (strcmp(folderElement.name(), "Folder") == 0) {
          auto flder = Folder { .path = ParseString(folderElement) };
          flder.noRecurse = folderElement.attribute("NoRecurse").as_bool();
          project.folders.emplace_back(flder);
        }
      }
    } else if (strcmp(child.name(), "Scripts") == 0) {
      for (pugi::xml_node& script : child.children())
        if (strcmp(script.name(), "Script") == 0)
          project.scripts.emplace_back(ParseString(script));
    } // we do not care about the rest of these
    //    else if (strcmp(child.name(), "Packages") == 0) {
    //      project.packages.output = ParseString(child.attribute("Output"));
    //      for (pugi::xml_node& package : child.children()) {
    //        if (strcmp(package.name(), "Package") == 0) {
    //          Package pack = ParseIncludeBase(package);
    //          project.packages.packages.emplace_back(pack);
    //        }
    //      }
    //    } else if (strcmp(child.name(), "ZipFiles") == 0) {
    //      project.zipFiles.output = ParseString(child.attribute("Output"));
    //      for (pugi::xml_node& zipFileElement : child.children()) {
    //        if (strcmp(zipFileElement.name(), "ZipFile") == 0) {
    //          ZipFile zip = (ZipFile)ParseIncludeBase(zipFileElement);
    //          zip.compression = ParseString(zipFileElement.attribute("Compression"));
    //          project.zipFiles.zipFiles.emplace_back(zip);
    //        }
    //      }
    //    } else if (strcmp(child.name(), "PreBuildEvent") == 0) {
    //      project.preBuildEvent = ParseCommandList(child);
    //    } else if (strcmp(child.name(), "PostBuildEvent") == 0) {
    //      project.postBuildEvent = ParseCommandList(child);
    //    } else if (strcmp(child.name(), "PreImportEvent") == 0) {
    //      project.preImportEvent = ParseCommandList(child);
    //    } else if (strcmp(child.name(), "PostImportEvent") == 0) {
    //      project.postImportEvent = ParseCommandList(child);
    //    } else if (strcmp(child.name(), "PreCompileEvent") == 0) {
    //      project.preCompileEvent = ParseCommandList(child);
    //    } else if (strcmp(child.name(), "PostCompileEvent") == 0) {
    //      project.postCompileEvent = ParseCommandList(child);
    //    } else if (strcmp(child.name(), "PreAnonymizeEvent") == 0) {
    //      project.preAnonymizeEvent = ParseCommandList(child);
    //    } else if (strcmp(child.name(), "PostAnonymizeEvent") == 0) {
    //      project.postAnonymizeEvent = ParseCommandList(child);
    //    } else if (strcmp(child.name(), "PrePackageEvent") == 0) {
    //      project.prePackageEvent = ParseCommandList(child);
    //    } else if (strcmp(child.name(), "PostPackageEvent") == 0) {
    //      project.postPackageEvent = ParseCommandList(child);
    //    } else if (strcmp(child.name(), "PreZipEvent") == 0) {
    //      project.preZipEvent = ParseCommandList(child);
    //    } else if (strcmp(child.name(), "PostZipEvent") == 0) {
    //      project.postZipEvent = ParseCommandList(child);
    //    }
  }
  return project;
}
}
