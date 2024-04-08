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
// Perhpas Orchestrator should be a singleton
// there should only ever be one instance of it. then we can always have a way
// to get that instance
// so then we can have the ross init and mapping functions call a wrapper that will get the
// single instance and then call the appropriate method
struct Orchestrator::OrchestratorImpl
{
  OrchestratorImpl(const std::string& configFileName, MPI_Comm comm)
    : Comm(comm)
    , ConfigFileName(configFileName)
    , ConfiguredNetworks(MAX_NETS, 0)
  {

  }

  ~OrchestratorImpl()
  {
  }

  MPI_Comm Comm;
  std::string ConfigFileName;

  bool SimulationConfigured = false;
  std::vector<int> ConfiguredNetworks;
  std::vector<LPConfig> LPConfigs;
};

Orchestrator::Orchestrator(const std::string& configFileName, MPI_Comm comm /*= MPI_COMM_WORLD*/)
  : Impl(new OrchestratorImpl(configFileName, comm))
{
  if (this->Impl->ConfigFileName.empty())
  {
    // TODO error

  }

  CodesYAML parser;
  parser.ParseConfig(this->Impl->ConfigFileName, this->Impl->LPConfigs);
}

Orchestrator::~Orchestrator() = default;

bool Orchestrator::IsSimulationConfigured()
{
  return this->Impl->SimulationConfigured;
}

void Orchestrator::ModelNetRegister()
{
  // So basically all we want to do here is check the lp types that are configured and for model-net
  // layer LPs (so any routers/switches essentially), note which ones we are using
  // then we need to call model_net_base_register to register them with ROSS. looks like we don't need
  // to update anything there
  for (const auto& config : this->Impl->LPConfigs)
  {
    for (int n = 0; n < MAX_NETS; n++)
    {
      if (!this->Impl->ConfiguredNetworks[n] && config.ModelName == model_net_lp_config_names[n])
 //             strcmp(model_net_lp_config_names[n], config.ModelName) == 0)
      {
        this->Impl->ConfiguredNetworks[n] = 1;
        break;
      }
    }
  }

  model_net_base_register(this->Impl->ConfiguredNetworks.data());
}

void Orchestrator::CodesMappingSetup()
{
  int numPEs = tw_nnodes();
  // first get total number of LPs
  tw_lpid globalNumLPs = 0;
  for (const auto& config : this->Impl->LPConfigs)
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
