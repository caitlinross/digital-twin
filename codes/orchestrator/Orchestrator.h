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

#include "codes/mapping/Mapper.h"
#include "codes/orchestrator/ConfigParser.h"

#include <mpi.h>
#include <ross.h>

#include <map>
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

  void LPTypeRegister(const std::string& name, const tw_lptype* type);

  std::shared_ptr<ConfigParser> GetConfigParser();

  std::shared_ptr<Mapper> GetMapper() { return this->_Mapper; }

private:
  Orchestrator();
  Orchestrator(const Orchestrator&) = delete;
  Orchestrator& operator=(const Orchestrator&) = delete;
  virtual ~Orchestrator();

  static void CreateInstance();

  static Orchestrator* Instance;
  static bool Destroyed;

  MPI_Comm Comm;

  std::map<std::string, const tw_lptype*> LPNameMap;

  const tw_lptype* LPTypeLookup(const std::string& name);

  std::shared_ptr<ConfigParser> Parser;
  std::shared_ptr<Mapper> _Mapper;
};

} // end namespace codes

#endif // CODES_ORCHESTRATOR_ORCHESTRATOR_H
