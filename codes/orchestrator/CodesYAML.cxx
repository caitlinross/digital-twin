//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include "codes/orchestrator/CodesYAML.h"

#include <ryml.hpp>
#include <ryml_std.hpp>

#include <fstream>
#include <filesystem>
#include <iostream>

namespace codes
{
namespace orchestrator
{

CodesYAML::CodesYAML()
{


}

void CodesYAML::ParseConfig(const std::string& configFile)
{
  if (!std::filesystem::exists(configFile))
  {
    std::cout << "the config file " << configFile << " does not exist!" << std::endl;
  }

  std::ifstream ifs(configFile, std::ifstream::in);
  ifs.seekg(0, ifs.end);
  int length = ifs.tellg();
  ifs.seekg(0, ifs.beg);

  std::string yaml(length, ' ');
  ifs.read(&yaml[0], yaml.size());
  ifs.close();
  //std::cout << "yaml:\n" << yaml << std::endl;

  // start parsing the file
  ryml::Tree tree = ryml::parse_in_arena(ryml::to_csubstr(yaml));

  ryml::ConstNodeRef root = tree.rootref();

  for (ryml::ConstNodeRef const& child : root.children())
  {
    std::cout << child.key() << std::endl;
    for (ryml::ConstNodeRef const& gc : child.children())
    {
      std::cout << "\t" << gc.key() << std::endl;
    }
  }

}

} // end namespace orchestrator
} // end namespace codes
