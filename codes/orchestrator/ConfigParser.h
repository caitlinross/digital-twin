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

#include "codes/CodesPropertyCollection.h"
#include "codes/LPTypeConfiguration.h"

#include <graphviz/cgraph.h>

#include <c4/yml/node.hpp>
#include <ryml.hpp>
#include <ryml_std.hpp>

#include <string>

namespace codes
{

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
   * returns the configuration for all lp types
   */
  std::vector<LPTypeConfig> GetLPTypeConfigs();

  /**
   * Returns the general simulation settings
   */
  CodesPropertyCollection GetSimulationConfig();

  Agraph_t* ParseGraphVizConfig();

private:
  std::string DOTFileName;

  ryml::Tree Tree;
};

} // end namespace codes

#endif // CODES_YAML_PARSER_H
