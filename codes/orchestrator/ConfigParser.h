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

#include "codes/SupportedLPTypes.h"

#include <graphviz/cgraph.h>

#include <c4/yml/node.hpp>
#include <ryml.hpp>
#include <ryml_std.hpp>

#include <string>

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
  CodesLPTypes ModelType;
  std::vector<std::string> NodeNames;
};

struct SimulationConfig
{
  int PacketSize;
  int ROSSMessageSize;
  std::string ModelNetScheduler;
  std::vector<std::string> ModelNetOrder;

  // TODO: maybe should store these differently, so that when
  // a model asks for a value, it can determine that it was actually set and we're not just
  // using a bad value
  // for simplep2p
  std::string LatencyFileName;
  std::string BandwidthFileName;

  // for simplenet
  double NetworkStartupNS;
  int NetworkBandwidthMbps;
};

/**
 * Handles parsing the yaml and DOT config files.
 */
class ConfigParser
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
   * Returns the graph from graphviz
   */
  Agraph_t* GetGraph();

private:
  std::string DOTFileName;

  std::vector<LPTypeConfig> LPConfigs;

  SimulationConfig SimConfig;

  std::string ParentDir;

  ryml::Tree Tree;

  Agraph_t* Graph;

  void RecurseConfig(ryml::ConstNodeRef root, int lpTypeIndex);

  void ParseGraphVizConfig();
};

} // end namespace codes

#endif // CODES_YAML_PARSER_H
