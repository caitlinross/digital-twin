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
  if (str.find("SimpleServer") != std::string::npos)
    return CodesLPTypes::SimpleServer;
  if (str.find("simplep2p") != std::string::npos)
    return CodesLPTypes::SimpleP2P;
  if (str.find("simplenet") != std::string::npos)
    return CodesLPTypes::SimpleNet;
  if (str.find("SyntheticWorkload") != std::string::npos)
    return CodesLPTypes::SyntheticWorkload;
  // we'll assume it's a custom lp type at this point
  return CodesLPTypes::Custom;
}

} // end namespace codes
