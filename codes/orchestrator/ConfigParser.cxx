//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include "codes/orchestrator/ConfigParser.h"
#include "codes/CodesPropertyCollection.h"
#include "codes/LPTypeConfiguration.h"
#include "codes/SupportedLPTypes.h"

#include <c4/yml/node.hpp>
#include <ross.h>

#include <ross-extern.h>

#include <c4/std/string.hpp>
#include <ryml.hpp>
#include <ryml_std.hpp>

#include <filesystem>
#include <fstream>

namespace codes
{

namespace
{

void HandleKeyValueConfigOption(ryml::ConstNodeRef node, CodesPropertyCollection& properties)
{
  std::string propName = std::string(node.key().str, node.key().len);
  if (node.val().is_integer())
  {
    auto& prop = properties.MakeProperty(propName, PropertyType::Int);
    int value;
    auto parseSuccessful = ryml::atoi(node.val(), &value);
    prop.Set(value);
  }
  else if (node.val().is_real())
  {
    auto& prop = properties.MakeProperty(propName, PropertyType::Double);
    double value;
    auto parseSuccessful = ryml::atod(node.val(), &value);
    prop.Set(value);
  }
  else
  {
    // assume string?
    auto& prop = properties.MakeProperty(propName, PropertyType::String);
    std::string value = std::string(node.val().str, node.val().len);
    prop.Set(value);
  }
}

void HandleKeyVectorConfigOption(ryml::ConstNodeRef node, CodesPropertyCollection& properties)
{
  if (!node.is_seq())
  {
    return;
  }
  std::string propName = std::string(node.key().str, node.key().len);
  auto numElements = node.num_children();
  if (numElements == 0)
    return;

  if (node[0].val().is_integer())
  {
    auto& prop = properties.MakeProperty(propName, PropertyType::Int, numElements);
    for (int i = 0; i < numElements; i++)
    {
      int value;
      auto parseSuccessful = ryml::atoi(node[i].val(), &value);
      prop.Set(value, i);
    }
  }
  else if (node[0].val().is_real())
  {
    auto& prop = properties.MakeProperty(propName, PropertyType::Double, numElements);
    for (int i = 0; i < numElements; i++)
    {
      double value;
      auto parseSuccessful = ryml::atod(node[i].val(), &value);
      prop.Set(value, i);
    }
  }
  else
  {
    auto& prop = properties.MakeProperty(propName, PropertyType::String, numElements);
    for (int i = 0; i < numElements; i++)
    {
      std::string value = std::string(node[i].val().str, node[i].val().len);
      prop.Set(value, i);
    }
  }
}

void RecurseConfig(ryml::ConstNodeRef root, std::vector<LPTypeConfig>& lpConfigs, int lpTypeIndex)
{
  while (lpTypeIndex >= lpConfigs.size())
  {
    lpConfigs.push_back(LPTypeConfig());
  }

  auto& lpConfig = lpConfigs[lpTypeIndex];
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
      lpConfig.ModelType = ConvertLPTypeNameToEnum(lpConfig.ModelName);
    }
    else
    {
      HandleKeyValueConfigOption(root, lpConfig.Properties);
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
  for (ryml::ConstNodeRef const& child : root.children())
  {
    RecurseConfig(child, lpConfigs, lpTypeIndex);
  }
}

} // end anon namespace

bool ConfigParser::ParseConfig(const std::string& configFile)
{
  std::filesystem::path yamlPath(configFile);
  if (!std::filesystem::exists(yamlPath))
  {
    tw_error(TW_LOC, "the config file %s does not exist", configFile.c_str());
  }

  this->DOTFileName = yamlPath.parent_path();
  this->DOTFileName += "/";

  std::ifstream ifs(configFile, std::ifstream::in);
  ifs.seekg(0, ifs.end);
  int length = ifs.tellg();
  ifs.seekg(0, ifs.beg);

  std::string yaml(length, ' ');
  ifs.read(&yaml[0], yaml.size());
  ifs.close();

  // start parsing the file
  this->Tree = ryml::parse_in_arena(ryml::to_csubstr(yaml));

  ryml::ConstNodeRef root = this->Tree.rootref();
  // handle the topology section
  if (!root["topology"].has_key() && !root["topology"].is_map())
  {
    tw_error(TW_LOC, "there should be a topology section in the yaml config");
  }
  for (ryml::ConstNodeRef const& child : root["topology"])
  {
    if (child.is_keyval())
    {
      if (child.key() == "filename")
      {
        this->DOTFileName += std::string(child.val().str, child.val().len);
      }
    }
  }

  this->ParseGraphVizConfig();
  return true;
}

std::vector<LPTypeConfig> ConfigParser::GetLPTypeConfigs()
{
  std::vector<LPTypeConfig> lpConfigs;
  ryml::ConstNodeRef root = this->Tree.rootref();

  int typeIndex = 0;
  for (ryml::ConstNodeRef const& child : root.children())
  {
    if (child.has_key() &&
        (child.key() == "simulation" || child.key() == "topology" || child.key() == "site"))
    {
      continue;
    }
    RecurseConfig(child, lpConfigs, typeIndex);
    typeIndex++;
  }

  return lpConfigs;
}

CodesPropertyCollection ConfigParser::GetSimulationConfig()
{
  ryml::ConstNodeRef root = this->Tree.rootref();
  CodesPropertyCollection simConfig;
  // handle the simulation config section
  if (!root["simulation"].has_key() && !root["simulation"].is_map())
  {
    tw_error(TW_LOC, "there should be a simulation section in the yaml config");
  }
  for (ryml::ConstNodeRef const& child : root["simulation"])
  {
    if (child.is_keyval())
    {
      HandleKeyValueConfigOption(child, simConfig);
    }
    else if (child.has_key() && child.is_seq())
    {
      HandleKeyVectorConfigOption(child, simConfig);
    }
  }
  return simConfig;
}

Agraph_t* ConfigParser::ParseGraphVizConfig()
{
  if (!std::filesystem::exists(this->DOTFileName))
  {
    tw_error(TW_LOC, "the topology (DOT) file %s does not exist", this->DOTFileName.c_str());
  }

  // can't use ifstream since graphviz is a C lib
  FILE* fp = fopen(this->DOTFileName.c_str(), "r");
  // TODO: check for errors

  Agraph_t* graph = agread(fp, 0);
  if (!graph)
  {
    tw_error(TW_LOC, "graph in file %s could not be read by GraphViz", this->DOTFileName.c_str());
  }
  return graph;
}

} // end namespace codes
