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

#include "codes/model-net/model-net-lp.h"
#include "codes/model-net/model-net.h"
#include "codes/util/jenkins-hash.h"

#include <ross.h>

#include <algorithm>
#include <iostream>
#include <vector>

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
  , ConfiguredNetworks(MAX_NETS, 0)
  , YAMLParser(std::make_shared<CodesYAML>())
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

std::shared_ptr<CodesYAML> Orchestrator::GetYAMLParser()
{
  return this->YAMLParser;
}

void Orchestrator::ParseConfig(const std::string& configFileName)
{
  if (configFileName.empty())
  {
    // TODO error
  }

  this->YAMLParser->ParseConfig(configFileName);
}

bool Orchestrator::IsSimulationConfigured()
{
  return this->SimulationConfigured;
}

void Orchestrator::ModelNetRegister()
{
  // So basically all we want to do here is check the lp types that are
  // configured and for model-net layer LPs (so any routers/switches
  // essentially), note which ones we are using then we need to call
  // model_net_base_register to register them with ROSS. looks like we don't
  // need to update anything there
  auto& lpConfigs = this->YAMLParser->GetLPTypeConfigs();
  for (const auto& config : lpConfigs)
  {
    for (int n = 0; n < MAX_NETS; n++)
    {
      if (!this->ConfiguredNetworks[n] && config.ModelName == model_net_lp_config_names[n])
      {
        this->ConfiguredNetworks[n] = 1;
        // TODO: this doesn't have the full functionality of
        // model_net_base_register so model-net lps that have custom
        // registration hooks aren't supported also the ROSS event tracing stuff
        // needs to be handled
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
  auto& lpConfigs = this->YAMLParser->GetLPTypeConfigs();
  for (const auto& config : lpConfigs)
  {
    globalNumLPs += config.NodeNames.size();
  }
  this->LPsPerPEFloor = globalNumLPs;
  this->LPsRemainder = this->LPsPerPEFloor % numPEs;
  this->LPsPerPEFloor /= numPEs;

  g_tw_mapping = CUSTOM;
  g_tw_custom_initial_mapping = &codesMappingInit;
  g_tw_custom_lp_global_to_local_map = &codesMappingToLP;

  // TODO need to add mem factor config
  int mem_factor = 1;
  g_tw_events_per_pe = mem_factor * this->CodesMappingGetLPsForPE();

  const auto& simConfig = this->YAMLParser->GetSimulationConfig();

  // we increment the number of RNGs used to let codes_local_latency use the
  // last one
  g_tw_nRNG_per_lp++;
  g_tw_nRNG_per_lp++; // Congestion Control gets its own RNG - second to last
                      // (CLL is last)

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
  // LPNameMapping nameMap;
  // nameMap.Name = name;
  // nameMap.LPType = type;
  // this->LPNameMap.push_back(nameMap);
  this->LPNameMap[name] = type;
}

const tw_lptype* Orchestrator::LPTypeLookup(const std::string& name)
{
  return this->LPNameMap.count(name) ? this->LPNameMap[name] : nullptr;
}

void Orchestrator::CodesMappingInit()
{
  const auto& configIndices = this->YAMLParser->GetLPTypeConfigIndices();
  const auto& lpConfigs = this->YAMLParser->GetLPTypeConfigs();

  /* have 16 kps per pe, this is the optimized configuration for ROSS custom
   * mapping */
  for (tw_lpid kpid = 0; kpid < g_tw_nkp; kpid++)
  {
    tw_kp_onpe(kpid, g_tw_pe);
  }

  tw_lpid lp_start =
    g_tw_mynode * this->LPsPerPEFloor + std::min(static_cast<unsigned long long>(g_tw_mynode),
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
    // that combined with the size of LPConfig->NodeNames, will help us to
    // figure out the lp type name for now, we'll only do this for serial impls
    // because we're working with such small networks in phase I. in phase II we
    // can add some kind of graph partitioner so we can do parallel
    std::string lpTypeName = lpConfigs[configIndices[ross_lid]].ModelName;
    const tw_lptype* lptype = this->LPTypeLookup(lpTypeName);

#if CODES_MAPPING_DEBUG
    printf("lp:%lu --> kp:%lu, pe:%llu\n", ross_gid, kpid, pe->id);
#endif
    if (!lptype)
    {
      tw_error(TW_LOC,
        "could not find LP with type name \"%s\", "
        "did you forget to register the LP?\n",
        lpTypeName.c_str());
    }
    else
    {
      tw_lp_settype(ross_lid, (tw_lptype*)lptype);
    }

    // TODO need to do the event tracing stuff
  }
}

tw_lp* Orchestrator::CodesMappingToLP(tw_lpid lpid)
{
  int index = lpid - (g_tw_mynode * this->LPsPerPEFloor) -
              std::min(static_cast<unsigned long long>(g_tw_mynode),
                static_cast<unsigned long long>(this->LPsRemainder));
  return g_tw_lp[index];
}

void Orchestrator::ModelNetBaseConfigure()
{
  uint32_t h1 = 0, h2 = 0;

  bj_hashlittle2("model_net_base", strlen("model_net_base"), &h1, &h2);
  model_net_base_magic = h1 + h2;

  // set up offsets - doesn't matter if they are actually used or not
  SetMsgOffsets(SIMPLENET, offsetof(model_net_wrap_msg, msg.m_snet));
  SetMsgOffsets(SIMPLEP2P, offsetof(model_net_wrap_msg, msg.m_sp2p));
  SetMsgOffsets(CONGESTION_CONTROLLER, offsetof(model_net_wrap_msg, msg.m_cc));

  // need to read parameters of all model-net lps
  // and save the info into global vars annos and all_params
  // SetAnnos();
  // auto& lpConfigs = this->YAMLParser->GetLPTypeConfigs();
  auto& simConfig = this->YAMLParser->GetSimulationConfig();
  // for (int lp = 0; lp < lpConfigs.size(); lp++)
  {
    // TODO So this should be done per anno or num_params, but i don't fully understand
    // in the simple case, I know we just need to set it for the first params
    model_net_base_params* params = GetAllParams(0);
    int i;
    for (i = 0; i < MAX_SCHEDS; i++)
    {
      if (sched_names[i] == simConfig.ModelNetScheduler)
      {
        params->sched_params.type = static_cast<sched_type>(i);
        break;
      }
    }
    if (i == MAX_SCHEDS)
    {
      tw_error(TW_LOC,
        "Unknown value for PARAMS:modelnet-scheduler : "
        "%s",
        simConfig.ModelNetScheduler.c_str());
    }

    // TODO need to support the following config options
    params->num_queues = 1;
    if (!g_tw_mynode)
    {
      fprintf(stdout,
        "NIC num injection port not specified, "
        "setting to %d\n",
        params->num_queues);
    }

    params->nic_seq_delay = 10;
    if (!g_tw_mynode)
    {
      fprintf(stdout,
        "NIC seq delay not specified, "
        "setting to %lf\n",
        params->nic_seq_delay);
    }

    params->node_copy_queues = 1;
    if (!g_tw_mynode)
    {
      fprintf(stdout,
        "NIC num copy queues not specified, "
        "setting to %d\n",
        params->node_copy_queues);
    }

    // TODO add in handling of packet_size stuff
    params->packet_size = simConfig.PacketSize;
  }
}

std::vector<int> Orchestrator::ModelNetConfigure(int& id_count)
{
  this->ModelNetBaseConfigure();

  id_count = 0;
  for (const auto& net : this->ConfiguredNetworks)
  {
    if (net)
    {
      id_count++;
    }
  }

  std::vector<int> ids(id_count);
  // TODO short-term: need to actually impl this
  // TODO longer-term: need to figure out a way to hide this from the user
  std::vector<std::string> mnOrder{ "simplep2p" };
  if (mnOrder.size() != id_count)
  {
    tw_error(TW_LOC, "number of networks in modelnet_order "
                     "do not match number of configured networks \n");
  }

  // set the index
  for (int i = 0; i < id_count; i++)
  {
    ids[i] = -1;
    for (int n = 0; n < MAX_NETS; n++)
    {
      if (mnOrder[i] == model_net_method_names[n])
      {
        if (!this->ConfiguredNetworks[n])
        {
          tw_error(TW_LOC,
            "network in PARAMS:modelnet_order not "
            "present in LPGROUPS: %s\n",
            mnOrder[i].c_str());
        }
        ids[i] = n;
        break;
      }
    }
    if (ids[i] == -1)
    {
      tw_error(TW_LOC, "unknown network in PARAMS:modelnet_order: %s\n", mnOrder[i].c_str());
    }
  }

  // init the per-msg params here
  initMsgParamsSet();
  if (!g_tw_mynode)
  {
    fprintf(stderr,
      "Bandwidth of compute node channels not specified, "
      "setting to %lf\n",
      cn_bandwidth);
  }

  codes_cn_delay = 1 / cn_bandwidth;
  if (!g_tw_mynode)
  {
    printf("within node transfer per byte delay is %f\n", codes_cn_delay);
  }

  if (!g_tw_mynode)
  {
    fprintf(stderr,
      "Within-node eager limit (node_eager_limit) not specified, "
      "setting to %d\n",
      codes_node_eager_limit);
  }

  return ids;
}

} // end namespace orchestrator
} // end namespace codes
