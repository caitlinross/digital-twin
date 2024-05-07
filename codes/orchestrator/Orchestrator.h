//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef CODES_ORCHESTRATOR_ORCHESTRATOR_H
#define CODES_ORCHESTRATOR_ORCHESTRATOR_H

#include "codes/SupportedLPTypes.h"
#include "codes/mapping/Mapper.h"
#include "codes/orchestrator/ConfigParser.h"

#include <mpi.h>
#include <ross.h>

#include <string>

namespace codes
{

/**
 * The Orchestrator takes in a config file and MPI communicator (default is MPI_COMM_WORLD)
 * and sets up the LPs, assignment to PEs, and mapping based on the topology.
 */
class Orchestrator
{
public:
  /**
   * Get the global Orchestrator instance
   */
  static Orchestrator& GetInstance();

  // TODO: add ability to set MPI_COMM_CODES. just have default be MPI_COMM_WORLD
  void ParseConfig(const std::string& configFileName);

  typedef void (*RegisterLPTypeCallback)();
  typedef void (*RegisterNetworkIdCallback)(int netId);
  bool RegisterLPType(
    CodesLPTypes type, RegisterLPTypeCallback registrationFn, RegisterNetworkIdCallback netIdFn);

  const tw_lptype* LPTypeLookup(const std::string& name);

  std::shared_ptr<ConfigParser> GetConfigParser();

  std::shared_ptr<Mapper> GetMapper() { return this->_Mapper; }

  int GetNumberOfNetworks() { return this->NumberOfNetworks; }

  int* GetNetworkIds() { return this->NetworkIds; }

  void ReportModelNetStats();

private:
  Orchestrator();
  Orchestrator(const Orchestrator&) = delete;
  Orchestrator& operator=(const Orchestrator&) = delete;
  virtual ~Orchestrator();

  static void CreateInstance();

  static Orchestrator* Instance;
  static bool Destroyed;

  MPI_Comm Comm;

  std::shared_ptr<ConfigParser> Parser;
  std::shared_ptr<Mapper> _Mapper;

  // enabling automatic registration of LP types
  std::vector<RegisterLPTypeCallback> LPTypeCallbacks;
  std::vector<RegisterNetworkIdCallback> NetworkIdCallbacks;

  // need to store network ids from model_net_configure
  // TODO: change to use a vector instead
  int* NetworkIds;
  int NumberOfNetworks;
};

} // end namespace codes

#endif // CODES_ORCHESTRATOR_ORCHESTRATOR_H
