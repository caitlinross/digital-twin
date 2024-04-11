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

#include <ryml.hpp>
#include <ryml_std.hpp>

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
struct LPTypeConfig
{
  ComponentType Type;
  std::string ConfigName;
  std::string ModelName; // the codes LP name
  std::vector<std::string> NodeNames;
};

struct SimulationConfig
{
  int PacketSize;
  int ROSSMessageSize;
  // TODO prob switch to enum?
  std::string ModelNetScheduler;
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

  void ParseConfig(const std::string& configFile);

  std::vector<LPTypeConfig>& GetLPTypeConfigs();
  std::vector<int>& GetLPTypeConfigIndices();
  const SimulationConfig& GetSimulationConfig();

private:
  std::string YamlString;
  std::string DOTFileName;

  std::vector<LPTypeConfig> LPConfigs;
  // this is an index into this->LPConfigs
  // this is an LPs local id mapped to its LP type
  std::vector<int> LPTypeConfigIndices;

  SimulationConfig SimConfig;

  void RecurseConfig(ryml::ConstNodeRef root, int lpTypeIndex);
};

} // end namespace orchestrator
} // end namespace codes

#endif // CODES_ORCHESTRATOR_CODESYAML_H
