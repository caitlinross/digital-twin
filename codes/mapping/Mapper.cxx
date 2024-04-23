//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include <mpi.h>
#include <ross.h>

#include "codes/mapping/Mapper.h"
#include "codes/model-net/lp-type-lookup.h"
#include "codes/orchestrator/Orchestrator.h"
#include "codes/orchestrator/YAMLParser.h"

#include <graphviz/cgraph.h>

#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>

#define CODES_MAPPER_DEBUG 1

// TODO: use MPI_COMM_CODES instead of world
namespace codes
{

void codesMappingInit()
{
  // a little weird, but we need to pass a function pointer to ROSS, but
  // we can't provide a pointer to a non-static member function.
  auto& orchestrator = orchestrator::Orchestrator::GetInstance();
  auto self = orchestrator.GetMapper();
  self->MappingInit();
}

tw_lp* codesMappingToLP(tw_lpid lpid)
{
  // a little weird, but we need to pass a function pointer to ROSS, but
  // we can't provide a pointer to a non-static member function.
  auto& orchestrator = orchestrator::Orchestrator::GetInstance();
  auto self = orchestrator.GetMapper();
  return self->MappingToLP(lpid);
}

tw_peid CodesMapping(tw_lpid gid)
{
  auto& orchestrator = orchestrator::Orchestrator::GetInstance();
  auto self = orchestrator.GetMapper();
  return self->GlobalMapping(gid);
}

// simple class for keeping track of the graph
struct Node
{
  // global LP id
  tw_lpid GlobalId;

  // the name of the node used in the DOT
  std::string NodeName;

  // store a pointer to its type config info
  int ConfigIdx;

  // For now, we'll store pointers to the nodes we're connected to in
  // a vector. Maybe we'll want to change this in the future, but should be
  // fine for now.
  // I think these need to be weak_ptrs so we don't get cyclical references
  // that will never be freed
  std::vector<std::weak_ptr<Node>> Connections;
};

std::ostream& operator<<(std::ostream& os, const Node& node)
{
  os << "Node: \n"
     << "\tGlobal Id: " << node.GlobalId << "\n\tNodeName: " << node.NodeName
     << "\n\tConfigIdx: " << node.ConfigIdx << "\n\tConnected to: ";
  for (auto connNode : node.Connections)
  {
    auto ptr = connNode.lock();
    if (ptr)
    {
      os << ptr->NodeName << ", ";
    }
  }
  return os;
}

int findConfigIndex(
  const std::vector<orchestrator::LPTypeConfig>& lpConfigs, const std::string& nodeName)
{
  for (int idx = 0; idx < lpConfigs.size(); idx++)
  {
    for (const auto& name : lpConfigs[idx].NodeNames)
    {
      if (nodeName == name)
      {
        return idx;
      }
    }
  }

  return -1;
}

Mapper::Mapper(std::shared_ptr<orchestrator::YAMLParser> parser)
  : Parser(parser)
{
  // construct the nodes
  const auto& lpConfigs = this->Parser->GetLPTypeConfigs();
  const auto& typeIndices = this->Parser->GetLPTypeConfigIndices();
  auto graph = this->Parser->GetGraphConfig().GetGraph();

  auto& tree = this->Parser->GetYAMLTree();

  std::set<std::string> uniqueNodes;
  this->Nodes.resize(agnnodes(graph));
  int nodesIdx = 0;

  /* loop thru the subgraphs */
  // global lp id is determined based on the order we see nodes in the DOT
  for (Agraph_t* sub = agfstsubg(graph); sub; sub = agnxtsubg(sub))
  {
    // subgraph name ends up being "cluster_*"
    // how to get the label?
    // to get the label call either:
    // agget(sub, "label");
    // or
    // agxget(sub, attr);
    // first check to see if we've already processed the node.
    // since we require an undirected graph, the first time we see/process
    // a node may be when it is seen as a connection of another node
    // printf("Subraph name: %s\n", agnameof(sub));
    for (Agnode_t* v = agfstnode(sub); v; v = agnxtnode(sub, v))
    {
      // printf("\tvertex: %s\n", agnameof(v));
      if (uniqueNodes.count(agnameof(v)))
      {
        // we've processed this node already
        // std::cout << "\t\t\twe've already processed this node. skipping" << std::endl;
        continue;
      }
      uniqueNodes.insert(agnameof(v));
      auto node = std::make_shared<Node>();
      node->NodeName = agnameof(v);
      node->GlobalId = nodesIdx;
      node->ConfigIdx = findConfigIndex(lpConfigs, node->NodeName);
      this->Nodes[nodesIdx] = node;
      nodesIdx++;
      for (Agedge_t* e = agfstout(sub, v); e; e = agnxtout(sub, e))
      {
        // printf("\t\tconnected to node: %s\n", agnameof(e->node));
        //  now we need to create the node its connected to if it doesn't exist
        if (uniqueNodes.count(agnameof(e->node)) == 0)
        {
          // std::cout << "creating connected node" << std::endl;
          uniqueNodes.insert(agnameof(e->node));
          auto connNode = std::make_shared<Node>();
          connNode->NodeName = agnameof(e->node);
          connNode->GlobalId = nodesIdx;
          connNode->ConfigIdx = findConfigIndex(lpConfigs, connNode->NodeName);
          // go ahead and connect this to node
          connNode->Connections.push_back(node);
          node->Connections.push_back(connNode);
          this->Nodes[nodesIdx] = connNode;
          nodesIdx++;
        }
        else
        {
          // TODO: doesn't happen in my current example, but probably could
          // happen in other graphs
          std::cout << "node already exists. need to update connections" << std::endl;
        }
      }
    }
  }
  // TODO: need to process the links outside of subgraphs

  std::cout << "final node configs:" << std::endl;
  for (int i = 0; i < this->Nodes.size(); i++)
  {
    std::cout << *this->Nodes[i] << std::endl;
  }

  for (int lpid = 0; lpid < typeIndices.size(); lpid++)
  {
  }
}

Mapper::~Mapper() = default;

int Mapper::GetLPsForPE()
{
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
#if CODES_MAPPER_DEBUG
  std::cout << this->LPsPerPEFloor + (static_cast<tw_lpid>(g_tw_mynode) < this->LPsLeftover)
            << " lps for rank " << rank << std::endl;
#endif
  return this->LPsPerPEFloor + (static_cast<tw_lpid>(g_tw_mynode) < this->LPsLeftover);
}

tw_peid Mapper::GlobalMapping(tw_lpid gid)
{
  tw_lpid lps_on_pes_with_leftover = this->LPsLeftover * (this->LPsPerPEFloor + 1);
  if (gid < lps_on_pes_with_leftover)
  {
    return gid / (this->LPsPerPEFloor + 1);
  }
  else
  {
    return (gid - lps_on_pes_with_leftover) / this->LPsPerPEFloor + this->LPsLeftover;
  }
}

void Mapper::MappingSetup(int offset)
{
  int grp, lpt, message_size;
  int pes = tw_nnodes();

  auto& lpConfigs = this->Parser->GetLPTypeConfigs();
  this->LPsPerPEFloor = 0;
  for (const auto& config : lpConfigs)
  {
    this->LPsPerPEFloor += config.NodeNames.size();
  }

  tw_lpid global_nlps = this->LPsPerPEFloor;
  this->LPsLeftover = this->LPsPerPEFloor % pes;
  this->LPsPerPEFloor /= pes;

  g_tw_mapping = CUSTOM;
  g_tw_custom_initial_mapping = &codesMappingInit;
  g_tw_custom_lp_global_to_local_map = &codesMappingToLP;

  const auto& simConfig = this->Parser->GetSimulationConfig();

  // configure mem-factor
  // TODO:
  // int mem_factor_conf;
  // int rc = configuration_get_value_int(&config, "PARAMS", "pe_mem_factor", NULL,
  //        &mem_factor_conf);
  // if (rc == 0 && mem_factor_conf > 0)
  //  mem_factor = mem_factor_conf;
  g_tw_events_per_pe = this->MemFactor * this->GetLPsForPE();

  // TODO: check for message_size in config and set to default value if not set
  // TODO: eventually have a way to figure out what the value should be during set up
  // and remove it completely as a config option
  // configuration_get_value_int(&config, "PARAMS", "message_size", NULL, &message_size);
  // if(!message_size)
  //{
  //    message_size = 256;
  //    printf("\n Warning: ross message size not defined, resetting it to %d", message_size);
  //}

  // we increment the number of RNGs used to let codes_local_latency use the
  // last one
  g_tw_nRNG_per_lp++;
  g_tw_nRNG_per_lp++; // Congestion Control gets its own RNG - second to last (CLL is last)

  tw_define_lps(this->GetLPsForPE(), simConfig.ROSSMessageSize);

  // use a similar computation to codes_mapping_init to compute the lpids and
  // offsets to use in tw_rand_initial_seed
  // an "offset" of 0 reverts to default RNG seeding behavior - see
  // ross/rand-clcg4.c for the specific computation
  // an "offset" < 0 is ignored
  if (offset > 0)
  {
    for (tw_lpid l = 0; l < g_tw_nlp; l++)
    {
      for (unsigned int i = 0; i < g_tw_nRNG_per_lp; i++)
      {
        tw_rand_initial_seed(&g_tw_lp[l]->rng[i],
          (g_tw_lp[l]->gid + global_nlps * offset) * g_tw_nRNG_per_lp + i, NULL);
      }
    }
  }
}

void Mapper::MappingInit()
{
  int grp_id, lpt_id, rep_id, offset;
  tw_lpid ross_gid, ross_lid; /* ross global and local IDs */
  tw_pe* pe;
  tw_lpid lpid, kpid;
  const tw_lptype* lptype;
  const st_model_types* trace_type;

  const auto& configIndices = this->Parser->GetLPTypeConfigIndices();
  const auto& lpConfigs = this->Parser->GetLPTypeConfigs();

  /* have 16 kps per pe, this is the optimized configuration for ROSS custom mapping */
  for (kpid = 0; kpid < g_tw_nkp; kpid++)
  {
    tw_kp_onpe(kpid, g_tw_pe);
  }

  tw_lpid lp_start = g_tw_mynode * this->LPsPerPEFloor +
                     std::min(static_cast<tw_lpid>(g_tw_mynode), this->LPsLeftover);
  tw_lpid lp_end = (g_tw_mynode + 1) * this->LPsPerPEFloor +
                   std::min(static_cast<tw_lpid>(g_tw_mynode) + 1, this->LPsLeftover);

  for (lpid = lp_start; lpid < lp_end; lpid++)
  {
    ross_gid = lpid;
    ross_lid = lpid - lp_start;
    kpid = ross_lid % g_tw_nkp;
    pe = g_tw_pe;

    // codes_mapping_get_lp_info(
    //   ross_gid, NULL, &grp_id, lp_type_name, &lpt_id, NULL, &rep_id, &offset);

#if CODES_MAPPER_DEBUG
    std::cout << "lp: " << ross_gid << " --> kp: " << kpid << ", pe: " << pe->id << std::endl;
#endif
    tw_lp_onpe(ross_lid, pe, ross_gid);
    tw_lp_onkp(g_tw_lp[ross_lid], g_tw_kp[kpid]);

    std::string lp_type_name = lpConfigs[configIndices[ross_lid]].ModelName;
    lptype = lp_type_lookup(lp_type_name.c_str());
    if (lptype == NULL)
      tw_error(TW_LOC,
        "could not find LP with type name \"%s\", "
        "did you forget to register the LP?\n",
        lp_type_name.c_str());
    else
      /* sorry, const... */
      tw_lp_settype(ross_lid, (tw_lptype*)lptype);
    if (g_st_ev_trace || g_st_model_stats || g_st_use_analysis_lps)
    {
      trace_type = st_model_type_lookup(lp_type_name.c_str());
      st_model_settype(ross_lid, (st_model_types*)trace_type);
    }
  }
  return;
}

/* This function takes the global LP ID, maps it to the local LP ID and returns the LP
 * lps have global and local LP IDs
 * global LP IDs are unique across all PEs, local LP IDs are unique within a PE */
tw_lp* Mapper::MappingToLP(tw_lpid lpid)
{
  int index = lpid - (g_tw_mynode * this->LPsPerPEFloor) -
              std::min(static_cast<tw_lpid>(g_tw_mynode), this->LPsLeftover);
  return g_tw_lp[index];
}

int Mapper::GetLPCount(const std::string& lp_type_name)
{
  // TODO: will probably get more complicated once we figure out how to refer
  // to different types of ML models? or maybe the ML model doesn't actually matter here,
  // we just need to know the number of LPs of this type regardless of what ML model they're using
  int count = 0;
  const auto& lpConfigs = this->Parser->GetLPTypeConfigs();
  for (const auto& conf : lpConfigs)
  {
    if (conf.ModelName == lp_type_name)
    {
      count += conf.NodeNames.size();
    }
  }
  std::cout << "num of " << lp_type_name << " lps: " << count << std::endl;
  return count;
}

tw_lpid Mapper::GetLPId(const std::string& lp_type_name, int offset)
{
  tw_lpid gid;

  return gid;
}

int Mapper::GetRelativeLPId(tw_lpid gid)
{
  // might want to get info about the type of LP first
  auto& configIndices = this->Parser->GetLPTypeConfigIndices();
  auto& configIdx = configIndices[gid];
  int lp_count = 0;
  for (int i = 0; i < gid; i++)
  {
    if (configIdx == configIndices[i])
    {
      lp_count++;
    }
  }
  return lp_count;
}

std::string Mapper::GetLPTypeName(tw_lpid gid)
{
  auto configIdx = this->Parser->GetLPTypeConfigIndices()[gid];
  return this->Parser->GetLPTypeConfigs()[configIdx].ModelName;
}

void Mapper::ConfigureMapping()
{
  // so I need the mapper to get the node info and
}

} // end namespace codes
