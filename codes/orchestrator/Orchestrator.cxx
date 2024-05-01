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
#include "codes/mapping/Mapper.h"
#include "codes/orchestrator/YAMLParser.h"

#include "codes/model-net/model-net-lp.h"
#include "codes/model-net/model-net.h"
#include "codes/util/jenkins-hash.h"

#include <memory>
#include <ross.h>

#include <algorithm>
#include <iostream>
#include <vector>

namespace codes
{

Orchestrator* Orchestrator::Instance = nullptr;
bool Orchestrator::Destroyed = false;

Orchestrator::Orchestrator()
  : Comm(MPI_COMM_WORLD)
  , ConfiguredNetworks(MAX_NETS, 0)
  , _YAMLParser(std::make_shared<YAMLParser>())
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

std::shared_ptr<YAMLParser> Orchestrator::GetYAMLParser()
{
  return this->_YAMLParser;
}

void Orchestrator::ParseConfig(const std::string& configFileName)
{
  if (configFileName.empty())
  {
    // TODO error
  }

  this->_YAMLParser->ParseConfig(configFileName);
  this->_Mapper = std::make_shared<Mapper>(this->_YAMLParser);
  // TODO: this can't be called here because model net register has to happen first
  // this->_Mapper->MappingSetup(0);
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
  auto& lpConfigs = this->_YAMLParser->GetLPTypeConfigs();
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
  // auto& lpConfigs = this->_YAMLParser->GetLPTypeConfigs();
  auto& simConfig = this->_YAMLParser->GetSimulationConfig();
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

} // end namespace codes
