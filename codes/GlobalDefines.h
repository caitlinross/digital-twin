//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef GLOBAL_DEFINES_H
#define GLOBAL_DEFINES_H

// This is a temporary measure. currently codes contains a lot of global defines
// and variables that are initialized all over the place. As part of the cleanup
// we'll move them here until we get rid of the globals

#define CONFIGURATION_MAX_ANNOS 10
#define CONFIGURATION_MAX_NAME 128

// use the X-macro to get types and names rolled up into one structure
// format: { enum vals, config name, internal lp name, lp method struct}
// last value is sentinel
#define NETWORK_DEF                                                                                \
  X(SIMPLENET, "modelnet_simplenet", "simplenet", &simplenet_method)                               \
  X(SIMPLEP2P, "modelnet_simplep2p", "simplep2p", &simplep2p_method)                               \
  X(CONGESTION_CONTROLLER, "congestion_controller", "congestion_controller", NULL)                 \
  X(MAX_NETS, NULL, NULL, NULL)

#endif // !GLOBAL_DEFINES_H
