//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include "codes/orchestrator/Orchestrator.h"
#include "codes/LPTypeConfiguration.h"
#include "codes/SupportedLPTypes.h"
#include "codes/codes.h"
#include "codes/lp-type-lookup.h"
#include "codes/mapping/Mapper.h"
#include "codes/model-net/model-net.h"
#include "codes/orchestrator/ConfigParser.h"
#include "mpi.h"

#include <filesystem>
#include <iostream>
#include <memory>
#include <ross.h>

namespace codes
{

Orchestrator* Orchestrator::Instance = nullptr;
bool Orchestrator::Destroyed = false;

Orchestrator::Orchestrator()
  : Parser(std::make_unique<ConfigParser>())
  , LPTypeCallbacks(static_cast<int>(CodesLPTypes::NumberOfTypes))
  , NetworkIdCallbacks(static_cast<int>(CodesLPTypes::NumberOfTypes))
{
  // sets MPI_COMM_CODES to MPI_COMM_ROSS
  // Can be updated when calling ConfigureSimulation
  // tw_init should be called before ever trying to get an instance of the Orchestrator
  // since it may split MPI_COMM_ROSS
  codes_comm_update();
}

Orchestrator::~Orchestrator()
{
  Instance = nullptr;
  Destroyed = true;
}

Orchestrator& Orchestrator::GetInstance()
{
  if (!Instance)
  {
    if (Destroyed)
    {
      throw std::runtime_error("Dead reference to Orchestrator singleton");
    }
    CreateInstance();
  }
  return *Instance;
}

void Orchestrator::CreateInstance()
{
  static Orchestrator theInstance;
  Instance = &theInstance;
}

void Orchestrator::ParseConfig(const std::string& configFileName)
{
  if (configFileName.empty())
  {
    // TODO error
  }

  std::filesystem::path yamlPath(configFileName);
  if (!std::filesystem::exists(yamlPath))
  {
    tw_error(TW_LOC, "the config file %s does not exist", configFileName.c_str());
  }

  this->ParentDir = yamlPath.parent_path();
  this->Parser->ParseConfig(configFileName);
  this->SimConfig = this->Parser->GetSimulationConfig();
  this->LPConfigs = this->Parser->GetLPTypeConfigs();
  this->Graph = this->Parser->ParseGraphVizConfig();
  this->_Mapper.Init();
}

void Orchestrator::ConfigureSimulation(const std::string& configFileName)
{
  this->ParseConfig(configFileName);

  // right now we're registering all non-model-net lps here
  // TODO: eventually make all LPs registered this way?
  for (const auto& config : this->LPConfigs)
  {
    RegisterLPTypeCallback callback = nullptr;
    if (config.ModelType == CodesLPTypes::Custom)
    {
      auto it = this->CustomLPTypeInfo.find(config.ModelName);
      if (it != this->CustomLPTypeInfo.end())
      {
        callback = it->second.RegistrationFn;
      }
    }
    else
    {
      callback = this->LPTypeCallbacks[static_cast<int>(config.ModelType)];
    }
    if (callback)
    {
      callback();
    }
  }

  model_net_register();
  this->_Mapper.MappingSetup();

  this->NetworkIds = model_net_configure(&this->NumberOfNetworks);
  int net_id = this->NetworkIds[0];
  // TODO: need to set the network id. network id is the id of the type of model net LP that a given
  // non model-net lp will be sending to. so right now, we only have one lp type like that, but
  // we need to figure out how to generally handle this. I think we'll also need to handle the case
  // where different instances of servers will need to connect to different types of model-net lps
  // so how to figure that out?
  for (const auto& config : this->LPConfigs)
  {
    RegisterNetworkIdCallback callback = nullptr;
    if (config.ModelType == CodesLPTypes::Custom)
    {
      auto it = this->CustomLPTypeInfo.find(config.ModelName);
      if (it != this->CustomLPTypeInfo.end())
      {
        callback = it->second.NetworkIdFn;
      }
    }
    else
    {
      callback = this->NetworkIdCallbacks[static_cast<int>(config.ModelType)];
    }
    if (callback)
    {
      assert(this->NumberOfNetworks == 1);
      callback(this->NetworkIds[0]);
    }
  }
}

void Orchestrator::ConfigureSimulation(const std::string& configFileName, MPI_Comm comm)
{
  MPI_COMM_CODES = comm;
  this->ConfigureSimulation(configFileName);
}

bool Orchestrator::RegisterLPType(
  CodesLPTypes type, RegisterLPTypeCallback registrationFn, RegisterNetworkIdCallback netIdFn)
{
  if (!registrationFn)
  {
    // only the registrationFn is required
    return false;
  }

  if (type < CodesLPTypes::NumberOfTypes)
  {
    this->LPTypeCallbacks[static_cast<int>(type)] = registrationFn;
    this->NetworkIdCallbacks[static_cast<int>(type)] = netIdFn;
    return true;
  }

  if (type == CodesLPTypes::Custom)
  {
    std::cout << "When using a custom LP type, you must use the RegisterLPType(string, "
                 "RegisterLPTypeCallback, RegisterNetworkIdCallback) signature"
              << std::endl;
  }
  return false;
}

bool Orchestrator::RegisterLPType(const std::string& typeName,
  RegisterLPTypeCallback registrationFn, RegisterNetworkIdCallback netIdFn)
{
  if (!registrationFn)
  {
    // only the registrationFn is required
    return false;
  }

  auto type = ConvertLPTypeNameToEnum(typeName);
  if (type < CodesLPTypes::NumberOfTypes)
  {
    this->LPTypeCallbacks[static_cast<int>(type)] = registrationFn;
    this->NetworkIdCallbacks[static_cast<int>(type)] = netIdFn;
    return true;
  }
  else if (type == CodesLPTypes::Custom)
  {
    if (this->CustomLPTypeInfo.count(typeName))
    {
      std::cout << "WARNING the callbacks for LP type " << typeName
                << " have already been registered. Check to make sure you didn't give the same "
                   "name to different LP types."
                << std::endl;
      return false;
    }

    this->CustomLPTypeInfo[typeName] = LPTypeInfo();
    this->CustomLPTypeInfo[typeName].RegistrationFn = registrationFn;
    this->CustomLPTypeInfo[typeName].NetworkIdFn = netIdFn;
    return true;
  }
  return false;
}

const tw_lptype* Orchestrator::LPTypeLookup(const std::string& name)
{
  return lp_type_lookup(name.c_str());
}

void Orchestrator::ReportModelNetStats()
{
  for (int i = 0; i < this->NumberOfNetworks; i++)
  {
    model_net_report_stats(this->NetworkIds[i]);
  }
}

std::string Orchestrator::GetParentPath()
{
  return this->ParentDir;
}

LPTypeConfig& Orchestrator::GetLPConfig(const std::string& name)
{
  for (auto& config : this->LPConfigs)
  {
    if (config.ModelName == name)
    {
      return config;
    }
  }
  tw_error(TW_LOC, "could not find config for lp type %s\n", name.c_str());
}

} // end namespace codes
