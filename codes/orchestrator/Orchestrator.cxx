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

#include <iostream>

namespace codes
{
namespace orchestrator
{

struct Orchestrator::OrchestratorImpl
{
  OrchestratorImpl(const std::string& configFileName, MPI_Comm comm)
    : Comm(comm)
    , ConfigFileName(configFileName)
  {

  }

  MPI_Comm Comm;
  std::string ConfigFileName;

  bool SimulationConfigured = false;
};

Orchestrator::Orchestrator(const std::string& configFileName, MPI_Comm comm /*= MPI_COMM_WORLD*/)
  : Impl(new OrchestratorImpl(configFileName, comm))
{
  if (this->Impl->ConfigFileName.empty())
  {
    // TODO error

  }

  CodesYAML parser;
  parser.ParseConfig(this->Impl->ConfigFileName);
}

Orchestrator::~Orchestrator() = default;

bool Orchestrator::IsSimulationConfigured()
{
  return this->Impl->SimulationConfigured;
}

} // end namespace orchestrator
} // end namespace codes
