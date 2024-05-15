//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef SIMULATION_CONFIGURATION_H
#define SIMULATION_CONFIGURATION_H

#include <string>
#include <vector>

namespace codes
{

/**
 * Stores general simulation configuration
 */
struct SimulationConfig
{
  int PacketSize;
  int ROSSMessageSize;
  std::string ModelNetScheduler;
  std::vector<std::string> ModelNetOrder;

  // TODO: maybe should store these differently, so that when
  // a model asks for a value, it can determine that it was actually set and we're not just
  // using a bad value
  // for simplep2p
  std::string LatencyFileName;
  std::string BandwidthFileName;

  // for simplenet
  double NetworkStartupNS;
  int NetworkBandwidthMbps;
};

}

#endif // SIMULATION_CONFIGURATION_H
