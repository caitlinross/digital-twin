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

#include "codes/CodesPropertyCollection.h"
#include "codes/LPTypeConfiguration.h"
#include "codes/SupportedLPTypes.h"
#include "codes/mapping/Mapper.h"
#include "codes/orchestrator/ConfigParser.h"

#include <mpi.h>
#include <ross.h>

#include <map>
#include <string>

namespace codes
{

/**
 * The Orchestrator takes in a config file and MPI communicator (default is MPI_COMM_ROSS)
 * and sets up the LPs, assignment to PEs, and mapping based on the topology.
 */
class Orchestrator
{
public:
  /**
   * Get the global Orchestrator instance
   */
  static Orchestrator& GetInstance();

  // following two are very similar, except that ConfigureSimulation will do all of the
  // configuration (including ParseConfig)
  // Using ParseConfig directly enables you to have more control over setup, similarly to the way
  // codes used to work, while ConfigureSimulation will have the Orchestrator take care of
  // everything
  void ParseConfig(const std::string& configFileName);
  void ConfigureSimulation(const std::string& configFileName);

  // Sets MPI_COMM_CODES to comm (it's MPI_COMM_ROSS by default).
  void ConfigureSimulation(const std::string& configFileName, MPI_Comm comm);

  typedef void (*RegisterLPTypeCallback)();
  typedef void (*RegisterNetworkIdCallback)(int netId);
  bool RegisterLPType(
    CodesLPTypes type, RegisterLPTypeCallback registrationFn, RegisterNetworkIdCallback netIdFn);
  bool RegisterLPType(const std::string& typeName, RegisterLPTypeCallback registrationFn,
    RegisterNetworkIdCallback netIdFn);

  const tw_lptype* LPTypeLookup(const std::string& name);

  Mapper& GetMapper() { return this->_Mapper; }

  int GetNumberOfNetworks() { return this->NumberOfNetworks; }

  int* GetNetworkIds() { return this->NetworkIds; }

  void ReportModelNetStats();

  /**
   * Returns the directory containing the yaml file
   */
  std::string GetParentPath();

  CodesPropertyCollection& GetSimulationConfig() { return this->SimConfig; }
  const std::vector<LPTypeConfig>& GetLPTypeConfigs() { return this->LPConfigs; }
  LPTypeConfig& GetLPConfig(const std::string& name);
  Agraph_t* GetGraph() { return this->Graph; }

private:
  Orchestrator();
  Orchestrator(const Orchestrator&) = delete;
  Orchestrator& operator=(const Orchestrator&) = delete;
  virtual ~Orchestrator();

  static void CreateInstance();

  static Orchestrator* Instance;
  static bool Destroyed;

  std::string ParentDir;

  std::unique_ptr<ConfigParser> Parser;
  Mapper _Mapper;
  Agraph_t* Graph;

  CodesPropertyCollection SimConfig;
  std::vector<LPTypeConfig> LPConfigs;

  // enabling automatic registration of LP types
  std::vector<RegisterLPTypeCallback> LPTypeCallbacks;
  std::vector<RegisterNetworkIdCallback> NetworkIdCallbacks;

  // this map is for storing custom LP type info
  // (ie lp types created outside of codes lib)
  // TODO: maybe just use a map for everything and not worry about keeping them separate?
  struct LPTypeInfo
  {
    RegisterLPTypeCallback RegistrationFn;
    RegisterNetworkIdCallback NetworkIdFn;
  };

  std::map<std::string, LPTypeInfo> CustomLPTypeInfo;

  // need to store network ids from model_net_configure
  int* NetworkIds;
  int NumberOfNetworks;
};

} // end namespace codes

#endif // CODES_ORCHESTRATOR_ORCHESTRATOR_H
