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

#include "codes/orchestrator/CodesYAML.h"

#include <mpi.h>

#include <string>

namespace codes
{
namespace orchestrator
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

  void ParseConfig(const std::string& configFileName);

  // TODO add ability to set MPI_COMM_CODES. just have default be MPI_COMM_WORLD

  bool ConfigureSimulation(const std::string& configFileName, MPI_Comm comm = MPI_COMM_WORLD);

  bool IsSimulationConfigured();

  // Register the LPs with model-net
  void ModelNetRegister();

  // map LPs to PEs
  void CodesMappingSetup();

private:
  Orchestrator();
  Orchestrator(const Orchestrator&) = delete;
  Orchestrator& operator=(const Orchestrator&) = delete;
  virtual ~Orchestrator();

  static void CreateInstance();

  static Orchestrator* Instance;
  static bool Destroyed;

  MPI_Comm Comm;
  std::string ConfigFileName;

  bool SimulationConfigured = false;
  std::vector<int> ConfiguredNetworks;
  std::vector<LPConfig> LPConfigs;

};

} // end namespace orchestrator
} // end namespace codes

#endif // CODES_ORCHESTRATOR_ORCHESTRATOR_H
