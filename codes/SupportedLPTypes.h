//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef CODES_SUPPORTED_LP_TYPES_H
#define CODES_SUPPORTED_LP_TYPES_H

#include <string>
namespace codes
{

enum class CodesLPTypes
{
  SimpleServer,
  SimpleP2P,
  SimpleNet,
  // the following should always remain the final 2 entries in this enum
  NumberOfTypes,
  Unknown
};

CodesLPTypes ConvertLPTypeNameToEnum(std::string str);

} // end namespace codes

#endif // CODES_SUPPORTED_LP_TYPES_H
