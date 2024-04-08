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
#include "codes/orchestrator/CodesYAML.h"

#include "codes/model-net/model-net.h"
#include "codes/model-net/model-net-lp.h"

#include <ross.h>

#include <iostream>

namespace codes
{
namespace orchestrator
{

Orchestrator* Orchestrator::Instance = nullptr;
bool Orchestrator::Destroyed = false;

Orchestrator::Orchestrator()
  : Comm(MPI_COMM_WORLD)
  , ConfiguredNetworks(MAX_NETS, 0) {  }

Orchestrator::~Orchestrator()
{
  Instance = nullptr;
  Destroyed = true;
}

// Perhpas Orchestrator should be a singleton
// there should only ever be one instance of it. then we can always have a way
// to get that instance
// so then we can have the ross init and mapping functions call a wrapper that will get the
// single instance and then call the appropriate method

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

  CodesYAML parser;
  parser.ParseConfig(configFileName, this->LPConfigs);
}

bool Orchestrator::IsSimulationConfigured()
{
  return this->SimulationConfigured;
}

void Orchestrator::ModelNetRegister()
{
  // So basically all we want to do here is check the lp types that are configured and for model-net
  // layer LPs (so any routers/switches essentially), note which ones we are using
  // then we need to call model_net_base_register to register them with ROSS. looks like we don't need
  // to update anything there
  for (const auto& config : this->LPConfigs)
  {
    for (int n = 0; n < MAX_NETS; n++)
    {
      if (!this->ConfiguredNetworks[n] && config.ModelName == model_net_lp_config_names[n])
 //             strcmp(model_net_lp_config_names[n], config.ModelName) == 0)
      {
        this->ConfiguredNetworks[n] = 1;
        break;
      }
    }
  }

  model_net_base_register(this->ConfiguredNetworks.data());
}

void Orchestrator::CodesMappingSetup()
{
  int numPEs = tw_nnodes();
  // first get total number of LPs
  tw_lpid globalNumLPs = 0;
  for (const auto& config : this->LPConfigs)
  {
    globalNumLPs += config.NodeNames.size();
  }
  tw_lpid lpsPerPE = globalNumLPs;
  tw_lpid lpsRemainder = lpsPerPE % numPEs;
  lpsPerPE /= numPEs;

  // TODO will need to check on those functions
  //g_tw_mapping=CUSTOM;
  //g_tw_custom_initial_mapping=&codes_mapping_init;
  //g_tw_custom_lp_global_to_local_map=&codes_mapping_to_lp;

  //// TODO need to add mem factor config

  //g_tw_events_per_pe = codes_mapping_get_lps_for_pe();
}

} // end namespace orchestrator
} // end namespace codes
