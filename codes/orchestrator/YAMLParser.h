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
  std::string ModelNetScheduler;
  std::string LatencyFileName;
  std::string BandwidthFileName;
};

/**
 * Handles parsing the yaml config file.
 */
class YAMLParser
{
public:
  /**
   * Parses the configFile
   */
  bool ParseConfig(const std::string& configFile);

  /**
   * Returns the directory containing the yaml file
   */
  std::string GetParentPath();

  /**
   * returns the configuration for all lp types
   */
  std::vector<LPTypeConfig>& GetLPTypeConfigs();

  /**
   * Returns the general simulation settings
   */
  const SimulationConfig& GetSimulationConfig();

  /**
   * returns the graph viz parser
   */
  GraphVizConfig& GetGraphConfig();

private:
  std::string DOTFileName;

  std::vector<LPTypeConfig> LPConfigs;

  SimulationConfig SimConfig;

  GraphVizConfig GraphConfig;

  std::string ParentDir;

  ryml::Tree Tree;

  void RecurseConfig(ryml::ConstNodeRef root, int lpTypeIndex);
};

} // end namespace codes

#endif // CODES_YAML_PARSER_H
