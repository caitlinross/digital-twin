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
#include "codes/SupportedLPTypes.h"

#include <iostream>
#include <ross.h>

#include <ross-extern.h>

#include <c4/std/string.hpp>
#include <ryml.hpp>
#include <ryml_std.hpp>

#include <filesystem>
#include <fstream>

namespace codes
{

bool ConfigParser::ParseConfig(const std::string& configFile)
{
  std::filesystem::path yamlPath(configFile);
  if (!std::filesystem::exists(yamlPath))
  {
    tw_error(TW_LOC, "the config file %s does not exist", configFile.c_str());
  }

  this->ParentDir = yamlPath.parent_path();
  this->DOTFileName = this->ParentDir;
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

  int typeIndex = 0;
  for (ryml::ConstNodeRef const& child : root.children())
  {
    if (child.has_key() &&
        (child.key() == "simulation" || child.key() == "topology" || child.key() == "site"))
    {
      continue;
    }
    this->RecurseConfig(child, typeIndex);
    typeIndex++;
  }

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

  // handle the simulation config section
  if (!root["simulation"].has_key() && !root["simulation"].is_map())
  {
    tw_error(TW_LOC, "there should be a simulation section in the yaml config");
  }
  for (ryml::ConstNodeRef const& child : root["simulation"])
  {
    if (child.is_keyval())
    {
      if (child.key() == "packet_size")
      {
        auto parseSuccessful = ryml::atoi(child.val(), &this->SimConfig.PacketSize);
        // TODO: error checking/defaults
      }
      else if (child.key() == "modelnet_scheduler")
      {
        this->SimConfig.ModelNetScheduler = std::string(child.val().str, child.val().len);
      }
      else if (child.key() == "ross_message_size")
      {
        auto parseSuccessful = ryml::atoi(child.val(), &this->SimConfig.ROSSMessageSize);
        // TODO: error checking/defaults
      }
      else if (child.key() == "net_latency_ns_file")
      {
        this->SimConfig.LatencyFileName = std::string(child.val().str, child.val().len);
      }
      else if (child.key() == "net_bw_mbps_file")
      {
        this->SimConfig.BandwidthFileName = std::string(child.val().str, child.val().len);
      }
      else if (child.key() == "net_startup_ns")
      {
        auto parseSuccessful = ryml::atod(child.val(), &this->SimConfig.NetworkStartupNS);
        // TODO: error checking/defaults
      }
      else if (child.key() == "net_bw_mbps")
      {
        auto parseSuccessful = ryml::atoi(child.val(), &this->SimConfig.NetworkBandwidthMbps);
        // TODO: error checking/defaults
      }
    }
    else if (child.has_key())
    {
      if (child.key() == "modelnet_order" && child.is_seq())
      {
        this->SimConfig.ModelNetOrder.resize(child.num_children());
        for (int i = 0; i < child.num_children(); i++)
        {
          this->SimConfig.ModelNetOrder[i] = std::string(child[i].val().str, child[i].val().len);
          std::cout << "adding " << SimConfig.ModelNetOrder[i] << " to modelnet order" << std::endl;
        }
      }
    }
  }

  this->ParseGraphVizConfig();
  return true;
}

std::string ConfigParser::GetParentPath()
{
  return this->ParentDir;
}

std::vector<LPTypeConfig>& ConfigParser::GetLPTypeConfigs()
{
  return this->LPConfigs;
}

const SimulationConfig& ConfigParser::GetSimulationConfig()
{
  return this->SimConfig;
}

void ConfigParser::RecurseConfig(ryml::ConstNodeRef root, int lpTypeIndex)
{
  while (lpTypeIndex >= this->LPConfigs.size())
  {
    this->LPConfigs.push_back(LPTypeConfig());
  }

  auto& lpConfig = this->LPConfigs[lpTypeIndex];
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
    this->RecurseConfig(child, lpTypeIndex);
  }
}

void ConfigParser::ParseGraphVizConfig()
{
  if (!std::filesystem::exists(this->DOTFileName))
  {
    tw_error(TW_LOC, "the topology (DOT) file %s does not exist", this->DOTFileName.c_str());
  }

  // can't use ifstream since graphviz is a C lib
  FILE* fp = fopen(this->DOTFileName.c_str(), "r");
  // TODO: check for errors

  this->Graph = agread(fp, 0);
  if (!this->Graph)
  {
    tw_error(TW_LOC, "graph in file %s could not be read by GraphViz", this->DOTFileName.c_str());
  }
}

Agraph_t* ConfigParser::GetGraph()
{
  return this->Graph;
}

} // end namespace codes
