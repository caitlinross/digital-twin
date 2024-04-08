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
  Orchestrator(const std::string& configFileName, MPI_Comm comm = MPI_COMM_WORLD);
  ~Orchestrator();

  bool IsSimulationConfigured();

  // Register the LPs with model-net
  void ModelNetRegister();

  // map LPs to PEs
  void CodesMappingSetup();

private:

  struct OrchestratorImpl;
  std::unique_ptr<OrchestratorImpl> Impl;

};

} // end namespace orchestrator
} // end namespace codes

#endif // CODES_ORCHESTRATOR_ORCHESTRATOR_H
