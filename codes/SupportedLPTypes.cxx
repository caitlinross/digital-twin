//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include "codes/SupportedLPTypes.h"

namespace codes
{

CodesLPTypes ConvertLPTypeNameToEnum(std::string str)
{
  // TODO: convert to all lower, and perhaps just do a find?
  if (str == "SimpleServer")
    return CodesLPTypes::SimpleServer;
  if (str == "simplep2p")
    return CodesLPTypes::SimpleP2P;
  if (str == "simplenet")
    return CodesLPTypes::SimpleNet;
  if (str == "SyntheticWorkload")
    return CodesLPTypes::SyntheticWorkload;
  // we'll assume it's a custom lp type at this point
  return CodesLPTypes::Custom;
}

} // end namespace codes
