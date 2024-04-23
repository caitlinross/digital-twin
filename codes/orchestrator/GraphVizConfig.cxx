//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include "codes/orchestrator/GraphVizConfig.h"

#include <filesystem>
#include <graphviz/cgraph.h>
#include <iostream>

namespace codes
{

void GraphVizConfig::ParseConfig(const std::string& dotFile)
{
  if (!std::filesystem::exists(dotFile))
  {
    std::cout << "the topology file " << dotFile << " does not exist!" << std::endl;
  }

  // can't use ifstream since graphviz is a C lib
  FILE* fp = fopen(dotFile.c_str(), "r");
  // TODO check for errors

  this->Graph = agread(fp, 0);
  if (!this->Graph)
  {
    std::cout << "error: graph couldn't be read" << std::endl;
    return;
  }
}

Agraph_t* GraphVizConfig::GetGraph()
{
  return this->Graph;
}

} // end namespace codes
