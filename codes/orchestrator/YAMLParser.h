//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef CODES_YAML_PARSER_H
#define CODES_YAML_PARSER_H

#include "codes/orchestrator/GraphVizConfig.h"
#include <c4/yml/node.hpp>
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
  std::string ConfigName; // name used in graphviz
  std::string ModelName;  // the codes LP name
  std::vector<std::string> NodeNames;
};

struct SimulationConfig
{
  int PacketSize;
  int ROSSMessageSize;
  // TODO prob switch to enum?
  std::string ModelNetScheduler;
  std::string LatencyFileName;
  std::string BandwidthFileName;
};

class YAMLParser
{
public:
  YAMLParser();

  void ParseConfig(const std::string& configFile);

  std::string GetParentPath();

  std::vector<LPTypeConfig>& GetLPTypeConfigs();
  std::vector<int>& GetLPTypeConfigIndices();
  const SimulationConfig& GetSimulationConfig();
  ryml::Tree& GetYAMLTree();

  GraphVizConfig& GetGraphConfig();

private:
  std::string YamlString;
  std::string DOTFileName;

  std::vector<LPTypeConfig> LPConfigs;
  // this is an index into this->LPConfigs
  // this is an LPs local id mapped to its LP type
  std::vector<int> LPTypeConfigIndices;

  SimulationConfig SimConfig;

  GraphVizConfig GraphConfig;

  std::string ParentDir;

  ryml::Tree Tree;

  void RecurseConfig(ryml::ConstNodeRef root, int lpTypeIndex);
};

} // end namespace orchestrator
} // end namespace codes

#endif // CODES_YAML_PARSER_H
