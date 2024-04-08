//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include "codes/orchestrator/CodesYAML.h"

#include <ryml.hpp>
#include <ryml_std.hpp>
#include <c4/std/string.hpp>

#include <fstream>
#include <filesystem>
#include <iostream>
#include <sstream>

namespace codes
{
namespace orchestrator
{

namespace
{

void recurseConfig(ryml::ConstNodeRef root, LPConfig& lpConfig, std::string indent = "")
{
  if (root.is_keyval())
  {
    if (root.key() == "type")
    {
      if (root.val() == "switch")
        lpConfig.Type = ComponentType::Switch;
      else if (root.val() == "router")
        lpConfig.Type = ComponentType::Router;
      else if (root.val() == "host")
        lpConfig.Type = ComponentType::Host;
    }
    else if (root.key() == "model")
    {
      lpConfig.ModelName = std::string(root.val().str, root.val().len);
    }
    return;
  }
  else if (root.has_key() && root.is_seq())
  {
    lpConfig.NodeNames.resize(root.num_children());
    for (int i = 0; i < root.num_children(); i++)
    {
      lpConfig.NodeNames[i] = std::string(root[i].val().str, root[i].val().len);
    }
  }
  else if (root.has_key() && root.is_map())
  {
    if (root.key() != "simulation" && root.key() != "topology")
    {
      lpConfig.ConfigName = std::string(root.key().str, root.key().len);
    }
  }
  indent += "\t";
  for (ryml::ConstNodeRef const& child : root.children())
  {
    recurseConfig(child, lpConfig, indent);
  }
}

} // end anon namespace

CodesYAML::CodesYAML()
{


}

void CodesYAML::ParseConfig(const std::string& configFile, std::vector<LPConfig>& lpConfigs)
{
  if (!std::filesystem::exists(configFile))
  {
    std::cout << "the config file " << configFile << " does not exist!" << std::endl;
  }

  std::ifstream ifs(configFile, std::ifstream::in);
  ifs.seekg(0, ifs.end);
  int length = ifs.tellg();
  ifs.seekg(0, ifs.beg);

  std::string yaml(length, ' ');
  ifs.read(&yaml[0], yaml.size());
  ifs.close();
  //std::cout << "yaml:\n" << yaml << std::endl;

  // start parsing the file
  ryml::Tree tree = ryml::parse_in_arena(ryml::to_csubstr(yaml));

  ryml::ConstNodeRef root = tree.rootref();
  lpConfigs.resize(root.num_children());
  int typeIndex = 0;
  for (ryml::ConstNodeRef const& child : root.children())
  {
    recurseConfig(child, lpConfigs[typeIndex]);
    typeIndex++;
  }

}

} // end namespace orchestrator
} // end namespace codes
