//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef CODES_ORCHESTRATOR_CODESYAML_H
#define CODES_ORCHESTRATOR_CODESYAML_H

#include <string>

namespace codes
{
namespace orchestrator
{

enum class ComponentType
{
  Switch,
  Router,
  Host
};

// config info for a specific component
struct LPConfig
{
  ComponentType Type;
  std::string ConfigName;
  std::string ModelName;
  std::vector<std::string> NodeNames;
};

//struct LPGroups
//{
//  // not necessary, just get Groups.size()
//  //int NumLPGroups;
//  int NumUniqueLPTypes;
//  std::vector<LPGroup> Groups;
//};

class CodesYAML
{
public:
  CodesYAML();

  void ParseConfig(const std::string& configFile, std::vector<LPConfig>& lpConfigs);

private:
  std::string YamlString;

};

} // end namespace orchestrator
} // end namespace codes

#endif // CODES_ORCHESTRATOR_CODESYAML_H
