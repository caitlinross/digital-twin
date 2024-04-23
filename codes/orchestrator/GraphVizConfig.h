//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef CODES_GRAPH_VIZ_CONFIG_H
#define CODES_GRAPH_VIZ_CONFIG_H

#include <string>

#include <graphviz/cgraph.h>

namespace codes
{

class GraphVizConfig
{
public:
  void ParseConfig(const std::string& dotFile);

  Agraph_t* GetGraph();

private:
  Agraph_t* Graph;
};

} // end namespace codes

#endif // CODES_GRAPH_VIZ_CONFIG_H
