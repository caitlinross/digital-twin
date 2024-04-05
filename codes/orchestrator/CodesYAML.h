//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef CODES_ORCHESTRATOR_CODESYAML_H
#define CODES_ORCHESTRATOR_CODESYAML_H

#include <string>

namespace codes
{
namespace orchestrator
{

class CodesYAML
{
public:
  CodesYAML();

  void ParseConfig(const std::string& configFile);

private:
  std::string YamlString;

};

} // end namespace orchestrator
} // end namespace codes

#endif // CODES_ORCHESTRATOR_CODESYAML_H
