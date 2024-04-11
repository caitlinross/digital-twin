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

#include <algorithm>
#include <iostream>

#define CODES_MAPPING_DEBUG 1
namespace codes
{
namespace orchestrator
{

static void codesMappingInit()
{
  Orchestrator::GetInstance().CodesMappingInit();
}

static tw_lp* codesMappingToLP(tw_lpid lpid)
{
  return Orchestrator::GetInstance().CodesMappingToLP(lpid);
}

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

  this->YAMLParser.ParseConfig(configFileName);
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
  auto& lpConfigs = this->YAMLParser.GetLPTypeConfigs();
  for (const auto& config : lpConfigs)
  {
    for (int n = 0; n < MAX_NETS; n++)
    {
      if (!this->ConfiguredNetworks[n] && config.ModelName == model_net_lp_config_names[n])
      {
        this->ConfiguredNetworks[n] = 1;
        // TODO: this doesn't have the full functionality of model_net_base_register
        // so model-net lps that have custom registration hooks aren't supported
        // also the ROSS event tracing stuff needs to be handled
        this->LPTypeRegister(config.ModelName, &model_net_base_lp);
        break;
      }
    }
  }
}

void Orchestrator::CodesMappingSetup()
{
  int numPEs = tw_nnodes();
  // first get total number of LPs
  tw_lpid globalNumLPs = 0;
  auto& lpConfigs = this->YAMLParser.GetLPTypeConfigs();
  for (const auto& config : lpConfigs)
  {
    globalNumLPs += config.NodeNames.size();
  }
  this->LPsPerPEFloor = globalNumLPs;
  this->LPsRemainder = this->LPsPerPEFloor % numPEs;
  this->LPsPerPEFloor /= numPEs;

  // TODO will need to check on those functions
  g_tw_mapping=CUSTOM;
  g_tw_custom_initial_mapping=&codesMappingInit;
  g_tw_custom_lp_global_to_local_map=&codesMappingToLP;

  // TODO need to add mem factor config
  int mem_factor = 1;
  g_tw_events_per_pe = mem_factor * this->CodesMappingGetLPsForPE();

  const auto& simConfig = this->YAMLParser.GetSimulationConfig();

  // we increment the number of RNGs used to let codes_local_latency use the
  // last one
  g_tw_nRNG_per_lp++;
  g_tw_nRNG_per_lp++; //Congestion Control gets its own RNG - second to last (CLL is last)

  tw_define_lps(this->CodesMappingGetLPsForPE(), simConfig.ROSSMessageSize);

  // TODO ignoring the offset stuff for the RNG
  // can add later if needed
}

int Orchestrator::CodesMappingGetLPsForPE()
{
  int rank;
  // TODO fix MPI_COMM_WORLD refs
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
#if CODES_MAPPING_DEBUG
  printf("%d lps for rank %d\n", this->LPsPerPEFloor + (g_tw_mynode < this->LPsRemainder), rank);
#endif
  return this->LPsPerPEFloor + ((tw_lpid)g_tw_mynode < this->LPsRemainder);
}

void Orchestrator::LPTypeRegister(const std::string& name, const tw_lptype* type)
{
  //LPNameMapping nameMap;
  //nameMap.Name = name;
  //nameMap.LPType = type;
  //this->LPNameMap.push_back(nameMap);
  this->LPNameMap[name] = type;
}

const tw_lptype* Orchestrator::LPTypeLookup(const std::string& name)
{
  return this->LPNameMap.count(name) ? this->LPNameMap[name] : nullptr;
}

void Orchestrator::CodesMappingInit()
{
  const auto& configIndices = this->YAMLParser.GetLPTypeConfigIndices();
  const auto& lpConfigs = this->YAMLParser.GetLPTypeConfigs();

  /* have 16 kps per pe, this is the optimized configuration for ROSS custom mapping */
  for(tw_lpid kpid = 0; kpid < g_tw_nkp; kpid++)
  {
    tw_kp_onpe(kpid, g_tw_pe);
  }

  tw_lpid lp_start = g_tw_mynode * this->LPsPerPEFloor +
    std::min(static_cast<unsigned long long>(g_tw_mynode),
             static_cast<unsigned long long>(this->LPsRemainder));
  tw_lpid lp_end = (g_tw_mynode + 1) * this->LPsPerPEFloor +
    std::min(static_cast<unsigned long long>(g_tw_mynode + 1),
             static_cast<unsigned long long>(this->LPsRemainder));

  for (tw_lpid lpid = lp_start; lpid < lp_end; lpid++)
  {
    tw_lpid ross_gid = lpid;
    tw_lpid ross_lid = lpid - lp_start;
    tw_lpid kpid = ross_lid % g_tw_nkp;
    tw_pe* pe = g_tw_pe;

    tw_lp_onpe(ross_lid, pe, ross_gid);
    tw_lp_onkp(g_tw_lp[ross_lid], g_tw_kp[kpid]);

    // now we need to figure out for each lpid, what kind of lp is it
    // so before we can go any further, we need to read in the DOT file to
    // get the topology, because that will be used for the mapping
    // that combined with the size of LPConfig->NodeNames, will help us to figure
    // out the lp type name
    // for now, we'll only do this for serial impls because we're working with
    // such small networks in phase I.
    // in phase II we can add some kind of graph partitioner so we can do parallel
    std::string lpTypeName = lpConfigs[configIndices[ross_lid]].ModelName;
    const tw_lptype* lptype = this->LPTypeLookup(lpTypeName);

#if CODES_MAPPING_DEBUG
         printf("lp:%lu --> kp:%lu, pe:%llu\n", ross_gid, kpid, pe->id);
#endif
    if (!lptype)
    {
      tw_error(TW_LOC, "could not find LP with type name \"%s\", "
              "did you forget to register the LP?\n", lpTypeName.c_str());
    }
    else
    {
      tw_lp_settype(ross_lid, (tw_lptype*) lptype);
    }

    //TODO need to do the event tracing stuff
  }

}

tw_lp* Orchestrator::CodesMappingToLP(tw_lpid lpid)
{
  int index = lpid - (g_tw_mynode * this->LPsPerPEFloor) -
      std::min(static_cast<unsigned long long>(g_tw_mynode),
               static_cast<unsigned long long>(this->LPsRemainder));
  return g_tw_lp[index];
}

} // end namespace orchestrator
} // end namespace codes
