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
#include "codes/lp-type-lookup.h"
#include "codes/mapping/Mapper.h"
#include "codes/model-net/model-net.h"
#include "codes/orchestrator/ConfigParser.h"

#include <memory>
#include <ross.h>

namespace codes
{

Orchestrator* Orchestrator::Instance = nullptr;
bool Orchestrator::Destroyed = false;

Orchestrator::Orchestrator()
  : Comm(MPI_COMM_WORLD)
  , Parser(std::make_shared<ConfigParser>())
  , LPTypeCallbacks(static_cast<int>(CodesLPTypes::NumberOfTypes))
  , NetworkIdCallbacks(static_cast<int>(CodesLPTypes::NumberOfTypes))
{
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

std::shared_ptr<ConfigParser> Orchestrator::GetConfigParser()
{
  return this->Parser;
}

void Orchestrator::ParseConfig(const std::string& configFileName)
{
  if (configFileName.empty())
  {
    // TODO error
  }

  this->Parser->ParseConfig(configFileName);

  // right now we're registering all non-model-net lps here
  // TODO: eventually make all LPs registered this way?
  for (const auto& config : this->Parser->GetLPTypeConfigs())
  {
    auto callback = this->LPTypeCallbacks[static_cast<int>(config.ModelType)];
    if (callback)
    {
      callback();
    }
  }

  this->_Mapper = std::make_shared<Mapper>(this->Parser);

  model_net_register();
  this->_Mapper->MappingSetup();

  this->NetworkIds = model_net_configure(&this->NumberOfNetworks);
  int net_id = this->NetworkIds[0];
  // TODO: need to set the network id. network id is the id of the type of model net LP that a given
  // non model-net lp will be sending to. so right now, we only have one lp type like that, but
  // we need to figure out how to generally handle this. I think we'll also need to handle the case
  // where different instances of servers will need to connect to different types of model-net lps
  // so how to figure that out?
  for (const auto& config : this->Parser->GetLPTypeConfigs())
  {
    auto callback = this->NetworkIdCallbacks[static_cast<int>(config.ModelType)];
    if (callback)
    {
      assert(this->NumberOfNetworks == 1);
      callback(this->NetworkIds[0]);
    }
  }
}

// TODO: move the functionality for lp_type_register/lookup to here?
bool Orchestrator::RegisterLPType(
  CodesLPTypes type, RegisterLPTypeCallback registrationFn, RegisterNetworkIdCallback netIdFn)
{
  // TODO: add in some error checks
  this->LPTypeCallbacks[static_cast<int>(type)] = registrationFn;
  this->NetworkIdCallbacks[static_cast<int>(type)] = netIdFn;
  return true;
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

} // end namespace codes
