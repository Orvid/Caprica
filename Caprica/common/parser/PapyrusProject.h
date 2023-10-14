#pragma once

#include "common/GameID.h"

#include <map>
#include <string>
#include <vector>
namespace caprica {

/**
 * PapyrusProject.xsd:
 * <?xml version="1.0" encoding="UTF-8"?>
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

        */
/**
 * Represents a Papyrus project (.ppj) file.
 *
 * See PapyrusProject.xsd for the schema.
 */
enum RecurseSetting {
  NotSet = -1,
  Recurse,
  NoRecurse
};

enum class BoolSetting : uint8_t {
  False,
  True,
  NOT_SET = 0xFF,
};

struct RecursablePath {
  bool noRecurse = false;
  std::string path = "";
};
using IncludePattern = RecursablePath;
struct MatchPattern : public RecursablePath {
  std::string in = "";
  std::string exclude = "";
};
using Match = MatchPattern;
using Include = IncludePattern;

struct IncludeBase {
  std::string name;
  std::string rootDir;
  std::vector<Include> includes;
  std::vector<Match> matches;
};
struct IncludeZip : public IncludeBase {
  std::string compression;
};

using Folder = RecursablePath;
using Package = IncludeBase;
using ZipFile = IncludeZip;

using VariableList = std::map<std::string, std::string>;
using ImportList = std::vector<std::string>;
using FolderList = std::vector<Folder>;
using ScriptList = std::vector<std::string>;

struct PackageList {
  std::vector<Package> packages;
  std::string output;
};
struct ZipList {
  std::vector<ZipFile> zipFiles;
  std::string output;
};
struct CommandList {
  std::vector<std::string> commands;
  std::string description;
  bool useInBuild = false;
};

struct PapyrusProject {
  // different values than GameID
  enum class PapyrusGame : uint16_t {
    INVALID = 0xFFFF,
    UNKNOWN = 0,
    Skyrim = 1,
    Fallout4 = 2,
    Fallout76 = 3,
    Starfield = 4,
    SkyrimSpecialEdition = 0xFF01,
    NOT_SET = UNKNOWN
  };

  enum class AsmType : uint8_t {
    None,
    Keep,
    Only,
    Discard,
    NOT_SET = 0xFF,
  };

  // Attributes
  PapyrusGame game = PapyrusGame::NOT_SET;
  BoolSetting optimize = BoolSetting::NOT_SET;
  BoolSetting release = BoolSetting::NOT_SET;
  BoolSetting finalAttr = BoolSetting::NOT_SET;
  BoolSetting anonymize = BoolSetting::NOT_SET;
  BoolSetting package = BoolSetting::NOT_SET;
  BoolSetting zip = BoolSetting::NOT_SET;
  std::string output; // required
  std::string flags;  // required
  AsmType asmAttr = AsmType::NOT_SET;

  // elements
  VariableList variables;
  ImportList imports;
  FolderList folders;
  ScriptList scripts;
  PackageList packages;
  ZipList zipFiles;
  CommandList preBuildEvent;
  CommandList postBuildEvent;
  CommandList preImportEvent;
  CommandList postImportEvent;
  CommandList preCompileEvent;
  CommandList postCompileEvent;
  CommandList preAnonymizeEvent;
  CommandList postAnonymizeEvent;
  CommandList prePackageEvent;
  CommandList postPackageEvent;
  CommandList preZipEvent;
  CommandList postZipEvent;
};
} // namespace caprica
