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
#include "codes/orchestrator/ConfigParser.h"

#include <memory>
#include <ross.h>

namespace codes
{

Orchestrator* Orchestrator::Instance = nullptr;
bool Orchestrator::Destroyed = false;

Orchestrator::Orchestrator()
  : Comm(MPI_COMM_WORLD)
  , Parser(std::make_shared<ConfigParser>())
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

std::shared_ptr<ConfigParser> Orchestrator::GetConfigParser()
{
  return this->Parser;
}

void Orchestrator::ParseConfig(const std::string& configFileName)
{
  if (configFileName.empty())
  {
    // TODO error
  }

  this->Parser->ParseConfig(configFileName);
  this->_Mapper = std::make_shared<Mapper>(this->Parser);
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

} // end namespace codes
