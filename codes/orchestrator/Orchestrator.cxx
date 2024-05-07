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
#include "codes/lp-type-lookup.h"
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
  , RegistrationCallbacks(static_cast<int>(CodesLPTypes::NumberOfTypes))
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

  // right now we're registering all non-model-net lps here
  // TODO: eventually make all LPs registered this way?
  for (const auto& config : this->Parser->GetLPTypeConfigs())
  {
    auto callback = this->RegistrationCallbacks[static_cast<int>(config.ModelType)];
    if (callback)
    {
      callback();
    }
  }

  this->_Mapper = std::make_shared<Mapper>(this->Parser);
}

// TODO: move the functionality for lp_type_register/lookup to here?
bool Orchestrator::RegisterLPType(CodesLPTypes type, RegisterLPTypeCallback registrationFn)
{
  // TODO: add in some error checks
  this->RegistrationCallbacks[static_cast<int>(type)] = registrationFn;
  return true;
}

const tw_lptype* Orchestrator::LPTypeLookup(const std::string& name)
{
  return lp_type_lookup(name.c_str());
}

} // end namespace codes
