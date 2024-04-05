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

#include <codes/orchestrator/Orchestrator.h>

#include <iostream>
#include <memory>

// For now this is just some code hacked together for testing while
// implementing other functionality. I'd like for the user to have a single
// executable they need to run and they're just creating their configuration file
// (or at most adding new models), instead of needing to write their own driver
// code for every new type of simulation they want to do
int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    std::cout << "a configuration file is required!" << std::endl;
    std::cout << "./codes-digital-twin <path/to/config.yml>" << std::endl;
    return EXIT_FAILURE;
  }
  std::string config = argv[1];
  // setup the orchestrator and pass the config file
  std::shared_ptr<codes::orchestrator::Orchestrator> orchestrator = std::make_shared<codes::orchestrator::Orchestrator>(config, MPI_COMM_WORLD);

  return EXIT_SUCCESS;
}
