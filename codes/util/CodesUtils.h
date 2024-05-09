//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef CODES_UTILS_H
#define CODES_UTILS_H

#include <ross.h>

// Defining some commonly used functions here that can be used throughout codes
namespace codes
{

/* convert ns to seconds */
inline tw_stime NSToSeconds(tw_stime ns)
{
  return (ns / (1000.0 * 1000.0 * 1000.0));
}

/* convert seconds to ns */
inline tw_stime SecondsToNS(tw_stime ns)
{
  return (ns * (1000.0 * 1000.0 * 1000.0));
}

} // end namespace codes

#endif
