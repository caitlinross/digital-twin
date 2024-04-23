//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef CODES_MAPPER_H
#define CODES_MAPPER_H

#include <ross.h>

#include "codes/orchestrator/YAMLParser.h"

#include <memory>
#include <string>
#include <vector>

namespace codes
{

struct Node;

/**
 * The Mapper class takes the YAML and DOT configuration files, determines
 * the mapping of LP types to global ids and provides methods that are used to
 * convert back and forth between the types of information.
 */
class Mapper
{
public:
  // the orchestrator can setup the class
  Mapper(std::shared_ptr<orchestrator::YAMLParser> parser);
  ~Mapper();

  /**
   * Using the YAML, determine the number of LPs on each PE
   * Currently it's just based on the order the LPs are defined in the YAML file,
   * since we're currently just working with very small networks and will be
   * running serially. To run in parallel, we could use the info from
   * the graph to also better provide a better partitioning across ranks.
   */
  void MappingSetup() { this->MappingSetup(0); }
  void MappingSetup(int offset);

  /**
   * Provided to ROSS to set up the initial mapping
   */
  void MappingInit();

  tw_lp* MappingToLP(tw_lpid lpid);

  /**
   * Returns the number of LPs on this PE
   */
  int GetLPsForPE();

  /**
   * Given a global LP ID, return the rank/PE it belongs to
   */
  tw_peid GlobalMapping(tw_lpid gid);

  /**
   * Calculates the count for LPs of the given type
   */
  int GetLPCount(const std::string& lp_type_name);

  /**
   * Calculates the global LP Id given the identifying info
   */
  tw_lpid GetLPId(const std::string& lp_type_name, int offset);

  /**
   * Calculates the LP id relative to the other LPs of its type
   */
  int GetRelativeLPId(tw_lpid gid);

  std::string GetLPTypeName(tw_lpid gid);

private:
  // perhaps this will get the yaml parser and stuff, and then use that to
  // create all the relevant data structures for setting up the mapping
  // LPConfigs is a vector of the different types of LPs
  // LPTypeConfigIndices is a vector of indices into the LPConfigs - but maybe
  // we won't need it eventually
  // RouterConnectivity - a matrix of the communication between routers
  std::shared_ptr<orchestrator::YAMLParser> Parser;

  // Keep track of info for each node in the network. Stored in a vector
  // so we can have O(1) access when we have the global LP id, and in the case
  // when we're looking who to send to, we'll have the id of the sender,
  // and then we can go into the Node itself to find the correct LP to send
  // to and its id.
  std::vector<std::shared_ptr<Node>> Nodes;

  tw_lpid LPsPerPEFloor = 0;
  tw_lpid LPsLeftover = 0;

  int MemFactor = 256;

  void ConfigureMapping();
};

tw_peid CodesMapping(tw_lpid gid);

} // end namespace codes

#endif // CODES_MAPPER_H
