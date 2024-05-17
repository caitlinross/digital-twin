//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef LP_TYPE_CONFIGURATION_H
#define LP_TYPE_CONFIGURATION_H

#include "codes/CodesPropertyCollection.h"
#include "codes/SupportedLPTypes.h"

#include <string>
#include <vector>

namespace codes
{

enum class ComponentType
{
  Switch,
  Router,
  Host
};

/**
 * Stores config info for a specific LP type
 */
struct LPTypeConfig
{
  ComponentType Type;
  std::string ConfigName; // name used in graphviz
  std::string ModelName;  // the codes LP name
  CodesLPTypes ModelType;
  std::vector<std::string> NodeNames;

  // store properties for this LP type
  CodesPropertyCollection Properties;
};

}

#endif // LP_TYPE_CONFIGURATION_H
